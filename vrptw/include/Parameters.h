#ifndef vrptw_PARAMETERS_H
#define vrptw_PARAMETERS_H

#include "Singleton.h"
#include "bcParameterParserC.hpp"

namespace vrptw
{
    class Parameters : public Singleton<Parameters>, ParameterParser
    {
        friend class Singleton<Parameters>;
    public:
        Parameters();
        virtual ~Parameters() {}

        bool loadParameters(const std::string & parameterFileName, int argc, char* argv[]);

        ApplicationParameter<double> cutOffValue;
        ApplicationParameter<bool> silent;
        ApplicationParameter<bool> exactNumVehicles;
        ApplicationParameter<int> minNumVehicles;
        ApplicationParameter<int> maxNumVehicles;
        ApplicationParameter<bool> roundDistances;
        ApplicationParameter<bool> enableCapacityResource;
        ApplicationParameter<bool> enableVNB;

        ApplicationParameter<bool> enableRyanFoster;
        ApplicationParameter<bool> dcvrp;

        ApplicationParameter<std::string> subTree;

        ApplicationParameter<bool> enableClusterBranching;
        ApplicationParameter<int> clusterBranchingMode;
        ApplicationParameter<std::string> clustersFilePath;
        ApplicationParameter<bool> enableRouteClusterBranching;
        ApplicationParameter<std::string> routeClusterFilePath;
        ApplicationParameter<bool> enableSingletons;
        ApplicationParameter<bool> enableBigClusters;

        ApplicationParameter<bool> enableCutsetsBranching;
        ApplicationParameter<bool> enableEdgeBranching;

        ApplicationParameter<double> stDevMultiplier;
        ApplicationParameter<double> bigClustersSizeThreshold;
        ApplicationParameter<double> decreasingStepStDev;

        ApplicationParameter<bool> enableFracClustering;
        ApplicationParameter<double> thresholdValueFracClustering;

        ApplicationParameter<bool> enableMinCut;

        ApplicationParameter<bool> enableBranchingFeatures;
    };
}

#endif
