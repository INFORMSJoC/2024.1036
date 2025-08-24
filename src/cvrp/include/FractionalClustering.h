/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef FRACTIONALCLUSTERING_H
#define FRACTIONALCLUSTERING_H

#include <vector>
#include "Data.h"
#include "Singleton.h"

namespace fracclu
{
    class FractionalClustering: public cvrp_joao::Singleton<fracclu::FractionalClustering>
    {
        friend class cvrp_joao::Singleton<fracclu::FractionalClustering>;

    public:
        std::vector<std::pair<int,std::vector<int>>> clusters;

        void loadFractionalClustering(double threshold_, int n);
        void updateClustersList(const cvrp_joao::Data & data, const std::vector<std::vector<double> > & xSolution);
        void insertCluster(std::vector<int> clust) { clusters.emplace_back(clusters.size() + 1, clust); };
        static void printClusters(const std::vector<std::pair<int,std::vector<int>>> & clusters_);
        void setThresholdValue(double value) { threshold = value; };
        bool isDisjoint(int i, int j) const { return disjoint[i][j]; };
        bool customerInCluster(int custId, int clustId){ return customerClusters[clustId][custId]; };

    private:
        double threshold;
        // The depot is always a single cluster
        std::vector<int> depotCluster;
        std::vector<std::vector<int>> newClusters;
        std::vector<std::vector<bool>> customerClusters;
        std::vector<std::vector<bool>> disjoint;

        FractionalClustering() : clusters(), threshold(0.5), depotCluster(), newClusters(), customerClusters(), disjoint() {}

        void getNewClusters(const cvrp_joao::Data & data, const std::vector<std::vector<double> > & xSolution);
        void removeDuplicates();
        void insertNewClusters(int n);
    };
}

#endif