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

#include <cstring>

#include <Parameters.h>

#include "Model.h"
#include "bcModelingLanguageC.hpp"
#include "bcModelNonPublicCuts.hpp"

#include "Data.h"
#include "RCSPSolver.h"
// #include "CutSeparation.h"
#include "Branching.h"

cvrp_joao::Model::Model(const BcInitialisation& bc_init) : BcModel(bc_init, bc_init.instanceFile())
{
    BcObjective objective(*this);
    if (data.roundType == Data::NO_ROUND)
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

    auto selectionStrategy = SelectionStrategy::MostFractional;
    if (params.enableRandomSBCandidates()){
        std::cout << "Random SB candidates" << std::endl;
        selectionStrategy = SelectionStrategy::Random;
    }

    BcBranchingConstrArray vehNumberBranching(master, "VNB", selectionStrategy, 1.0);
    vehNumberBranching(0); /// this is for the total number of vehicles

    BcBranchingConstrArray edgeBranching(master, "EDGE", selectionStrategy, 1.0);
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

    if (params.exactNumVehicles())
    {
        std::cout << "Num vehicles: " << data.minNumVehicles << std::endl;
        spForm == data.minNumVehicles;
    }
    else
    {
        std::cout << "Num vehicles: " << data.minNumVehicles << ", " << data.maxNumVehicles << std::endl;
        spForm >= data.minNumVehicles;
        spForm <= data.maxNumVehicles;
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
            if (!params.enableCostlyEdgeBranching() && params.enableEdgeBranching())
                edgeBranching[firstCustId][secondCustId] += bcVar;
        }

    // Fixing vars or expressions for solving subtrees
    std::vector<cvrp_joao::PackSets> packingSetsRFTobeFixed;
    if (subtree.enabledAtRoot) {
        auto trash = addSubtree(master, xVar, subtree.branchAtRoot, true);
        std::cout << "ACTIVE" << std::endl;
    }
    if (subtree.enabled)
        packingSetsRFTobeFixed = addSubtree(master, xVar, subtree.branch, false);


    RCSPSolver solver(spForm, packingSetsRFTobeFixed);
    spForm.attach(solver.getOracle());

    std::vector<int> demands(data.nbCustomers + 1, 0);
    for (auto custId : customerIds)
        demands[custId] = data.customers[custId].demand;

    // auto userFunctor = new UserNonRobustFunctor(master, 1.0, data, params);

    // Enabling user branching
    BcBranchingConstrArray userBranching(master, "UBR");
    if (params.enableClusterBranching() || params.enableCostlyEdgeBranching() || params.enableCutsetsBranching())
    {
        if (params.enableCostlyEdgeBranching()) std::cout << "Costly edge branching ENABLED" << std::endl;
        if (params.enableClusterBranching()) std::cout << "Cluster branching ENABLED" << std::endl;
        if (params.enableCutsetsBranching()) std::cout << "Cutsets branching ENABLED" << std::endl;
        userBranching.attach(
                new UserBranchingFunctor(data, params, clustering, fractionalClustering, branchingFeatures));
    }
    else if (!params.enableEdgeBranching() && !params.enableRyanFoster())
    {
        std::cerr << "No branching strategy defined!" << std::endl;
    }
    // BcCutConstrArray userRobustCuts(master, "URC", 'F', 3.0, 1.0);
    // userRobustCuts.attach(new UserRobustFunctor(data, params));

    BcCapacityCutConstrArray capacityCuts(master, data.veh_capacity, demands,  true, true, -1, 3.0, 1.0);

    BcLimMemRankOneCutConstrArray limMemRank1Cuts(master);

    if (params.enableKPathCuts())
    {
        std::cout << "SKP cuts ENABLED" << std::endl;
        BcStrongKPathCutConstrArray strongKPathCuts(master, data.veh_capacity, demands, true, false, -1, 1.0, 1.0);
    }
}

std::vector<cvrp_joao::PackSets> cvrp_joao::Model::addSubtree(BcMaster & master, BcVarArray & xVar, const std::vector<Branch> & branchs, bool atRoot)
{
    std::vector<cvrp_joao::PackSets> packingSetsTobeFixed;
    BcConstrArray branchConstr(master, "BRANCH");
    int constId = 1;

    if (atRoot)
        std::cout << "BRANCH AT ROOT, ";
    std::cout << "SUBTREE: ";

    for (auto & branch : branchs)
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
            auto count = 0;
            for (auto &pair: branch.varIndices) {
                BcVar bcVar = xVar(pair.first, pair.second);
                // std::cout << "(" << pair.first << "," << pair.second << ") ";
                // count += 1;
                // if (count % 20 == 0) std::cout << std::endl;
                branchConstr[constId] += constMultiplier * bcVar;
            }
        }
        else
        {
            std::cout << "Pack.sets " << branch.i << " " << branch.j << " " << oper << " ";
        }

        std::cout << ((constId == branchs.size()) ? " " : "| ");
        constId++;
    }
    std::cout << std::endl;

    return packingSetsTobeFixed;
}