#ifndef CVRP_JOAO_LOADER_H
#define CVRP_JOAO_LOADER_H

#include <string>
#include "Clustering.h"
#include "FractionalClustering.h"

namespace cvrp_joao
{
    class Data;
    class Parameters;
    class BranchingFeatures;
    class Subtree;

    class Loader
    {
    public:
        Loader();

        bool loadData(const std::string & file_name);
        bool loadParameters(const std::string & file_name, int argc, char* argv[]);
        void loadClustering();
        void loadFractionalClustering();
        void loadSubtree();
        void loadBranchingFeatures(int nbSBcandidates);

    private:
        Data & data;
        Parameters & parameters;
        Subtree & subtree;
        cluster::Clustering & clustering;
        fracclu::FractionalClustering & fractionalClustering;
        BranchingFeatures & branchingFeatures;

        bool loadCVRPFile(std::ifstream & ifs);
        bool loadSubtreeFile(std::ifstream & ifs, Subtree & subtree_, const std::vector<std::pair<int,std::vector<int>>> & clusters_);
    };
}

#endif
