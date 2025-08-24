/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

/**
 *
 * This file Parameters.cpp is a part of BaPCod - a generic Branch-And-Price Code.
 * https://bapcod.math.u-bordeaux.fr
 *
 * Inria Bordeaux, All Rights Reserved. 
 * See License.pdf file for license terms.
 * 
 * This file is available only for academic use. 
 * For other uses, please contact stip-bso@inria.fr.
 *
 **/

#include "Parameters.h"

cvrp_joao::Parameters::Parameters() :
        silent("silent", false),
        cutOffValue("cutOffValue", std::numeric_limits<double>::infinity()),
        exactNumVehicles("exactNumVehicles", false),
        minNumVehicles("minNumVehicles", 1),
        maxNumVehicles("maxNumVehicles", 1e6),
        roundDistances("roundDistances", true),
        enableCapacityResource("enableCapacityResource", true),
        enableVNB("enableVNB", true),
        enableRyanFoster("enableRyanFoster", false),
        enableKPathCuts("enableKPathCuts", false),
        dcvrp("dcvrp", false),
        subTree("subTree","","Subtree file path"),
        subTreeAtRoot("subTreeAtRoot","","Subtree at root file path"),
        enableCostlyEdgeBranching("enableCostlyEdgeBranching", false),
        enableEdgeBranching("enableEdgeBranching", true),
        enableRandomSBCandidates("enableRandomSBCandidates", false),
        enableClusterBranching("enableClusterBranching", false),
        enableCutsetsBranching("enableCutsetsBranching", false),
        clusterBranchingMode("clusterBranchingMode", 1,
                             "1 -> Activated: Applies MST or TSP clustering; "
                             "2 -> Activated: User must provide the clusters file (option --clustersFileName)"),
        enableTSPClustering("enableTSPClustering", false),
        enableBothClustering("enableBothClustering", false),
        clustersFilePath("clustersFilePath","","Clusters file path"),
        clusterBranchingAtRoot("clusterBranchingAtRoot","","Clusters at root file path"),
        enableRouteClusterBranching("enableRouteClusterBranching", false),
        routeClusterFilePath("routeClusterFilePath","","Routes file path (CVRP solution format)"),
        enableSingletons("enableSingletons", true),
        enableBigClusters("enableBigClusters", false),
        stDevMultiplier("stDevMultiplier", -1.0),
        stDevMultiplierTSP("stDevMultiplierTSP", -1.0),
        decreasingStepStDev("decreasingStepStDev", -1.0),
        bigClustersSizeThreshold("bigClustersSizeThreshold", -1.0),
        enableFracClustering("enableFracClustering", false),
        thresholdValueFracClustering("thresholdValueFracClustering", 0.5),
        enableMinCut("enableMinCut", false),
        enableBranchingFeatures("enableBranchingFeatures", false),
        rootFracSolutionFilePath("rootFracSolutionFilePath", "", "File to save the root frac. solution")
{}

bool cvrp_joao::Parameters::loadParameters(const std::string & parameterFileName, int argc, char* argv[])
{
    setParameterFileName(parameterFileName);
    addApplicationParameter(silent);
    addApplicationParameter(cutOffValue);
    addApplicationParameter(exactNumVehicles);
    addApplicationParameter(minNumVehicles);
    addApplicationParameter(maxNumVehicles);
    addApplicationParameter(roundDistances);
    addApplicationParameter(enableCapacityResource);
    addApplicationParameter(enableVNB);

    addApplicationParameter(enableRyanFoster);
    addApplicationParameter(enableKPathCuts);
    addApplicationParameter(dcvrp);

    addApplicationParameter(subTree);
    addApplicationParameter(subTreeAtRoot);

    addApplicationParameter(enableCostlyEdgeBranching);
    addApplicationParameter(enableEdgeBranching);
    addApplicationParameter(enableRandomSBCandidates);

    addApplicationParameter(enableClusterBranching);
    addApplicationParameter(clusterBranchingMode);
    addApplicationParameter(clusterBranchingAtRoot);
    addApplicationParameter(enableTSPClustering);
    addApplicationParameter(enableBothClustering);
    addApplicationParameter(clustersFilePath);
    addApplicationParameter(enableRouteClusterBranching);
    addApplicationParameter(routeClusterFilePath);
    addApplicationParameter(enableSingletons);
    addApplicationParameter(enableBigClusters);

    addApplicationParameter(enableCutsetsBranching);

    addApplicationParameter(stDevMultiplier);
    addApplicationParameter(stDevMultiplierTSP);
    addApplicationParameter(decreasingStepStDev);
    addApplicationParameter(bigClustersSizeThreshold);

    addApplicationParameter(enableFracClustering);
    addApplicationParameter(thresholdValueFracClustering);

    addApplicationParameter(enableMinCut);
    addApplicationParameter(enableBranchingFeatures);

    addApplicationParameter(rootFracSolutionFilePath);

    parse(argc, argv);

    return true;
}
