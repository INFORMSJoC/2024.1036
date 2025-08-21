
#include "Clustering.h"
//#include "DisjointSets.h"
#include "runLKH.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

cluster::MSTClustering::MSTClustering(const cvrp_joao::Data *data, const ClusteringParams & clusteringParams, bool verbose) :
        clusters(), graph(data), mstTree(), vertexDegree(), mstWeight(0), avgMSTWeight(0), stDevMSTWeight(0)
{
    // MST clustering
    kruskalMST(verbose);
    splitMST(clusteringParams);
    // multiSplitMST(clusteringParams);
}

void cluster::MSTClustering::kruskalMST(bool verbose)
{
    ds::DisjointSets sets(graph.n);

    std::cout << "Single linkage clustering (Applying Kruskal MST alg.)!" << std::endl;
    if (verbose)
        std::cout << "MST edges: ";

    // Pair <nearest vertex, vertex degree>
    vertexDegree = std::vector<int> (graph.n + 1, 0);
    std::sort(graph.edges.begin(), graph.edges.end(), sortByWeight);
    for (const auto & edge: graph.edges)
    {
        int firstVertex = edge.first.first;
        int secondVertex = edge.first.second;

        if (sets.findParent(firstVertex) != sets.findParent(secondVertex))
        {
            sets.unionRank(firstVertex, secondVertex);
            mstWeight += edge.second;
            mstTree.push_back(edge);

            if (verbose)
                std::cout << "(" << firstVertex << "," << secondVertex << ") ";

            vertexDegree[firstVertex] += 1;
            vertexDegree[secondVertex] += 1;
        }
    }
    if (verbose)
        std::cout << std::endl;

    // MST weight average
    avgMSTWeight = mstWeight / (int) mstTree.size();
    // MST weight standard deviation
    double sqSum = 0.0;
    for (const auto & edge: mstTree)
    {
        sqSum += (edge.second - avgMSTWeight) * (edge.second - avgMSTWeight);
    }
    stDevMSTWeight = std::sqrt( sqSum / (int) mstTree.size());

    std::cout << "MST weight: " << mstWeight << ", with size: " << mstTree.size() << "\n"
              << "Min weight: " << mstTree.front().second << ", Max weight: " << mstTree.back().second
              << ", Avg weight: " << avgMSTWeight << ", stDev: " << stDevMSTWeight << std::endl;
    std::cout << "Clustering algorithm: Single linkage clustering (Applying Kruskal MST alg.)" << std::endl;
}

void cluster::MSTClustering::splitMST(const ClusteringParams & clusteringParams)
{
    int maxClusterSize = std::floor(clusteringParams.bigClustersSizeThreshold()  * graph.n);
    double stDevMultiplierAux = clusteringParams.stDevMultiplier();
    int edgeCutOff = std::ceil(avgMSTWeight + stDevMultiplierAux * stDevMSTWeight);
    while (edgeCutOff > mstTree.back().second) // mstTree.back() is the most costly edge in the MST
    {
        stDevMultiplierAux = stDevMultiplierAux - clusteringParams.decreasingStepStDev();
        edgeCutOff = std::ceil(avgMSTWeight + stDevMultiplierAux * stDevMSTWeight);
    }

    std::cout << "User stDev multiplier: " << clusteringParams.stDevMultiplier()
              << ", user cluster size threshold: " << clusteringParams.bigClustersSizeThreshold() << std::endl;
    std::cout << "stDev multiplier applied: " << stDevMultiplierAux
              << ", dynamic decreasing step of stDev: " << clusteringParams.decreasingStepStDev() << std::endl;
    std::cout << "MST edge cutoff: " << edgeCutOff << ", max cluster size: " << maxClusterSize << std::endl;
    std::cout << "Clustering algorithm parametrization: stDevMult("  << stDevMultiplierAux
              << "), ClusterSizeThreshold(" << clusteringParams.bigClustersSizeThreshold() << ") " << std::endl;

    ds::DisjointSets dsSets(graph.n);
    std::map<std::pair<int, int>, bool> edgesHasParent;

    // Splitting by cutoff
    for (const auto & edge: mstTree)
    {
        int firstVertex = edge.first.first;
        int secondVertex = edge.first.second;
        if (edge.second < edgeCutOff)
        {
            dsSets.unionRank(firstVertex, secondVertex);
            edgesHasParent[std::make_pair(firstVertex, secondVertex)] = true;
        }
        else
        {
            edgesHasParent[std::make_pair(firstVertex, secondVertex)] = false;
            vertexDegree[firstVertex] -= 1;
            vertexDegree[secondVertex] -= 1;
        }
    }
    // Identifying dsSets
    std::vector<std::vector<Edge>> parentsTree(graph.n + 1);
    for (const auto & edge: mstTree)
    {
        int firstVertex = edge.first.first;
        int secondVertex = edge.first.second;
        if (edgesHasParent[std::make_pair(firstVertex, secondVertex)])
        {
            parentsTree[dsSets.findParent(firstVertex)].push_back(edge);
        }
    }
    // dsSets.getDisjointSets(n, true, 2);

    // Splitting cluster by maximum size
    if (!clusteringParams.enableBigClusters()) {
        std::cout << "Splits by cluster size... " << std::endl;
        for (const auto &tree: parentsTree) {
            double sz = (int) tree.size() > 0 ? (int) tree.size() + 1 : 0;
            if (sz > maxClusterSize) {
                int nbSplit = std::ceil(sz / maxClusterSize);
                std::cout << "|--> Size: " << sz << ", nbSplits: " << nbSplit << std::endl;
                auto it = tree.rbegin();
                while (nbSplit > 0) {
                    int firstVertex = (it)->first.first;
                    int secondVertex = (it)->first.second;
                    if (vertexDegree[firstVertex] > 1 // To avoid singleton clusters
                        && vertexDegree[secondVertex] > 1) {
                        edgesHasParent[std::make_pair(firstVertex, secondVertex)] = false;
                        vertexDegree[firstVertex] -= 1;
                        vertexDegree[secondVertex] -= 1;
                        nbSplit--;
                    }
                    it++;
                }
            }
        }
    }

    // Removing singletons
    if (!clusteringParams.enableSingletons())
    {
        std::cout << "Connecting singletons... ";
        // for (auto edge = mstTree.rbegin(); edge != mstTree.rend(); edge++)
        for (const auto &edge: mstTree) {
            if (edge.second < edgeCutOff) continue;

            int firstVertex = edge.first.first;
            int secondVertex = edge.first.second;
            if (vertexDegree[firstVertex] == 0 || vertexDegree[secondVertex] == 0) {
                edgesHasParent[std::make_pair(firstVertex, secondVertex)] = true;
                vertexDegree[firstVertex] += 1;
                vertexDegree[secondVertex] += 1;

                if (vertexDegree[firstVertex] == 1)
                    std::cout << firstVertex << " ";
                if (vertexDegree[secondVertex] == 1)
                    std::cout << secondVertex << " ";
            }
        }
        std::cout << std::endl;
    }

    // Identifying clusters
    ds::DisjointSets dsSetsAux(graph.n);
    for (const auto & edge: edgesHasParent)
        if (edge.second)
            dsSetsAux.unionRank(edge.first.first, edge.first.second);
    clusters = dsSetsAux.getDisjointSets(graph.n);
}

void cluster::MSTClustering::multiSplitMST(const ClusteringParams & clusteringParams)
{
    int maxClusterSize = std::floor(clusteringParams.bigClustersSizeThreshold()  * graph.n);
    double stDevMultiplierAux = clusteringParams.stDevMultiplier();
    int edgeCutOff = std::ceil(avgMSTWeight + stDevMultiplierAux * stDevMSTWeight);
    while (edgeCutOff > mstTree.back().second) // mstTree.back() is the most costly edge in the MST
    {
        stDevMultiplierAux = stDevMultiplierAux - clusteringParams.decreasingStepStDev();
        edgeCutOff = std::ceil(avgMSTWeight + stDevMultiplierAux * stDevMSTWeight);
    }

    std::cout << "User stDev multiplier: " << clusteringParams.stDevMultiplier()
              << ", user cluster size threshold: " << clusteringParams.bigClustersSizeThreshold() << std::endl;
    std::cout << "stDev multiplier applied: " << stDevMultiplierAux
              << ", dynamic decreasing step of stDev: " << clusteringParams.decreasingStepStDev() << std::endl;
    std::cout << "MST edge cutoff: " << edgeCutOff << ", max cluster size: " << maxClusterSize << std::endl;
    std::cout << "Clustering algorithm parametrization: stDevMult("  << stDevMultiplierAux
              << "), ClusterSizeThreshold(" << clusteringParams.bigClustersSizeThreshold() << ") " << std::endl;

    // Splitting by cutoff
    int pos = mstTree.size() - 1;
    while (true) // Clustering for each edge removed
    {
        ds::DisjointSets dsSets(graph.n);
        for (int i = 0; i < pos; i++)
        {
            int firstVertex = mstTree[i].first.first;
            int secondVertex = mstTree[i].first.second;
            dsSets.unionRank(firstVertex, secondVertex);
        }

        auto clustersAux = dsSets.getDisjointSets(graph.n);
        for (auto & clu1: clustersAux)
        {
            bool exists = false;
            for (auto & clu2: clusters)
            {
                if (ds::Utils::compare(clu1, clu2))
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
                clusters.push_back(clu1);
        }

        pos--;
        if (mstTree[pos].second < edgeCutOff)
            break;
    }
}


cluster::TSPClustering::TSPClustering(const cvrp_joao::Data *data, const ClusteringParams & clusteringParams, bool verbose) :
        clusters(), data(data), vertexDegree(), tspWeight(0), avgTSPWeight(0), stDevTSPWeight(0), minWeight(1e10), maxWeight(0)
{
    // MST clustering
    lkhTSP(verbose);
    splitTSP(clusteringParams);
    // multiSplitMST(clusteringParams);
}

void cluster::TSPClustering::lkhTSP(bool verbose) {

    // Create a CityArray to return to LKH function in C code
    auto* customersArray = new struct CustomersArray;
    auto dimension = data->nbCustomers + 1;
    customersArray->length = dimension;
    customersArray->customer = new Customer[dimension + 1];
    customersArray->dimension = dimension;
    customersArray->problemType = 0; // TSP
    customersArray->weightType = 1; // 1 = EUC_2D
    customersArray->name = "tmp";

    // Copy data to the struct
    // Depot has id 1 in LKH code
    customersArray->customer[1].id = 1;
    customersArray->customer[1].x = data->depot_x;
    customersArray->customer[1].y = data->depot_y;
    for (size_t i = 1; i < dimension; ++i) {
        auto idx = i + 1;
        customersArray->customer[idx].id = idx;
        customersArray->customer[idx].x = data->customers[i].x;
        customersArray->customer[idx].y = data->customers[i].y;
//        std::cout << "ID: " << customersArray->customer[idx].id <<
//         		" X: " << customersArray->customer[idx].x <<
//         		" Y: " << customersArray->customer[idx].y << std::endl;
    }
//    std::cout << "Dimension: " << dimension << std::endl;

    // Calling LKH code
    int* tour = nullptr;
    double tourCost;
    auto success = runLKH(0, (char **) "", customersArray, &tour, &tourCost, &dimension);
    auto auxTour =  std::vector<int> (tour, tour + dimension);

    if ((auxTour.size() != dimension) || (auxTour.size() - 1 != data->nbCustomers))
        std::cerr << "Number of customers in the TSP tour (" << auxTour.size() - 1
                  << ") not equal to the number of customers in the problem (" << data->nbCustomers << ")" << std::endl;

    // Subtract 1 from each element in the vector
    for (auto &element : auxTour) element -= 1;
    // Organizing vector to start from 0 (depot) as element tspTour[0]
    // Find the iterator to the first occurrence of 0
    auto it = std::find(auxTour.begin(), auxTour.end(), 0);
    if (it != auxTour.end()) {
        // Create a new vector with elements after 1 followed by elements before 1
        tspTour = std::vector<int> (it, auxTour.end());
        tspTour.insert(tspTour.end(), auxTour.begin(), it);
    } else {
        std::cerr << "Element 0 (depot) not found in the TSP tour." << std::endl;
    }

    tspWeight = tourCost;
    avgTSPWeight = tspWeight / (int) tspTour.size();

    std::cout << "Single linkage clustering (Applying LKH TSP alg.)!" << std::endl;
    if (verbose)
        std::cout << "TSP tour: 0 ";

    double sqSum = 0.0; // TSP weight standard deviation
    auto costAux =  data->getDepotToCustDistance(tspTour[1]); // cost of the depot to first city
    auto costCheck = costAux;
    sqSum += (costAux - avgTSPWeight) * (costAux - avgTSPWeight);
    minWeight = (costAux < minWeight) ? costAux : minWeight;
    maxWeight = (costAux > maxWeight) ? costAux : maxWeight;
    for (size_t i = 1; i < dimension - 1; ++i) {
        costAux = data->getCustToCustDistance(tspTour[i], tspTour[i+1]);
        costCheck += costAux;
        sqSum += (costAux - avgTSPWeight) * (costAux - avgTSPWeight);
        minWeight = (costAux < minWeight) ? costAux : minWeight;
        maxWeight = (costAux > maxWeight) ? costAux : maxWeight;

        if (verbose)
            std::cout << tspTour[i] << " ";
    }
    costAux = data->getDepotToCustDistance(tspTour.back()); // cost of the depot to last city
    costCheck += costAux;
    sqSum += (costAux - avgTSPWeight) * (costAux - avgTSPWeight);
    minWeight = (costAux < minWeight) ? costAux : minWeight;
    maxWeight = (costAux > maxWeight) ? costAux : maxWeight;

    if (verbose)
        std::cout << tspTour.back() << std::endl;

    std::cout << "LKH cost: " << tourCost << ", App cost: " << costCheck << std::endl;

    stDevTSPWeight = std::sqrt( sqSum / (int) tspTour.size());

    std::cout << "TSP weight: " << tspWeight << ", with size: " << tspTour.size() << "\n"
              << "Min weight: " << minWeight << ", Max weight: " << maxWeight
              << ", Avg weight: " << avgTSPWeight << ", stDev: " << stDevTSPWeight << std::endl;
    std::cout << "Clustering algorithm: Single linkage clustering (Applying LKH TSP alg.)" << std::endl;
}

void cluster::TSPClustering::splitTSP(const ClusteringParams & clusteringParams)
{
    int maxClusterSize = std::floor(clusteringParams.bigClustersSizeThreshold()  * data->nbCustomers);
    double stDevMultiplierAux = clusteringParams.stDevMultiplierTSP();
    int edgeCutOff = std::ceil(avgTSPWeight + stDevMultiplierAux * stDevTSPWeight);
    while (edgeCutOff > maxWeight) // mstTree.back() is the most costly edge in the MST
    {
        stDevMultiplierAux = stDevMultiplierAux - clusteringParams.decreasingStepStDev();
        edgeCutOff = std::ceil(avgTSPWeight + stDevMultiplierAux * stDevTSPWeight);
    }

    std::cout << "User stDev multiplier: " << clusteringParams.stDevMultiplier()
              << ", user cluster size threshold: " << clusteringParams.bigClustersSizeThreshold() << std::endl;
    std::cout << "stDev multiplier applied: " << stDevMultiplierAux
              << ", dynamic decreasing step of stDev: " << clusteringParams.decreasingStepStDev() << std::endl;
    std::cout << "MST edge cutoff: " << edgeCutOff << ", max cluster size: " << maxClusterSize << std::endl;
    std::cout << "Clustering algorithm parametrization: stDevMult("  << stDevMultiplierAux
              << "), ClusterSizeThreshold(" << clusteringParams.bigClustersSizeThreshold() << ") " << std::endl;

    // Identifying clusters
    ds::DisjointSets dsSets(data->nbCustomers);
    for (size_t i = 1; i < tspTour.size() - 1; ++i) { // Split skips depot at tapTour[0]
        auto costAux = data->getCustToCustDistance(tspTour[i], tspTour[i+1]);
        if (costAux < edgeCutOff)
            dsSets.unionRank(tspTour[i], tspTour[i+1]);
    }
    clusters = dsSets.getDisjointSets(data->nbCustomers);
}

cluster::Clustering::Clustering() :
        clusters(), clustersAtRoot(), routeClusters(), vertexCluster(), vertexRouteCluster(), clusterCustomers(), disjoint(),
        data(nullptr), clusteringParams(nullptr), nbMSTclusters(0), nbTSPclusters(0)
{}

void cluster::Clustering::loadClustering(const cvrp_joao::Data & data_, const cvrp_joao::Parameters & parameters_)
{

    data = &data_;
    clusteringParams = ClusteringParams(&parameters_);
    // Cluster branching to be fixed at root
    if (!clusteringParams.clusterBranchingAtRoot().empty())
        loadClustersAtRoot();
    // Default clustering procedure
    if (clusteringParams.enableClusterBranching())
        loadClusters();
    // Load clusters from route solution file (CVRP solution format)
    if (clusteringParams.enableRouteClusterBranching())
        loadClustersFromRoutesFile(clusteringParams.routeClusterFilePath());
}

void cluster::Clustering::loadClusters()
{
    std::cout << "Cluster Branching ENABLED" << std::endl;

    if (clusteringParams.clusterBranchingMode() == 1) {
        std::vector<std::vector<int>> clustersAux;
        if (clusteringParams.enableBothClustering()) {
            MSTClustering MSTclusters(data, clusteringParams);
            TSPClustering TSPclusters(data, clusteringParams);

            nbMSTclusters = MSTclusters.clusters.size();
            nbTSPclusters = TSPclusters.clusters.size();

            clustersAux = MSTclusters.clusters;
            // clustersAux.insert(clustersAux.end(), TSPclusters.clusters.begin(), TSPclusters.clusters.end());
            auto nbEqClusters = 0;
            for (const auto clu1 : TSPclusters.clusters) {
                bool exist = false;
                for (const auto clu2 : clustersAux) {
                    if (clu1.size() != clu2.size())
                        continue;
                    bool equals = true;
                    for (int i = 0; i < clu1.size(); ++i) {
                        if (clu1[i] != clu2[i]) {
                            equals = false;
                            break;
                        }
                    }
                    if (equals) {
                        exist = true;
                        break;
                    }
                }
                if (!exist)
                    clustersAux.push_back(clu1);
                else
                    nbEqClusters++;
            }

            std::cout << "Number of MST clusters: " << nbMSTclusters << " | C[2..." << nbMSTclusters << "+1]" <<  std::endl;
            std::cout << "Number of TSP clusters: " << nbTSPclusters << " | C[" << nbMSTclusters
                      << "+1..." << nbMSTclusters << "+" << nbTSPclusters << "]" <<  std::endl;
            std::cout << "Number of equivalent clusters find by both methods: " << nbEqClusters << std::endl;

        } else if (clusteringParams.enableTSPClustering()) {
            TSPClustering TSPclusters(data, clusteringParams);
            nbTSPclusters = TSPclusters.clusters.size();
            clustersAux = TSPclusters.clusters;
        } else {
            MSTClustering MSTclusters(data, clusteringParams);
            nbMSTclusters = MSTclusters.clusters.size();
            clustersAux = MSTclusters.clusters;
        }
        auto cluId = 1;
        clusters.emplace_back(cluId++, std::vector<int>(1, 0)); // Depot clusters predefined as cluster id 1
        for (auto & clu : clustersAux)
            clusters.emplace_back(cluId++, clu);
    }
    else if (clusteringParams.clusterBranchingMode() == 2)
    {
        auto success = loadClusterData(clusteringParams.clustersFilePath(), clusters);
        if (!success)
        {
            std::cerr << "Error : something wrong in clusters file!" << std::endl;
            exit(1);
        }
    }
    else
    {
        std::cerr << "Error : cluster branching option not recognized!" << std::endl;
        exit(1);
    }

    // Checking for intersected clusters and identifying customer clusters
    disjoint = std::vector<std::vector<bool>>(nbClusters() + 1, std::vector<bool> (nbClusters() + 1, true));
    clusterCustomers = std::vector<std::vector<bool>> (nbClusters() + 1, std::vector<bool> (data->nbCustomers + 1, false));
    for (int i = 0; i < clusters.size(); i++)
    {
        for (int j = i + 1; j < clusters.size(); j++)
            if (ds::Utils::intersection(clusters[i].second, clusters[j].second))
                disjoint[clusters[i].first][clusters[j].first] = disjoint[clusters[j].first][clusters[i].first] = false;

        for (auto & cust : clusters[i].second)
            clusterCustomers[clusters[i].first][cust] = true;
    }
    // Showing results
    printClusters(clusters);
    getVerticesClusterId(data->nbCustomers + 1);
}

void cluster::Clustering::loadClustersAtRoot() {
    std::cout << "Clusters of the root: " << std::endl;
    auto success = loadClusterData(clusteringParams.clusterBranchingAtRoot(), clustersAtRoot);
    if (!success)
    {
        std::cerr << "Error : something wrong in clusters at root file!" << std::endl;
        exit(1);
    }
    printClusters(clustersAtRoot);
}

void cluster::Clustering::printClusters(const std::vector<std::pair<int,std::vector<int>>> & clusters_) {

    if (clusters_.empty())
        std::cerr << "The clusters are not defined yet!" << std::endl;

    std::cout << "+-------- Printing clusters ---------+" << std::endl;
    for (const auto & clu : clusters_) {
        std::cout << "C" << clu.first;
        for (const auto & vertex : clu.second)
        {
            std::cout << " " << vertex;
        }
        std::cout << std::endl;
    }
    std::cout << "+----------- End of print -----------+" << std::endl;
}

void cluster::Clustering::getVerticesClusterId(int n, bool fromRouteCluster) {
    if (!fromRouteCluster)
    {
        if (nbClusters() == 0)
        {
            std::cerr << "Error: no clusters defined!" << std::endl;
            exit(1);
        }

        vertexCluster = std::vector<int> (n+1, -1);
        for (int clusterIdx = 1; clusterIdx <= nbClusters(); ++clusterIdx)
        {
            for (const auto vertexId : clusters[clusterIdx - 1].second) {
                vertexCluster[vertexId] = clusterIdx;
            }
        }
    }
    else
    {
        if (nbRoutes() == 0)
        {
            std::cerr << "Error: no route clusters defined!" << std::endl;
            exit(1);
        }

        vertexRouteCluster = std::vector<int> (n+1, -1);
        for (int clusterIdx = 1; clusterIdx <= nbRoutes(); ++clusterIdx)
        {
            for (const auto vertexId : routeClusters[clusterIdx - 1]) {
                vertexRouteCluster[vertexId] = clusterIdx;
            }
        }
    }
}

int cluster::Clustering::getVertexClusterId(int i, bool fromRouteCluster) const
{
    return !fromRouteCluster ? vertexCluster[i] : vertexRouteCluster[i];
}

bool cluster::Clustering::loadClusterData(const std::string & clustersFile, std::vector<std::pair<int,std::vector<int>>> & clusters_)
{
    std::ifstream ifs(clustersFile.c_str(), std::ios::in);
    if (!ifs)
    {
        std::cerr << "Clusters file reader Error: cannot open file " << clustersFile << std::endl;
        return false;
    }

    if (ifs.eof())
    {
        std::cout << "Clusters file reader Error: empty input file " << clustersFile << std::endl;
        ifs.close();
        return false;
    }

    bool success = false;
    success = loadClusterFile(ifs, clusters_);
    ifs.close();

    return success;
}

bool cluster::Clustering::loadClusterFile(std::ifstream & ifs, std::vector<std::pair<int,std::vector<int>>> & clusters_)
{
    int dimension = 0;
    std::string clustering_alg, clustering_params;
    std::string line;
    while (std::getline(ifs, line))
    {
        if (line.find("DIMENSION") != std::string::npos)
        {
            if ((sscanf(line.c_str(), "DIMENSION : %d", &dimension) != 1)
                && (sscanf(line.c_str(), "DIMENSION: %d", &dimension) != 1)) {
                std::cerr << "Error : cannot read DIMENSION" << std::endl;
                exit(1);
            }
        }
        if (line.find("CLUSTERING_ALG") != std::string::npos)
        {
            int n;
            if ((sscanf(line.c_str(), "CLUSTERING_ALG : %n", &n) == -1)
                && (sscanf(line.c_str(), "CLUSTERING_ALG: %n", &n) == -1)) {
                std::cerr << "Error : cannot read CLUSTERING_ALG" << std::endl;
                exit(1);
            }
            clustering_alg = line.c_str() + n;
        }
        if (line.find("CLUSTERING_PARAMS") != std::string::npos)
        {
            int n;
            if ((sscanf(line.c_str(), "CLUSTERING_PARAMS : %n", &n) == -1)
                && (sscanf(line.c_str(), "CLUSTERING_PARAMS: %n", &n) == -1)) {
                std::cerr << "Error : cannot read CLUSTERING_PARAMS" << std::endl;
                exit(1);
            }
            clustering_params = line.c_str() + n;
            break;
        }
    }

    std::cout << "Clusters defined by file!" << std::endl;
    std::cout << "Clustering algorithm: " << clustering_alg << std::endl;
    std::cout << "Clustering algorithm parametrization: " << clustering_params << std::endl;

    if (dimension == 0)
    {
        std::cerr << "Clusters file reader Error : no DIMENSION specified" << std::endl;
        exit(1);
    }

    if (ifs.eof() || ifs.bad())
        return false;

    int dim = 0;
    auto cluId = 1;
    while (std::getline(ifs, line))
    {
        if (line.find("CLUSTERS") != std::string::npos)
        {
            while (std::getline(ifs, line)) {
                if (strncmp(line.c_str(), "EOF", strlen("EOF")) == 0)
                    break;

                std::vector<int> clusterNodes;
                std::string s;
                std::istringstream ss(line);
                while (std::getline(ss, s, ' '))
                {
                    int n;
                    if (sscanf(s.c_str(), "%d", &n) == 1)
                        clusterNodes.push_back(n);
                }
                clusterNodes.erase(clusterNodes.begin());
                dim += (int) clusterNodes.size();
                clusters_.emplace_back(cluId++, clusterNodes);
            }
            break;
        }
    }

    if ((dim != dimension) || (dim != data->nbCustomers + 1))
    {
        std::cerr << "Clusters file reader Error : the total number of nodes in clusters differs from the DIMENSION specified!" << std::endl;
        exit(1);
    }

    return true;
}

void cluster::Clustering::loadClustersFromRoutesFile(const std::string & clustersRouteFile)
{
    std::ifstream ifs(clustersRouteFile.c_str(), std::ios::in);
    if (!ifs)
    {
        std::cerr << "Route clusters file reader Error: cannot open file " << clustersRouteFile << std::endl;
    }

    if (ifs.eof())
    {
        std::cerr << "Route clusters file reader Error: empty input file " << clustersRouteFile << std::endl;
    }

    std::string line;
    while (std::getline(ifs, line))
    {
        if (line.find("CLUSTERS") != std::string::npos)
            break;

        if (line.find("Route") != std::string::npos) {
            std::vector<int> routeNodes;
            std::string s;
            std::istringstream ss(line);
            int count = 1;
            while (std::getline(ss, s, ' ')) {
                if (count < 3)
                {
                    count++;
                    continue;
                }
                int n;
                if (sscanf(s.c_str(), "%d", &n) == 1)
                    routeNodes.push_back(n);
            }
            routeClusters.push_back(routeNodes);
        }
    }
    // printClusters(routeClusters);
    getVerticesClusterId(data->nbCustomers + 1, true);
}