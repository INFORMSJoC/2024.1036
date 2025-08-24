/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

/**
 *
 * This file Model.cpp is a part of BaPCod - a generic Branch-And-Price Code.
 * https://bapcod.math.u-bordeaux.fr
 *
 * Inria Bordeaux, All Rights Reserved. 
 * See License.pdf file for license terms.
 * 
 * This file is available only for academic use. 
 * For other uses, please contact stip-bso@inria.fr.
 *
 **/

#include <Parameters.h>

#include "Model.h"
#include "bcModelingLanguageC.hpp"
#include "Data.h"
#include "RCSPSolver.h"

#include "Branching.h"
//#include "Clustering.h"
    
vrptw::Model::Model(const BcInitialisation& bc_init) : BcModel(bc_init, bc_init.instanceFile())
{
    BcObjective objective(*this);
    if (data.roundType == Data::NO_ROUND || data.roundType == Data::ROUND_ONE_DECIMAL)
        objective.setStatus(BcObjStatus::minFloat);
    else
        objective.setStatus(BcObjStatus::minInt);

    double upperBound = params.cutOffValue();
    if (upperBound != std::numeric_limits<double>::infinity())
        objective <= upperBound;
    if (std::abs(upperBound) < 1e4)
        objective.setArtCostValue(std::abs(upperBound));
    else
        objective.setArtCostValue(1e4);

    std::vector<int> customerIds;
    for (int custId = 1; custId <= data.nbCustomers; ++custId)
        customerIds.push_back(custId);

    BcMaster master(*this);

    BcConstrArray degreeConstr(master, "DEG");
    for (auto custId : customerIds)
    {
        degreeConstr(custId) == 2;
    }

    BcBranchingConstrArray vehNumberBranching(master, "VNB", SelectionStrategy::MostFractional, 1.0);
    vehNumberBranching(0); /// this is for the total number of vehicles

    BcBranchingConstrArray edgeBranching(master, "EDGE", SelectionStrategy::MostFractional, 1.0);
    if (params.enableEdgeBranching()) {
        for (int firstCustId = 0; firstCustId <= data.nbCustomers; ++firstCustId)
            for (int secondCustId = firstCustId + 1; secondCustId <= data.nbCustomers; ++secondCustId)
                edgeBranching(firstCustId, secondCustId);
        std::cout << "Standard EDGE branching ENABLED!" << std::endl;
    }
    else
        std::cout << "Standard EDGE branching DISABLED!" << std::endl;

    if (params.enableRyanFoster())
    {
        BcPackSetRyanFosterBranching packSetRyanFosterBranching(master, 1.0);
        std::cout << "RyanFoster Branching ENABLED" << std::endl;
    }

    BcColGenSpArray columnGenSP(*this);
    BcFormulation spForm = columnGenSP(0);
    auto maxNumVeh = data.maxNumVehicles;
    if (params.exactNumVehicles())
    {
        if (params.maxNumVehicles() < 1e6)
            maxNumVeh = params.maxNumVehicles();
        std::cout << "Num vehicles: " << maxNumVeh << std::endl;
        spForm == maxNumVeh;
    }
    else if ((params.minNumVehicles() > 1) || (params.maxNumVehicles() < 1e6))
    {
        maxNumVeh = std::min(params.maxNumVehicles(), data.nbCustomers);
        auto minNumVeh = std::max(params.minNumVehicles(), data.minNumVehicles);
        std::cout << "Num vehicles: " << minNumVeh << ", " << maxNumVeh << std::endl;
        spForm >= minNumVeh;
        spForm <= maxNumVeh;
    }
    else
    {
        std::cout << "Max. Num vehicles: " << maxNumVeh << std::endl;
        spForm <= maxNumVeh;
    }
    if (maxNumVeh < data.minNumVehicles)
    {
        std::cerr << "Infeasible:: The minimum number of vehicles required to meet all demands is "
                  << data.minNumVehicles << "!" << std::endl;
        exit(0);
    }

    BcVarArray xVar(spForm, "X");
    xVar.type('I');
    xVar.priorityForMasterBranching(-1);
    xVar.priorityForSubproblemBranching(-1);
    xVar.defineIndexNames(MultiIndexNames('i', 'j'));

    /// customer number 0 is the depot
    for (int firstCustId = 0; firstCustId <= data.nbCustomers; ++firstCustId )
        for (int secondCustId = firstCustId + 1; secondCustId <= data.nbCustomers; ++secondCustId)
        {
            BcVar bcVar = xVar(firstCustId, secondCustId);
            if (firstCustId == 0)
            {
                if (params.enableVNB()) vehNumberBranching[0] += 0.5 * bcVar;
                objective += data.getDepotToCustDistance(secondCustId) * bcVar;
            }
            else
            {
                degreeConstr[firstCustId] += bcVar;
                objective += data.getCustToCustDistance(firstCustId, secondCustId) * bcVar;
            }
            degreeConstr[secondCustId] += bcVar;
            if (params.enableEdgeBranching()) edgeBranching[firstCustId][secondCustId] += bcVar;
        }

    // Fixing vars or expressions for solving subtrees
    std::vector<vrptw::PackSets> packingSetsRFTobeFixed;
    if (subtree.enabled)
        packingSetsRFTobeFixed = addSubtree(master, xVar);

    RCSPSolver solver(spForm, packingSetsRFTobeFixed);
    spForm.attach(solver.getOracle());

    std::vector<int> demands(data.nbCustomers + 1, 0);
    for (auto custId : customerIds)
        demands[custId] = data.customers[custId].demand;

    // Enabling user branching
    BcBranchingConstrArray userBranching(master, "UBR");
    if (params.enableClusterBranching() || params.enableCutsetsBranching())
    {
        if (params.enableClusterBranching()) std::cout << "Cluster branching ENABLED" << std::endl;
        if (params.enableCutsetsBranching()) std::cout << "Cutsets branching ENABLED" << std::endl;
        userBranching.attach(new UserBranchingFunctor(data, params, clustering));
    }
    else if (!params.enableEdgeBranching() && !params.enableRyanFoster())
    {
        std::cerr << "No branching strategy defined!" << std::endl;
    }

    BcCapacityCutConstrArray capacityCuts(master, data.veh_capacity, demands,  false, true,
                                          -1, 3.0, 1.0);

    BcLimMemRankOneCutConstrArray limMemRank1Cuts(master);
}

std::vector<vrptw::PackSets> vrptw::Model::addSubtree(BcMaster & master, BcVarArray & xVar)
{
    std::vector<vrptw::PackSets> packingSetsTobeFixed;
    BcConstrArray branchConstr(master, "BRANCH");
    int constId = 1;

    std::cout << "SUBTREE: ";

    for (auto & branch : subtree.branch)
    {
        auto oper = "";
        if (branch.operType == Branch::OperType::EQUAL)
        {
            oper = "==";
            branchConstr(constId) == branch.rhs;
        }
        else if (branch.operType == Branch::OperType::LEQ)
        {
            oper = "<=";
            branchConstr(constId) <= branch.rhs;
        }
        else if (branch.operType == Branch::OperType::GEQ)
        {
            oper = ">=";
            branchConstr(constId) >= branch.rhs;
        }
        else if (branch.operType == Branch::OperType::RFT)
        {
            oper = "together";
            packingSetsTobeFixed.emplace_back(branch.i, branch.j, true);
        }
        else if (branch.operType == Branch::OperType::RFNOT)
        {
            oper = "not together";
            packingSetsTobeFixed.emplace_back(branch.i, branch.j, false);
        }
        else
            continue;

        double constMultiplier = 1.0;
        if (branch.varType != Branch::VarType::RF)
        {
            if (branch.varType == Branch::VarType::X)
                std::cout << "EDGE[" << branch.i << "," << branch.j << "]" << oper << branch.rhs << " ";
            else if (branch.varType == Branch::VarType::W)
                std::cout << "AggClusters[" << branch.i << "," << branch.j << "]" << oper << branch.rhs << " ";
            else if (branch.varType == Branch::VarType::Z) {
                constMultiplier = 0.5;
                std::cout << "DegCluster[" << branch.i << "]" << oper << branch.rhs << " ";
            }
            for (auto &pair: branch.varIndices) {
                BcVar bcVar = xVar(pair.first, pair.second);
                branchConstr[constId] += constMultiplier * bcVar;
            }
        }
        else
        {
            std::cout << "Pack.sets " << branch.i << " " << branch.j << " " << oper << " ";
        }

        std::cout << ((constId == subtree.branch.size()) ? " " : "| ");
        constId++;
    }
    std::cout << std::endl;

    return packingSetsTobeFixed;
}