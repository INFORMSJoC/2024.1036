/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */


#include "FractionalClustering.h"
#include "DisjointSets.h"

#include <algorithm>

void fracclu::FractionalClustering::loadFractionalClustering(double threshold_, int n)
{
    setThresholdValue(threshold_);
    // Initializing depot '0'
    depotCluster.push_back(0);
    clusters.emplace_back(1, depotCluster);
    disjoint.emplace_back(std::vector<bool> (clusters.size() + 1, true));
    disjoint.emplace_back(std::vector<bool> (clusters.size() + 1, true));
    std::vector<bool> aux(n, false);
    customerClusters.push_back(aux);
    aux[0] = true;
    customerClusters.push_back(aux);
}

void fracclu::FractionalClustering::updateClustersList(const cvrp_joao::Data & data,
                                                       const std::vector<std::vector<double>> & xSolution)
{
    getNewClusters(data, xSolution);
    removeDuplicates();
    insertNewClusters(data.nbCustomers + 1);
}

void fracclu::FractionalClustering::getNewClusters(const cvrp_joao::Data & data, const std::vector<std::vector<double>> & xSolution)
{
    // Clear vector
    newClusters.clear();
    // Creating sets based on the fractional solution of the edges
    ds::DisjointSets sets(data.nbCustomers);
    for (int firstCustId = 1; firstCustId <= data.nbCustomers; ++firstCustId )
        for (int secondCustId = firstCustId + 1; secondCustId <= data.nbCustomers; ++secondCustId)
            if (xSolution[firstCustId][secondCustId] >= threshold)
                sets.unionRank(firstCustId, secondCustId);

    newClusters = sets.getDisjointSets(data.nbCustomers);
}

void fracclu::FractionalClustering::removeDuplicates()
{
    auto clustersAux = newClusters;
    newClusters.clear();
    // Checking for and removing duplicate clusters
    for (auto & clu1: clustersAux)
    {
        bool exists = false;
        for (auto & clu2: clusters)
        {
            if (ds::Utils::compare(clu1, clu2.second))
            {
                exists = true;
                break;
            }
        }
        if (!exists)
            newClusters.push_back(clu1);
    }
}

void fracclu::FractionalClustering::insertNewClusters(int n)
{
    if (!newClusters.empty())
    {
        for (auto & clust: newClusters)
        {
            insertCluster(clust);

            // Identifying customer multi-clusters
            std::vector<bool> aux(n, false);
            for (auto & i : clust)
                aux[i] = true;
            customerClusters.push_back(aux);
        }

        // Checking for intersected clusters
        for (auto & aux : disjoint)
            aux.insert(aux.end(), newClusters.size(), true);
        disjoint.insert(disjoint.end(), newClusters.size(), std::vector<bool> (clusters.size() + 1, true));
        auto pos = (int) (clusters.size() - newClusters.size() + 1);
        for (int i = 0; i < clusters.size(); i++)
        {
            auto k = std::max(pos, i+1);
            for (int j = k; j < clusters.size(); j++)
                if (ds::Utils::intersection(clusters[i].second, clusters[j].second))
                    disjoint[clusters[i].first][clusters[j].first] = disjoint[clusters[j].first][clusters[i].first] = false;
        }
        std::cout << "Added " << newClusters.size() << " cluster(s) to the list; list size = " << clusters.size() << std::endl;
        printClusters(clusters);
    }
    else
    {
        std::cout << "No new clusters were created!!!" << std::endl;
    }
}

void fracclu::FractionalClustering::printClusters(const std::vector<std::pair<int,std::vector<int>>> & clusters_) {

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