/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

/**
 *
 * This file Main.cpp is a part of BaPCod - a generic Branch-And-Price Code.
 * https://bapcod.math.u-bordeaux.fr
 *
 * Inria Bordeaux, All Rights Reserved. 
 * See License.pdf file for license terms.
 * 
 * This file is available only for academic use. 
 * For other uses, please contact stip-bso@inria.fr.
 *
 **/

#include <iostream>

#include <bcModelPointerC.hpp>
#include <bcModelMasterC.hpp>

#include "Parameters.h"
#include "Loader.h"

#include "Model.h"
#include "SolutionChecker.h"

using namespace std;

int main(int argc, char** argv)
{
    BcInitialisation bapcodInit(argc, argv, "", true); // BaPCod parameters are read here (-b option)

    vrptw::Loader loader;
    if (!loader.loadParameters(bapcodInit.configFile(), argc, argv) // Application parameters are read here (-a option)
        || !loader.loadData(bapcodInit.instanceFile()))
        return -1;

    loader.loadClustering();
    loader.loadSubtree();

    vrptw::SolutionChecker * sol_checker = new vrptw::SolutionChecker;

    vrptw::Model model(bapcodInit);
    model.attach(sol_checker);

    BcSolution solution = model.solve();
    bool feasibleSol = (solution.defined()) && sol_checker->isFeasible(solution, true, true, true);

    bapcodInit.outputBaPCodStatistics(bapcodInit.instanceFile());

    double rootGap = (bapcodInit.getStatisticValue("bcRecBestInc") - bapcodInit.getStatisticValue("bcRecRootDb"))
                     / bapcodInit.getStatisticValue("bcRecRootDb");
    double bestGap = (bapcodInit.getStatisticValue("bcRecBestInc") - bapcodInit.getStatisticValue("bcRecBestDb"))
                     / bapcodInit.getStatisticValue("bcRecBestDb");

    size_t lastPostOfSlash = bapcodInit.instanceFile().find_last_of("/");
    std::string instanceName =  bapcodInit.instanceFile().substr(lastPostOfSlash + 1);

    std::cout << ">>-!-!-<<" << std::endl
              << "Instance,InitUB,FeasFinalSol,bcFailToSolveModel,bcCountNodeProc,bcRecRootDb,bcRecBestDb,bcRecBestInc,"
              << "rootGap,bestGap,bcCountMastSol,bcCountCg,bcCountSpSol,bcCountCol,bcCountCutInMaster,bcCountCutCAP,"
              << "bcCountCutR1C,bcCountCut1rowPackR1C,bcCountCut3rowPackR1C,bcCountCut4rowPackR1C,"
              << "bcCountCut5rowPackR1C,bcCountRootActiveCutCAP,bcCountRootActiveCutR1C,bcCountRootActive1rowPackR1C,"
              << "bcCountRootActive3rowPackR1C,bcCountRootActive4rowPackR1C,bcCountRootActive5rowPackR1C,"
              << "bcTimeMastMPsol,bcTimeColGen,bcTimeCutSeparation,bcTimeAddCutToMaster,bcTimeRedCostFixAndEnum,"
              << "bcTimeEnumMPsol,bcTimeRootEval,bcTimeBaP" << std::endl
              << instanceName << ","
              << vrptw::Parameters::getInstance().cutOffValue() << ","
              << feasibleSol << ","
              << bapcodInit.getStatisticValue("bcFailToSolveModel") << ","
              << bapcodInit.getStatisticCounter("bcCountNodeProc") << ","
              << std::setprecision(3) << std::fixed
              << bapcodInit.getStatisticValue("bcRecRootDb") << ","
              << bapcodInit.getStatisticValue("bcRecBestDb") << ","
              << bapcodInit.getStatisticValue("bcRecBestInc") << ","
              << rootGap << ","
              << bestGap << ","
              << bapcodInit.getStatisticCounter("bcCountMastSol") << ","
              << bapcodInit.getStatisticCounter("bcCountCg") << ","
              << bapcodInit.getStatisticCounter("bcCountSpSol") << ","
              << bapcodInit.getStatisticCounter("bcCountCol") << ","
              << bapcodInit.getStatisticCounter("bcCountCutInMaster") << ","
              << bapcodInit.getStatisticCounter("bcCountCutCAP") << ","
              << bapcodInit.getStatisticCounter("bcCountCutR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountCut1rowPackR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountCut3rowPackR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountCut4rowPackR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountCut5rowPackR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountRootActiveCutCAP") << ","
              << bapcodInit.getStatisticCounter("bcCountRootActiveCutR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountRootActive1rowPackR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountRootActive3rowPackR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountRootActive4rowPackR1C") << ","
              << bapcodInit.getStatisticCounter("bcCountRootActive5rowPackR1C") << ","
              << std::setprecision(6) << std::fixed
              << bapcodInit.getStatisticTime("bcTimeMastMPsol") << ","
              << bapcodInit.getStatisticTime("bcTimeColGen") << ","
              << bapcodInit.getStatisticTime("bcTimeCutSeparation") << ","
              << bapcodInit.getStatisticTime("bcTimeAddCutToMaster") << ","
              << bapcodInit.getStatisticTime("bcTimeRedCostFixAndEnum") << ","
              << bapcodInit.getStatisticTime("bcTimeEnumMPsol") << ","
              << bapcodInit.getStatisticTime("bcTimeRootEval") << ","
              << bapcodInit.getStatisticTime("bcTimeBaP")
              << std::endl;
    std::cout << ">>-!-!-<<" << std::endl;


    return 0;
}
