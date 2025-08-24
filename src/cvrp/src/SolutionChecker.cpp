/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

/**
 *
 * This file SolutionChecker.cpp is a part of BaPCod - a generic Branch-And-Price Code.
 * https://bapcod.math.u-bordeaux.fr
 *
 * Inria Bordeaux, All Rights Reserved. 
 * See License.pdf file for license terms.
 * 
 * This file is available only for academic use. 
 * For other uses, please contact stip-bso@inria.fr.
 *
 **/

#include "SolutionChecker.h"
#include "Data.h"

cvrp_joao::Route::Route(const BcSolution & solution, int id) :
        id(id), cost(solution.cost()), vertIds(), capConsumption(0.0)
{
    const BcNetwork network(solution.formulation().network());
    vertIds.reserve(solution.orderedIds().size() + 1);

    if (solution.orderedIds().empty())
    {
        std::cout << "Error! A vertIds in a solution is empty!" << std::endl;
        return;
    }

    auto arcIdIt = solution.orderedIds().begin();
    vertIds.push_back(network.getArc(*arcIdIt).tail().ref());
    while (arcIdIt != solution.orderedIds().end())
    {
        vertIds.push_back(network.getArc(*arcIdIt).head().ref());
        ++arcIdIt;
    }
}

cvrp_joao::Solution::Solution(const BcSolution & solution) :
        routes(), cost(solution.cost()), feasible(false)
{
    BcSolution sol = solution.next(); /// skip master solution, go straight to the subproblem solutions

    int routeId = 0;
    while (sol.defined())
    {
        routes.emplace_back(sol, routeId++);
        sol = sol.next();
    }
}

bool cvrp_joao::SolutionChecker::isFeasible(BcSolution & bcSolution, bool printSolution, bool best,
                                       bool printDebugSolution) const
{
    Solution solution(bcSolution);

//    std::set<BcVar> xVarSet;
//    bcSolution.extractVar("X", xVarSet);
//    for (const auto & bcVar : xVarSet)
//    {
//        int firstNodeId = bcVar.id().first();
//        int secondNodeId = bcVar.id().second();
//        std::cout << "(i,j,val) = (" << firstNodeId << "," << secondNodeId << "," <<  bcVar.solVal() << ")" << std::endl;
//    }

    if (printSolution) {
        std::cout << "------------------------------------------ " << std::endl;
        if (best)
            std::cout << "Best found solution of value " << solution.cost << " : " << std::endl;
        else
            std::cout << "New solution of value " << solution.cost << " : " << std::endl;
    }

    solution.feasible = true;

    int depotVehicleNumber = 0;

    for (auto & route : solution.routes)
    {
        if (route.vertIds.empty())
            continue;

        depotVehicleNumber += 1;
        if (printSolution)
            std::cout << "Vehicle " << depotVehicleNumber << " : ";

        int routeCumDemand = 0;
        double routeCumCost = 0.0;
        auto vertIt = route.vertIds.begin();
        int prevVertId = *vertIt;
        if (*vertIt != 0)
            solution.feasible = false; /// first vertex is not depot
        if (printSolution)
            std::cout << *vertIt;
        for (++vertIt; vertIt != route.vertIds.end(); ++vertIt)
        {
            bool vertexIsDepot = (*vertIt > data.nbCustomers);
            bool prevVertexIsDepot = (prevVertId == 0) || (prevVertId > data.nbCustomers);
            double distance = 0;
            if (vertexIsDepot)
                distance = data.getDepotToCustDistance(prevVertId);
            else if (prevVertexIsDepot)
                distance = data.getDepotToCustDistance(*vertIt);
            else
                distance = data.getCustToCustDistance(prevVertId, *vertIt);

            routeCumCost += distance;

            if (!vertexIsDepot)
                routeCumDemand += data.customers[*vertIt].demand;

            if (printSolution)
                std::cout << " -> " << *vertIt << "(" << routeCumDemand << ")";

            prevVertId = *vertIt;
        }
        if ((prevVertId > 0) && (prevVertId <= data.nbCustomers))
            solution.feasible = false; /// last vertex is not depot
        if (printSolution)
            std::cout << ", demand = " << routeCumDemand << ", cost = "
                        << routeCumCost << std::endl;
        route.capConsumption = routeCumDemand;
        if (routeCumDemand > data.veh_capacity)
            solution.feasible = false; /// capacity is exceeded
    }
    if (depotVehicleNumber < data.minNumVehicles || depotVehicleNumber > data.maxNumVehicles)
        solution.feasible = false; /// depot available number of vehicles is exceeded

    if (params.exactNumVehicles() && depotVehicleNumber != data.minNumVehicles)
        solution.feasible = false; /// depot available number of vehicles is exceeded

    if (printSolution)
    {
        std::cout << "Solution is " << ((solution.feasible) ? "" : "NOT ") << "feasible" << std::endl;
        std::cout << "------------------------------------------ " << std::endl;
    }

    if (printDebugSolution)
    {
        for (auto & route : solution.routes)
        {
            std::cout << "V 0";
            for (auto vertId : route.vertIds)
                std::cout << " " << vertId;
            std::cout << std::endl;
        }
        std::cout << "------------------------------------------ " << std::endl;
    }
    return solution.feasible;
}

bool cvrp_joao::SolutionChecker::operator()(BcSolution new_solution) const
{
    return isFeasible(new_solution, true, false);
}
