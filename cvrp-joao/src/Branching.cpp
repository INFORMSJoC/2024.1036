
#include "Branching.h"
#include "DisjointSets.h"
#include "Cutsets.h"

#include <algorithm>
#include <utility>
#include "bcModelNetworkFlow.hpp"
#include "bcProbConfigC.hpp"

cvrp_joao::UserBranchingFunctor::UserBranchingFunctor(const Data & data_,
                                                      const Parameters & params_,
                                                      const cluster::Clustering & clusters_,
                                                      fracclu::FractionalClustering & fractionalClustering_,
                                                      BranchingFeatures & branchingFeatures_):
        BcDisjunctiveBranchingConstrSeparationFunctor(), data(data_), params(params_), clusters(clusters_),
        fractionalClusters(fractionalClustering_), branchingFeatures(branchingFeatures_), constrCount(0), cutsetsIdMap()
{}

cvrp_joao::UserBranchingFunctor::~UserBranchingFunctor() = default;

bool cvrp_joao::UserBranchingFunctor::rootProcessed = false;

bool cvrp_joao::UserBranchingFunctor::operator() (BcFormulation master, BcSolution & primalSol,
                                                  std::list<std::pair<double, BcSolution>> & columnsInSol,
                                                  const int & candListMaxSize,
                                                  std::list<std::pair<BcConstr, std::string> > & returnBrConstrList)
{
    std::set<BcVar> xVarSet;
    primalSol.extractVar("X", xVarSet);

    BcBranchingConstrArray userBranching(master, "UBR");
    BcFormulation spForm = master.colGenSubProblemList().front();
    BcVarArray xVar(spForm, "X");

    std::vector<std::vector<double> > xSolution(data.nbCustomers + 1, std::vector<double>(data.nbCustomers + 1, 0.0));
    for (const auto & bcVar : xVarSet)
    {
        int firstNodeId = bcVar.id().first();
        int secondNodeId = bcVar.id().second();
        xSolution[firstNodeId][secondNodeId] = bcVar.solVal();
        xSolution[secondNodeId][firstNodeId] = bcVar.solVal();
    }

    if (!rootProcessed)
        saveRootFracSolution(xVarSet, data.name, params.rootFracSolutionFilePath());

    if (params.enableClusterBranching())
        bool success = branchingOverDefaultClusters(userBranching, xVar, xSolution, returnBrConstrList);

    if (params.enableRouteClusterBranching())
        bool success = branchingOverRouteClusters(userBranching, xVar, xSolution, returnBrConstrList);

    if (params.enableFracClustering())
        bool success = branchingOverFractionalClusters(userBranching, xVar, xSolution, returnBrConstrList);

    if (params.enableCutsetsBranching())
        bool success = branchingOverCutsets(userBranching, xVar, xSolution, returnBrConstrList);

    if (params.enableMinCut())
        bool success = computeMinCut(xVar, xSolution);

    if (params.enableBranchingFeatures())
    {
        auto xReducedCosts = getReducedCosts(spForm);
        auto xNbColumns = getVarNbColumns(columnsInSol);
        branchingFeatures.updateBranchingFeatures(data, xSolution, xReducedCosts, xNbColumns);
    }

    if (params.enableCostlyEdgeBranching())
        bool success = branchingOverCostlyEdges(userBranching, xVar, xSolution, returnBrConstrList,
                                                candListMaxSize);

    return true;
}

std::vector<std::vector<double>> cvrp_joao::UserBranchingFunctor::getReducedCosts(const BcFormulation & spForm) const
{
    std::vector<std::vector<double>> xReducedCost(data.nbCustomers + 1, std::vector<double>(data.nbCustomers + 1, 0.0));
    auto * probConfigPtr = spForm.probConfPtr();
    {
        auto & instVarPts = probConfigPtr->instVarPts();
        auto & instVarRedCosts = probConfigPtr->instVarRedCosts();

        int numVariables = (int)instVarPts.size();
        for (int varId = 0; varId < numVariables; ++varId)
        {
            auto * iVarPtr = instVarPts[varId];
            if (iVarPtr->genVarConstrPtr()->defaultName() != "X")
                continue;

            // firstNodeId = iVarPtr->id().first(), secondNodeId = iVarPtr->id().second();
            xReducedCost[iVarPtr->id().first()][iVarPtr->id().second()] = instVarRedCosts[varId];

            //if (firstNodeId == 2)
            //    std::cout << " RC[" << iVarPtr->name() << "]=" << instVarRedCosts[varId];
        }
    }

    return xReducedCost;
}

std::vector<std::vector<int>> cvrp_joao::UserBranchingFunctor::getVarNbColumns(
                                                    const std::list<std::pair<double, BcSolution>> & columnsInSol) const
{
    std::vector<std::vector<int>> xNbColumns(data.nbCustomers + 1, std::vector<int>(data.nbCustomers + 1, 0));

    for (const auto & pair : columnsInSol)
    {
        // auto value = pair.first;
        auto & solution = pair.second;
        auto & network = solution.formulation().network();
        auto & arcIds = solution.orderedIds();

        if (arcIds.empty())
            continue;

        std::set<std::pair<int, int>> xVarList;
        for (auto & arcId: arcIds)
        {
            int prev = network.getArc(arcId).tail().ref();
            int next = network.getArc(arcId).head().ref();
            int firstNode = std::min(prev, next), secondNode = std::max(prev, next);
            if (secondNode > data.nbCustomers)
            {
                firstNode = 0;
                secondNode = prev;
            }

            if (xVarList.count(std::make_pair(firstNode, secondNode)) != 0)
                continue;

            xNbColumns[firstNode][secondNode] += 1;
            xVarList.insert(std::make_pair(firstNode, secondNode));
        }
    }

    return xNbColumns;
}

bool cvrp_joao::UserBranchingFunctor::branchingOverDefaultClusters(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                                                  const std::vector<std::vector<double> > & xSolution,
                                                                  std::list<std::pair<BcConstr, std::string> > & returnBrConstrList)
{
    double threshold = 0.1; /// should be between 0.0 and 0.5
    /// In indices: the last pair identifies the branching
    std::vector<std::vector<std::pair<int, int>>> indices;
    /// In candidates: pair.first = pos. in indices, pair.second = fract. sol. part
    std::vector<std::pair<int, double>> candidates;

    // Branching over cluster degree
    for (const auto & cluster: clusters.clusters)
    {
        double totalXvalueDeg = 0;
        std::vector<std::pair<int, int>> indicesDeg;
        for (int i = 0; i <= data.nbCustomers; ++i)
        {
            if (clusters.customerInCluster(i, cluster.first))
                continue;

            for (const int & j : cluster.second)
            {
                int firstNodeId = std::min(i, j), secondNodeId = std::max(i, j);
                totalXvalueDeg += xSolution[firstNodeId][secondNodeId];
                indicesDeg.emplace_back(firstNodeId, secondNodeId);
            }
        }
        totalXvalueDeg /= 2.0; /// we should divide by 2 to have the number of paths
        double fractionalPart = totalXvalueDeg - (int) totalXvalueDeg;
        if (fractionalPart > threshold && fractionalPart < 1 - threshold)
        {
            int indId = indices.size();
            // last element identifies the branching name
            indicesDeg.emplace_back(cluster.first, -1);
            indices.push_back(indicesDeg);
            candidates.emplace_back(indId, fractionalPart);
        }
    }

    // Branching over aggregated edges between cluster
    for (int k = 0; k < clusters.clusters.size(); k++)
    {
        auto firstClusterId = clusters.clusters[k].first;
        for (int l = k + 1; l < clusters.clusters.size(); l++)
        {
            auto secondClusterId = clusters.clusters[l].first;
            if (!clusters.isDisjoint(firstClusterId, secondClusterId))
                continue;

            double totalXvalueBtwClusters = 0;
            std::vector<std::pair<int, int>> indicesBtwClusters;
            for (const int & i : clusters.clusters[k].second)
            {
                for (const int & j : clusters.clusters[l].second)
                {
                    int firstNodeId = std::min(i, j), secondNodeId = std::max(i, j);
                    totalXvalueBtwClusters += xSolution[firstNodeId][secondNodeId];
                    indicesBtwClusters.emplace_back(firstNodeId, secondNodeId);
                }
            }

            double fractionalPart = totalXvalueBtwClusters - (int) totalXvalueBtwClusters;
            if (fractionalPart > threshold && fractionalPart < 1 - threshold) {
                int indId = indices.size();
                // last element identifies the branching name
                indicesBtwClusters.emplace_back(firstClusterId, secondClusterId);
                indices.push_back(indicesBtwClusters);
                candidates.emplace_back(indId, fractionalPart);
            }
        }
    }

    // Sort candidates by most fractional
    sort(candidates.begin(), candidates.end(), sortRule);
    std::cout << "CB candidates list size: " << candidates.size() << std::endl;
    // To add the candidates in the list
    for (auto cand : candidates)
    {
        auto candidateIndices = indices[cand.first];
        double constMultiplier = 1.0;
        std::string branchingName = "AggClusters[" + std::to_string(candidateIndices.back().first) +
                                    "," + std::to_string(candidateIndices.back().second) + "]";
        if (candidateIndices.back().second == -1) // Degree cluster branching
        {
            branchingName = "DegCluster " + std::to_string(candidateIndices.back().first);
            constMultiplier = 0.5;
        }
        // std::cout << branchingName << std::endl;
        BcConstr bcConstr = userBranching(constrCount++);
        for (auto pair = candidateIndices.begin(); pair != std::prev(candidateIndices.end()); ++pair)
            bcConstr += constMultiplier * xVar[pair->first][pair->second];
        /// second parameter here is an unique string which characterizes the branching constraint
        /// this string is used to keep the branching history
        returnBrConstrList.emplace_back(bcConstr, branchingName);
    }

    return true;
}

bool cvrp_joao::UserBranchingFunctor::branchingOverRouteClusters(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                                                const std::vector<std::vector<double> > & xSolution,
                                                                std::list<std::pair<BcConstr, std::string> > & returnBrConstrList)
{
    double threshold = 0.1; /// should be between 0.0 and 0.5
    for (const auto & firstCluster: clusters.routeClusters)
    {
        int firstClusterId = clusters.getVertexClusterId(firstCluster.front(), true);
        double totalXvalueDeg = 0;
        std::vector<std::pair<int, int>> indicesDeg;
        for (const auto & secondCluster: clusters.routeClusters)
        {
            int secondClusterId = clusters.getVertexClusterId(secondCluster.front(), true);
            if (firstClusterId != secondClusterId)
            {
                for (const int & i: firstCluster)
                {
                    for (const int & j: secondCluster)
                    {
                        int firstNodeId = std::min(i, j), secondNodeId = std::max(i, j);
                        totalXvalueDeg += xSolution[firstNodeId][secondNodeId];
                        indicesDeg.emplace_back(firstNodeId, secondNodeId);
                    }
                }
            }
        }
        // Branching over cluster degree
        totalXvalueDeg /= 2.0; /// we should divide by 2 to have the number of paths
        double fractionalPart = totalXvalueDeg - (int) totalXvalueDeg;
        if (fractionalPart > threshold && fractionalPart < 1 - threshold)
        {
            BcConstr bcConstr = userBranching(constrCount++);
            for (auto pair : indicesDeg)
                bcConstr += 0.5 * xVar[pair.first][pair.second];
            /// second parameter here is an unique string which characterizes the branching constraint
            /// this string is used to keep the branching history
            returnBrConstrList.emplace_back(bcConstr,"DegRouteCluster " + std::to_string(firstClusterId));
        }
    }

    return true;
}

bool cvrp_joao::UserBranchingFunctor::branchingOverFractionalClusters(BcBranchingConstrArray & userBranching,
                                                                      BcVarArray &xVar,
                                                                      const std::vector<std::vector<double>> &xSolution,
                                                                      std::list<std::pair<BcConstr, std::string>> &returnBrConstrList)
{
    std::cout << "Fractional clustering evaluation" << std::endl;
    fractionalClusters.updateClustersList(data, xSolution);

    double threshold = 0.1; /// should be between 0.0 and 0.5
    /// In indices: the last pair identifies the branching
    std::vector<std::vector<std::pair<int, int>>> indices;
    /// In candidates: pair.first = pos. in indices, pair.second = fract. sol. part
    std::vector<std::pair<int, double>> candidates;

    // Branching over cluster degree
    for (auto & clust : fractionalClusters.clusters)
    {
        double totalXvalueDeg = 0;
        std::vector<std::pair<int, int>> indicesDeg;
        for (int i = 0; i <= data.nbCustomers; ++i)
        {
            if (fractionalClusters.customerInCluster(i, clust.first))
                continue;

            for (const int & j : clust.second)
            {
                int firstNodeId = std::min(i, j), secondNodeId = std::max(i, j);
                totalXvalueDeg += xSolution[firstNodeId][secondNodeId];
                indicesDeg.emplace_back(firstNodeId, secondNodeId);
            }
        }
        totalXvalueDeg /= 2.0; /// we should divide by 2 to have the number of paths
        double fractionalPart = totalXvalueDeg - (int) totalXvalueDeg;
        if (fractionalPart > threshold && fractionalPart < 1 - threshold)
        {
            int indId = indices.size();
            // last element identifies the branching name
            indicesDeg.emplace_back(clust.first, -1);
            indices.push_back(indicesDeg);
            candidates.emplace_back(indId, fractionalPart);
        }
    }

    // Branching over aggregated edges between cluster
    for (int k = 0; k < fractionalClusters.clusters.size(); k++)
    {
        auto firstClusterId = fractionalClusters.clusters[k].first;
        for (int l = k + 1; l < fractionalClusters.clusters.size(); l++)
        {
            auto secondClusterId = fractionalClusters.clusters[l].first;
            if (!fractionalClusters.isDisjoint(firstClusterId, secondClusterId))
            {
                // std::cout << "No disjoint: " << firstClusterId << " and " << secondClusterId << std::endl;
                continue;
            }

            double totalXvalueBtwClusters = 0;
            std::vector<std::pair<int, int>> indicesBtwClusters;
            for (const int & i : fractionalClusters.clusters[k].second)
            {
                for (const int & j : fractionalClusters.clusters[l].second)
                {
                    int firstNodeId = std::min(i, j), secondNodeId = std::max(i, j);
                    totalXvalueBtwClusters += xSolution[firstNodeId][secondNodeId];
                    indicesBtwClusters.emplace_back(firstNodeId, secondNodeId);
                }
            }

            double fractionalPart = totalXvalueBtwClusters - (int) totalXvalueBtwClusters;
            if (fractionalPart > threshold && fractionalPart < 1 - threshold) {
                int indId = indices.size();
                // last element identifies the branching name
                indicesBtwClusters.emplace_back(firstClusterId, secondClusterId);
                indices.push_back(indicesBtwClusters);
                candidates.emplace_back(indId, fractionalPart);
            }
        }
    }
    // Sort candidates by most fractional
    sort(candidates.begin(), candidates.end(), sortRule);
    std::cout << "CB candidates list size: " << candidates.size() << std::endl;
    // To add the candidates in the list
    for (auto cand : candidates)
    {
        auto candidateIndices = indices[cand.first];
        double constMultiplier = 1.0;
        std::string branchingName = "AggClusters[" + std::to_string(candidateIndices.back().first) +
                                    "," + std::to_string(candidateIndices.back().second) + "]";
        if (candidateIndices.back().second == -1) // Degree cluster branching
        {
            branchingName = "DegCluster " + std::to_string(candidateIndices.back().first);
            constMultiplier = 0.5;
        }
        // std::cout << branchingName << std::endl;
        BcConstr bcConstr = userBranching(constrCount++);
        for (auto pair = candidateIndices.begin(); pair != std::prev(candidateIndices.end()); ++pair)
            bcConstr += constMultiplier * xVar[pair->first][pair->second];
        /// second parameter here is an unique string which characterizes the branching constraint
        /// this string is used to keep the branching history
        returnBrConstrList.emplace_back(bcConstr, branchingName);
    }

    return true;
}

bool cvrp_joao::UserBranchingFunctor::branchingOverCutsets(BcBranchingConstrArray &userBranching, BcVarArray &xVar,
                                                           const std::vector<std::vector<double>> &xSolution,
                                                           std::list<std::pair<BcConstr, std::string>> &returnBrConstrList) {
    /// Creating required data structures
    std::vector<int> demands(data.nbCustomers + 1, 0), edgeTail(1, 0), edgeHead(1, 0);
    std::vector<double> edgeLPValue(1, 0.0);
    int nbEdges;

    /// Filling the datastructures
    /// the CVRPSEP default assume that the depot is numbered nbCustomers + 1
    /// Only information on those edges e with LP value x_e > 0 should be passed
    for (int custId = 1; custId <= data.nbCustomers; ++custId)
        demands[custId] = data.customers[custId].demand;

    for (int firstNodeId = 0; firstNodeId < data.nbCustomers; firstNodeId++) {
        for (int secondNodeId = firstNodeId + 1; secondNodeId <= data.nbCustomers; secondNodeId++) {
            if (xSolution[firstNodeId][secondNodeId] > 0) {
                auto custId = (firstNodeId == 0) ? data.nbCustomers + 1 : firstNodeId;
                edgeTail.push_back(custId);
                edgeHead.push_back(secondNodeId);
                edgeLPValue.push_back(xSolution[firstNodeId][secondNodeId]);
            }
        }
    }
    nbEdges = (int) edgeTail.size() - 1;

    Cutsets cutsets(data.nbCustomers, data.veh_capacity, demands, nbEdges,
            edgeTail, edgeHead, edgeLPValue,3.0, data.nbCustomers, true);

    /// return a unordered_map<"HashName", pair<vector<int> cutset, double cutset rhs>>
    auto myCutsets = cutsets.getCutsets();
    // Check if the name exists in the map and/or if the cutset cluster exist in CB cluster list
    std::unordered_map<std::string, std::pair<std::vector<int>, double>> updatedCutsets;
    for (auto & pair : myCutsets) {
        auto & stringId = pair.first; // cutset string id

        size_t position;
        auto it = cutsetsIdMap.find(stringId);
        if (it != cutsetsIdMap.end()) {
            // Name exists in the map, print its position
            position = it->second;
        } else {
            // Name does not exist, add it to the map with the next position
            position = cutsetsIdMap.size();
            cutsetsIdMap[stringId] = position;
        }

        // Checking if the cutset cluster exist in CB cluster list
        bool flag = false;
        if (params.enableClusterBranching())
            for (const auto & cluster : clusters.clusters)
                if (pair.second.first == cluster.second) {
                    flag = true;
                    break;
                }

        // Update the key in the new map
        if (!flag) updatedCutsets[std::to_string(position)] = std::move(pair.second);
    }
    myCutsets = std::move(updatedCutsets);
    Cutsets::printCutsets(myCutsets);

    double threshold = 0.1; /// should be between 0.0 and 0.5
    /// In indices: the last pair identifies the branching
    std::vector<std::vector<std::pair<int, int>>> indices;
    /// In candidates: pair.first = pos. in indices, pair.second = fract. sol. part
    std::vector<std::pair<int, double>> candidates;
    /// Branching over cutset degree
    for (const auto & cutset : myCutsets) {
        double totalXvalueDeg = cutset.second.second; /// Cutset RHS value
        totalXvalueDeg /= 2.0; /// we should divide by 2 to have the number of paths
        double fractionalPart = totalXvalueDeg - (int) totalXvalueDeg;
        if (fractionalPart > threshold && fractionalPart < 1 - threshold) {
            std::string branchingName = "Cutset[" + cutset.first + "]";
            BcConstr bcConstr = userBranching(constrCount++); // Creating branching constraint

            std::vector<int> set = cutset.second.first;
            for (const int & i: set) {
                for (int j = 1; j <= data.nbCustomers + 1; j++) {
                    if (std::find(set.begin(), set.end(), j) == set.end()) {
                        int firstNodeId, secondNodeId;
                        if ((i == data.nbCustomers + 1) || (j == data.nbCustomers + 1)) {
                            firstNodeId = 0;
                            secondNodeId = std::min(i, j);
                        } else {
                            firstNodeId = std::min(i, j);
                            secondNodeId = std::max(i, j);
                        }
                        bcConstr += 0.5 * xVar[firstNodeId][secondNodeId];
                    }
                }
            }
            /// second parameter here is an unique string which characterizes the branching constraint
            /// this string is used to keep the branching history
            returnBrConstrList.emplace_back(bcConstr, branchingName);
        }
    }
    return true;
}

bool cvrp_joao::UserBranchingFunctor::computeMinCut(BcVarArray & xVar, const std::vector<std::vector<double>> & xSolution)
{
    std::cout << "Min cut evaluation" << std::endl;
    ds::MinimumCut minimumCut(xSolution, data.nbCustomers);
    std::cout << minimumCut.nbMinCuts() << " min cut(s) with value = " << minimumCut.value() << std::endl;
    minimumCut.printCuts();

    return true;
}

// ##############################################################################################
//  Edge Branching functor:
//  First: The edges with fractional LP values in the interval [0.2, 0.8] are identified
//  Second: The list of branching candidates is defined with the K most costly edges in that interval
// ##############################################################################################

bool cvrp_joao::UserBranchingFunctor::branchingOverCostlyEdges(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                              const std::vector<std::vector<double> > & xSolution,
                              std::list<std::pair<BcConstr, std::string> > & returnBrConstrList,
                              const int & candListMaxSize)
{
    double minFracValue = 0.2;
    double maxFracValue = 0.8;
    /// In candidates: pair.first = pair {i,j}, pair.second = edge {i,j} cost
    std::vector<std::pair<std::pair<int, int>, double>> candidates;

    for (int firstNodeId = 0; firstNodeId < data.nbCustomers; firstNodeId++)
    {
        for (int secondNodeId = firstNodeId + 1; secondNodeId <= data.nbCustomers; secondNodeId++)
        {
            if ((xSolution[firstNodeId][secondNodeId] >= minFracValue) &&
                (xSolution[firstNodeId][secondNodeId] <= maxFracValue))
            {
                auto cost = (firstNodeId != 0) ? data.getCustToCustDistance(firstNodeId, secondNodeId)
                                               : data.getDepotToCustDistance(secondNodeId);
                candidates.emplace_back(std::make_pair(firstNodeId, secondNodeId), cost);
            }
        }
    }

    // Sort candidates by most fractional
    auto listSize = std::min(candListMaxSize, (int) candidates.size());
    std::cout << "Branching candidates list size: " << listSize << std::endl;
    std::partial_sort(candidates.begin(), candidates.begin() + listSize, candidates.end(), costSort);
    // To add the candidates in the list
    for (int i = 0; i < listSize; i++)
    {
        std::string branchingName = "EDGE[" + std::to_string(candidates[i].first.first) +
                                    "," + std::to_string(candidates[i].first.second) + "]";
        // std::cout << branchingName << std::endl;
        BcConstr bcConstr = userBranching(constrCount++);
        bcConstr += xVar[candidates[i].first.first][candidates[i].first.second];
        /// second parameter here is an unique string which characterizes the branching constraint
        /// this string is used to keep the branching history
        returnBrConstrList.emplace_back(bcConstr, branchingName);
    }

    return true;
}

// ##############################################################################################
//  Function to save fractional solution of the root node
// ##############################################################################################

void cvrp_joao::UserBranchingFunctor::saveRootFracSolution(std::set<BcVar> & xVarSet, const std::string & instanceName, const std::string & filePath)
{
    std::string  fileName = filePath;

    if (fileName.empty()) {
        fileName = instanceName;
        // Find the last path separator (either '/' or '\\')
        size_t lastPathSeparator = instanceName.find_last_of("/\\");
        // Find the last dot (.) after the last path separator
        size_t lastDot = instanceName.find_last_of(".");
        // Extract the file name (without path)
        if (lastPathSeparator != std::string::npos)
            fileName = instanceName.substr(lastPathSeparator + 1);
        // Extract the file name (without path and extension)
        if (lastDot != std::string::npos && lastDot > lastPathSeparator)
            fileName = fileName.substr(0, lastDot - lastPathSeparator - 1);

        // Get the current date and time
        std::time_t t = std::time(nullptr);
        std::tm *now = std::localtime(&t);

        // Format the date as YYYYMMDD (e.g., 20231107) and time as HHMMSS (e.g., 150215)
        std::stringstream dateTimeStream;
        dateTimeStream << (now->tm_year + 1900)
                       << std::setw(2) << std::setfill('0') << (now->tm_mon + 1)
                       << std::setw(2) << std::setfill('0') << now->tm_mday
                       << "-"
                       << std::setw(2) << std::setfill('0') << now->tm_hour
                       << std::setw(2) << std::setfill('0') << now->tm_min
                       << std::setw(2) << std::setfill('0') << now->tm_sec;
        std::string formattedDateTime = dateTimeStream.str();

        fileName = fileName + "-" + formattedDateTime + "-rootFracSolution.txt";
    }

    std::cout << "Root fractional solution file: " << fileName << std::endl;
    // Open a file for writing
    std::ofstream outFile(fileName);
    if (!outFile.is_open()) {
        std::cout << "Failed to open the file." << std::endl;
    }

    for (const auto & bcVar : xVarSet)
    {
        auto value = bcVar.solVal();
        if (value > 0)
            outFile << bcVar.id().first() << " " << bcVar.id().second() << " " << value << std::endl;
    }
    outFile.close();

    rootProcessed = true;
}


