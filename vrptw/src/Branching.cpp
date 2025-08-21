
#include "Branching.h"
#include "DisjointSets.h"

#include <utility>
#include "bcModelNetworkFlow.hpp"
#include "bcProbConfigC.hpp"

vrptw::UserBranchingFunctor::UserBranchingFunctor(const Data & data_,
                                                      const Parameters & params_,
                                                      const cluster::Clustering & clusters_) :
        BcDisjunctiveBranchingConstrSeparationFunctor(), data(data_), params(params_), clusters(clusters_), constrCount(0)
{}

vrptw::UserBranchingFunctor::~UserBranchingFunctor() = default;

bool vrptw::UserBranchingFunctor::operator() (BcFormulation master, BcSolution & primalSol,
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

    if (params.enableClusterBranching())
        bool success = branchingOverDefaultClusters(userBranching, xVar, xSolution, returnBrConstrList);

    if (params.enableCutsetsBranching())
        bool success = branchingOverCutsets(userBranching, xVar, xSolution, returnBrConstrList);

    return true;
}

bool vrptw::UserBranchingFunctor::branchingOverDefaultClusters(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
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

bool vrptw::UserBranchingFunctor::branchingOverCutsets(BcBranchingConstrArray &userBranching, BcVarArray &xVar,
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
