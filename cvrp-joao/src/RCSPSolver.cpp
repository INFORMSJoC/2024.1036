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

cvrp_joao::RCSPSolver::RCSPSolver(BcFormulation spForm, const std::vector<PackSets> & packingSetsRFTobeFixed) :
                        spForm(std::move(spForm)), oracle(nullptr)
{
    /*if (params.enableKPathCuts())
        network = BcNetwork(spForm, data.nbCustomers + 1, data.nbCustomers + 1, data.nbCustomers + 1);
    else
        network = BcNetwork(spForm, data.nbCustomers + 1, data.nbCustomers + 1);
    */
    BcNetwork network(spForm, data.nbCustomers + 1, data.nbCustomers + 1);

    if (params.enableCapacityResource())
    {
        cap_res = BcNetworkResource(network, 1);
        cap_res.setAsMainResource();
    } else
    {
        std::cout << "Capacity resource DISABLED" << std::endl;
    }

    if (params.dcvrp()) {
        dist_res = BcNetworkResource(network, 0);
        dist_res.setAsMainResource();
        std::cout << "Distance resource ENABLED" << std::endl;
    }

    buildVertices(network, params.enableKPathCuts());
    buildArcs(network);
    buildElemSetDistanceMatrix(network);

    for (auto & ps : packingSetsRFTobeFixed)
        network.addPermanentRyanAndFosterConstraint(ps.psId1, ps.psId2, ps.together);

    oracle = new BcRCSPFunctor(spForm);
}

void cvrp_joao::RCSPSolver::buildVertices(BcNetwork & network, bool enableCovSets)
{
    // 0 is the source and data.nbCustomers + 1 is the sink
    for (int custId = 0; custId <= data.nbCustomers + 1; ++custId)
    {
        BcVertex vertex = network.createVertex();

        if (params.enableCapacityResource())
        {
            cap_res.setVertexConsumptionLB(vertex, 0);
            cap_res.setVertexConsumptionUB(vertex, data.veh_capacity);
        }

        if (params.dcvrp())
        {
            dist_res.setVertexConsumptionLB(vertex, 0);
            dist_res.setVertexConsumptionUB(vertex, data.max_distance);
        }

        if (custId == 0)
        {
            network.setPathSource(vertex);
        }
        else if (custId <= data.nbCustomers)
        {
            vertex.setElementaritySet(custId);
            vertex.setPackingSet(custId);
            if (enableCovSets) vertex.setCoveringSet(custId);
        }
        else
        {
            network.setPathSink(vertex);
        }
    }
}

void cvrp_joao::RCSPSolver::buildArcs(BcNetwork & network)
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

            if (firstCustId == 0)
            { // firstCustId is the source
                if (params.enableCapacityResource()) cap_res.setArcConsumption(arc, data.customers[secondCustId].demand / 2.0);
                if (params.dcvrp()) dist_res.setArcConsumption(arc, (data.serv_time / 2.0)
                                                                    + data.getDepotToCustDistance(secondCustId));
            }
            else if (secondCustId <= data.nbCustomers)
            {
                if (params.enableCapacityResource()) cap_res.setArcConsumption(arc, (data.customers[firstCustId].demand
                                                + data.customers[secondCustId].demand) / 2.0);
                if (params.dcvrp()) dist_res.setArcConsumption(arc, data.serv_time
                                                                + data.getCustToCustDistance(firstCustId, secondCustId));
            }
            else
            { // secondCustId is the sink
                if (params.enableCapacityResource()) cap_res.setArcConsumption(arc, data.customers[firstCustId].demand / 2.0);
                if (params.dcvrp()) dist_res.setArcConsumption(arc, (data.serv_time / 2.0)
                                                                    + data.getDepotToCustDistance(firstCustId));
            }
        }
}

void cvrp_joao::RCSPSolver::buildElemSetDistanceMatrix(BcNetwork & network)
{
    int nbElemSets = data.nbCustomers + 1;
    std::vector<std::vector<double> > distanceMatrix(nbElemSets, std::vector<double>(nbElemSets, 1e12));

    for (int firstCustId = 1; firstCustId <= data.nbCustomers; ++firstCustId )
        for (int secondCustId = 1; secondCustId <= data.nbCustomers; ++secondCustId)
            distanceMatrix[firstCustId][secondCustId] = data.getCustToCustDistance(firstCustId, secondCustId);

    network.setElemSetsDistanceMatrix(distanceMatrix);
}


