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
        explicit ClusteringParams(const vrptw::Parameters *params_) : params(params_)
        {}
        // Parameters
        bool enableClusterBranching() const { return params->enableClusterBranching(); };
        bool enableRouteClusterBranching() const { return params->enableRouteClusterBranching(); };
        std::string clustersFilePath() const { return params->clustersFilePath(); };
        std::string routeClusterFilePath() const { return params->routeClusterFilePath(); };
        bool enableSingletons() const { return params->enableSingletons(); };
        bool enableBigClusters() const { return params->enableBigClusters(); };

        int clusterBranchingMode() const {
            return params->clusterBranchingMode() != 0 ? params->clusterBranchingMode() : 1;
        };
        double stDevMultiplier() const {
            return params->stDevMultiplier() != -1 ? params->stDevMultiplier() : 1.5;
        };
        double bigClustersSizeThreshold() const {
            return params->bigClustersSizeThreshold() != -1 ? params->bigClustersSizeThreshold() : 0.4;
        };
        double decreasingStepStDev() const {
            return params->bigClustersSizeThreshold() != -1 ? params->bigClustersSizeThreshold() : 0.25;
        };

    protected:
        const vrptw::Parameters *params;
    };

class MSTClustering : public ds::Utils
    {
    public:
        std::vector<std::vector<int>> clusters;

        MSTClustering(const vrptw::Data *data, const ClusteringParams & clusteringParams, bool verbose = true);

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

class Clustering : private ds::Utils, public vrptw::Singleton<cluster::Clustering>
    {
        friend class vrptw::Singleton<cluster::Clustering>;
    public:
        Clustering();
        virtual ~Clustering() {}

        void loadClustering(const vrptw::Data & data_, const vrptw::Parameters & parameters_);

        std::vector<std::pair<int,std::vector<int>>> clusters;
        std::vector<std::vector<int>> routeClusters;
        std::vector<int> vertexCluster;
        std::vector<int> vertexRouteCluster;

        static void printClusters(const std::vector<std::pair<int,std::vector<int>>> & clusters_);
        int nbClusters() const { return (int) clusters.size(); }
        int nbRoutes() const { return (int) routeClusters.size(); }
        int getVertexClusterId(int i, bool fromRouteCluster = false) const;
        bool isDisjoint(int i, int j) const { return disjoint[i][j]; };
        bool customerInCluster(int custId, int clustId) const { return clusterCustomers[clustId][custId]; };

    private:
        ClusteringParams clusteringParams;
        const vrptw::Data *data;
        std::vector<std::vector<bool>> disjoint;
        std::vector<std::vector<bool>> clusterCustomers;

        void loadClusters();
        void getVerticesClusterId(int n, bool fromRouteCluster = false);
        bool loadClusterData(const std::string & clustersFile);
        bool loadClusterFile(std::ifstream & ifs);
        void loadClustersFromRoutesFile(const std::string & clustersRouteFile);
    };
}

#endif