/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef CVRP_JOAO_PARAMETERS_H
#define CVRP_JOAO_PARAMETERS_H

#include "Singleton.h"
#include "bcParameterParserC.hpp"

namespace cvrp_joao
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
        ApplicationParameter<bool> enableKPathCuts;
        ApplicationParameter<bool> dcvrp;

        ApplicationParameter<std::string> subTree;
        ApplicationParameter<std::string> subTreeAtRoot;

        ApplicationParameter<bool> enableCostlyEdgeBranching;
        ApplicationParameter<bool> enableEdgeBranching;
        ApplicationParameter<bool> enableRandomSBCandidates;

        ApplicationParameter<bool> enableClusterBranching;
        ApplicationParameter<int> clusterBranchingMode;
        ApplicationParameter<std::string> clusterBranchingAtRoot;
        ApplicationParameter<bool> enableTSPClustering;
        ApplicationParameter<bool> enableBothClustering;
        ApplicationParameter<std::string> clustersFilePath;
        ApplicationParameter<bool> enableRouteClusterBranching;
        ApplicationParameter<std::string> routeClusterFilePath;
        ApplicationParameter<bool> enableSingletons;
        ApplicationParameter<bool> enableBigClusters;

        ApplicationParameter<bool> enableCutsetsBranching;

        ApplicationParameter<double> stDevMultiplier;
        ApplicationParameter<double> stDevMultiplierTSP;
        ApplicationParameter<double> bigClustersSizeThreshold;
        ApplicationParameter<double> decreasingStepStDev;

        ApplicationParameter<bool> enableFracClustering;
        ApplicationParameter<double> thresholdValueFracClustering;

        ApplicationParameter<bool> enableMinCut;

        ApplicationParameter<bool> enableBranchingFeatures;

        ApplicationParameter<std::string> rootFracSolutionFilePath;
    };
}

#endif
