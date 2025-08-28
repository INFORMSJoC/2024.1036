/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

/**
 *
 * This file RCSPSolver.cpp is a part of BaPCod - a generic Branch-And-Price Code.
 * https://bapcod.math.u-bordeaux.fr
 *
 * Inria Bordeaux, All Rights Reserved. 
 * See License.pdf file for license terms.
 * 
 * This file is available only for academic use. 
 * For other uses, please contact stip-bso@inria.fr.
 *
 **/

#include "RCSPSolver.h"
#include "Data.h"
#include "Parameters.h"
#include "SolutionChecker.h"

vrptw::RCSPSolver::RCSPSolver(BcFormulation spForm, const std::vector<PackSets> & packingSetsRFTobeFixed) :
                                spForm(std::move(spForm)), oracle(nullptr)
{
    BcNetwork network(spForm, data.nbCustomers + 1, data.nbCustomers + 1);

    BcNetworkResource time_res(network, 0);
    time_res.setAsMainResource();

    if ( params.enableCapacityResource() ) {
        cap_res = BcNetworkResource(network, 1);
        cap_res.setAsMainResource();
        std::cout << "Capacity resource ENABLED" << std::endl;
    }

    buildVertices(network, time_res);
    buildArcs(network, time_res);
    buildElemSetDistanceMatrix(network);

    for (auto & ps : packingSetsRFTobeFixed)
        network.addPermanentRyanAndFosterConstraint(ps.psId1, ps.psId2, ps.together);

    oracle = new BcRCSPFunctor(spForm);
}

void vrptw::RCSPSolver::buildVertices(BcNetwork & network, BcNetworkResource & time_res)
{
    for (int custId = 0; custId <= data.nbCustomers + 1; ++custId)
    {
        BcVertex vertex = network.createVertex();

        if ((custId >= 1) && (custId <= data.nbCustomers))
        {
            time_res.setVertexConsumptionLB(vertex, data.customers[custId].tw_start);
            time_res.setVertexConsumptionUB(vertex, data.customers[custId].tw_end + EPS);
        } else
        {
            time_res.setVertexConsumptionLB(vertex, data.depot_tw_start);
            time_res.setVertexConsumptionUB(vertex, data.depot_tw_end + EPS);
        }

        if ( params.enableCapacityResource() ) {
            cap_res.setVertexConsumptionLB(vertex, 0);
            cap_res.setVertexConsumptionUB(vertex, data.veh_capacity);
        }

        if (custId == 0)
        {
            network.setPathSource(vertex);
        }
        else if (custId <= data.nbCustomers)
        {
            vertex.setElementaritySet(custId);
            vertex.setPackingSet(custId);
        }
        else
        {
            network.setPathSink(vertex);
        }
    }
}

void vrptw::RCSPSolver::buildArcs(BcNetwork & network, BcNetworkResource & time_res)
{
    BcVarArray xVar(spForm, "X");

    for (int firstCustId = 0; firstCustId <= data.nbCustomers; ++firstCustId )
        for (int secondCustId = 1; secondCustId <= data.nbCustomers + 1; ++secondCustId)
        {
            if (firstCustId == secondCustId)
                continue;

            int minCustId = (std::min)(firstCustId, (secondCustId <= data.nbCustomers) ? secondCustId : 0);
            int maxCustId = (std::max)(firstCustId, (secondCustId <= data.nbCustomers) ? secondCustId : 0);

            if (minCustId == maxCustId)
                continue;

            BcArc arc = network.createArc(firstCustId, secondCustId, 0.0);
            arc.arcVar((BcVar)xVar[minCustId][maxCustId]);

            if ( params.enableCapacityResource() ) {
                cap_res.setArcConsumption(arc, secondCustId <= data.nbCustomers ? data.customers[secondCustId].demand : 0);
            }
            if (minCustId == 0)
                time_res.setArcConsumption(arc, data.getDepotToCustDistance(maxCustId)
                                                + ((firstCustId > 0) ? data.customers[firstCustId].service_time : 0.0));
            else
                time_res.setArcConsumption(arc, data.getCustToCustDistance(firstCustId, secondCustId)
                                                + data.customers[firstCustId].service_time);
        }
}

void vrptw::RCSPSolver::buildElemSetDistanceMatrix(BcNetwork & network)
{
    int nbElemSets = data.nbCustomers + 1;
    std::vector<std::vector<double> > distanceMatrix(nbElemSets, std::vector<double>(nbElemSets, 1e12));

    for (int firstCustId = 1; firstCustId <= data.nbCustomers; ++firstCustId )
        for (int secondCustId = 1; secondCustId <= data.nbCustomers; ++secondCustId)
            distanceMatrix[firstCustId][secondCustId] = data.getCustToCustDistance(firstCustId, secondCustId);

    network.setElemSetsDistanceMatrix(distanceMatrix);
}


