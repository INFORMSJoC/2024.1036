#ifndef VRPTW_LOADER_H
#define VRPTW_LOADER_H

#include <string>
#include "Clustering.h"

namespace vrptw
{
    class Data;
    class Parameters;
    class Subtree;

    class Loader
    {
    public:
        Loader();

        bool loadData(const std::string & file_name);
        bool loadParameters(const std::string & file_name, int argc, char* argv[]);
        void loadClustering();
        void loadSubtree();

    private:
        Data & data;
        Parameters & parameters;
        Subtree & subtree;
        cluster::Clustering & clustering;

        bool loadVRPTWFile(std::ifstream & ifs);
        bool loadSubtreeFile(std::ifstream & ifs);
    };
}

#endif
