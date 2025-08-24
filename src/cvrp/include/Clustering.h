/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef CLUSTERING_H
#define CLUSTERING_H

#include "Singleton.h"
#include "Data.h"
#include "Parameters.h"
#include "DisjointSets.h"

#include <vector>
#include <algorithm>
#include <string>

namespace cluster
{
    class ClusteringParams
    {
    public:
        explicit ClusteringParams(const cvrp_joao::Parameters *params_) : params(params_)
        {}
        // Parameters
        bool enableClusterBranching() const { return params->enableClusterBranching(); };
        bool enableTSPClustering() const { return  params->enableTSPClustering();};
        bool enableBothClustering() const { return  params->enableBothClustering();};
        bool enableRouteClusterBranching() const { return params->enableRouteClusterBranching(); };
        std::string clustersFilePath() const { return params->clustersFilePath(); };
        std::string clusterBranchingAtRoot() const { return params->clusterBranchingAtRoot(); };
        std::string routeClusterFilePath() const { return params->routeClusterFilePath(); };
        bool enableSingletons() const { return params->enableSingletons(); };
        bool enableBigClusters() const { return params->enableBigClusters(); };

        int clusterBranchingMode() const {
            return params->clusterBranchingMode() != 0 ? params->clusterBranchingMode() : 1;
        };
        double stDevMultiplier() const {
            return params->stDevMultiplier() != -1 ? params->stDevMultiplier() : 1.5;
        };
        double stDevMultiplierTSP() const {
            return params->stDevMultiplierTSP() != -1 ? params->stDevMultiplierTSP() : 1.5;
        };
        double bigClustersSizeThreshold() const {
            return params->bigClustersSizeThreshold() != -1 ? params->bigClustersSizeThreshold() : 0.4;
        };
        double decreasingStepStDev() const {
            return params->bigClustersSizeThreshold() != -1 ? params->bigClustersSizeThreshold() : 0.25;
        };

    protected:
        const cvrp_joao::Parameters *params;
    };

    class MSTClustering : public ds::Utils
    {
    public:
        std::vector<std::vector<int>> clusters;

        MSTClustering(const cvrp_joao::Data *data, const ClusteringParams & clusteringParams, bool verbose = true);

    private:
        ds::Graph graph;
        std::vector<Edge> mstTree;
        std::vector<int> vertexDegree;
        double mstWeight;
        double avgMSTWeight;
        double stDevMSTWeight;

        void kruskalMST(bool verbose = true);
        void splitMST(const ClusteringParams & clusteringParams);
        void multiSplitMST(const ClusteringParams & clusteringParams);
    };

    class TSPClustering : public ds::Utils
    {
    public:
        std::vector<std::vector<int>> clusters;

        TSPClustering(const cvrp_joao::Data *data, const ClusteringParams & clusteringParams, bool verbose = true);

    private:
        const cvrp_joao::Data *data;
        std::vector<int> tspTour;
        std::vector<int> vertexDegree;
        double tspWeight;
        double avgTSPWeight;
        double stDevTSPWeight;
        double minWeight;
        double maxWeight;

        void lkhTSP(bool verbose = true);
        void splitTSP(const ClusteringParams & clusteringParams);
    };

    class Clustering : private ds::Utils, public cvrp_joao::Singleton<cluster::Clustering>
    {
        friend class cvrp_joao::Singleton<cluster::Clustering>;
    public:
        Clustering();
        virtual ~Clustering() {}

        void loadClustering(const cvrp_joao::Data & data_, const cvrp_joao::Parameters & parameters_);

        std::vector<std::pair<int,std::vector<int>>> clusters;
        std::vector<std::pair<int,std::vector<int>>> clustersAtRoot;
        std::vector<std::vector<int>> routeClusters;
        std::vector<int> vertexCluster;
        std::vector<int> vertexRouteCluster;

        static void printClusters(const std::vector<std::pair<int,std::vector<int>>> & clusters_);
        int nbClusters() const { return (int) clusters.size(); }
        int nbMSTClusters() const { return nbMSTclusters; }
        int nbTSPClusters() const { return nbTSPclusters; }
        int nbRoutes() const { return (int) routeClusters.size(); }
        int getVertexClusterId(int i, bool fromRouteCluster = false) const;
        bool isDisjoint(int i, int j) const { return disjoint[i][j]; };
        bool customerInCluster(int custId, int clustId) const { return clusterCustomers[clustId][custId]; };

    private:
        ClusteringParams clusteringParams;
        const cvrp_joao::Data *data;
        std::vector<std::vector<bool>> disjoint;
        std::vector<std::vector<bool>> clusterCustomers;
        int nbMSTclusters;
        int nbTSPclusters;

        void loadClusters();
        void loadClustersAtRoot();
        void getVerticesClusterId(int n, bool fromRouteCluster = false);
        bool loadClusterData(const std::string & clustersFile, std::vector<std::pair<int,std::vector<int>>> & clusters_);
        bool loadClusterFile(std::ifstream & ifs, std::vector<std::pair<int,std::vector<int>>> & clusters_);
        void loadClustersFromRoutesFile(const std::string & clustersRouteFile);
    };
}

#endif