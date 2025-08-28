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

vrptw::Route::Route(const BcSolution & solution, int id) :
        id(id), cost(solution.cost()), vertIds(), capConsumption(0.0),
        timeConsumption(0.0)
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

vrptw::Solution::Solution(const BcSolution & solution) :
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

bool vrptw::SolutionChecker::isFeasible(const BcSolution & bcSolution, bool printSolution, bool best,
                                        bool printDebugSolution) const
{
    Solution solution(bcSolution);

    if (printSolution) {
        std::cout << "------------------------------------------ " << std::endl;
        if (best)
            std::cout << "Best found solution of value " << solution.cost << " : " << std::endl;
        else
            std::cout << "New solution of value " << solution.cost << " : " << std::endl;
    }

    std::vector<int> routeIds;

    for (const auto & route : solution.routes)
        routeIds.push_back(route.id);

    solution.feasible = true;

    int depotVehicleNumber = 0;
    std::vector<int> numOfVisits(data.nbCustomers+2, 0);

    for (int routeId : routeIds)
    {
        Route & route = solution.routes[routeId];
        if (route.vertIds.empty())
            continue;

        depotVehicleNumber += 1;
        if (printSolution)
            std::cout << "Vehicle " << depotVehicleNumber << " : ";

        int routeCumDemand = 0;
        double routeCumCost = 0.0;
        double routeCumTime = data.depot_tw_start;
        auto vertIt = route.vertIds.begin();
        int prevVertId = *vertIt;
        if (*vertIt != 0) {
            solution.feasible = false; /// first vertex is not depot
        }
        if (printSolution)
            std::cout << *vertIt;
        for (++vertIt; vertIt != route.vertIds.end(); ++vertIt)
        {
            numOfVisits[*vertIt]++;
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
            double tw_start = (vertexIsDepot) ? data.depot_tw_start : data.customers[*vertIt].tw_start;
            double tw_end = (vertexIsDepot) ? data.depot_tw_end : data.customers[*vertIt].tw_end;
            routeCumTime = (std::max)(routeCumTime + distance + data.customers[prevVertId].service_time, tw_start);

            if (!vertexIsDepot)
                routeCumDemand += data.customers[*vertIt].demand;

            if (printSolution)
                std::cout << " -> " << *vertIt << "(" << routeCumDemand << "," << routeCumTime << ")";

            if (routeCumTime > tw_end + EPS)
            {
                solution.feasible = false;
                if (printSolution)
                    std::cout << "!!!";
            }

            prevVertId = *vertIt;
        }
        if ((prevVertId > 0) && (prevVertId <= data.nbCustomers)) {
            solution.feasible = false; /// last vertex is not depot
        }
        if (printSolution)
            std::cout << ", demand = " << routeCumDemand << ", time = " << routeCumTime << ", cost = "
                        << routeCumCost << std::endl;
        route.capConsumption = routeCumDemand;
        route.timeConsumption = routeCumTime;
        if (routeCumDemand > data.veh_capacity) {
            solution.feasible = false; /// capacity is exceeded
        }
    }

    for (int i=1; i < numOfVisits.size()-1; ++i) {
        if (numOfVisits[i] != 1) {
            std::cout << "Customer " << i << " was visited " << numOfVisits[i] << " times!" << std::endl;
            solution.feasible = false;
        }
    }

    if (depotVehicleNumber > data.maxNumVehicles) {
        solution.feasible = false; /// depot available number of vehicles is exceeded
    }
    
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

bool vrptw::SolutionChecker::operator()(BcSolution new_solution) const
{
    return isFeasible(new_solution, true, false);
}
