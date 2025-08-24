/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef CVRP_JOAO_RCSPSOLVER_H
#define CVRP_JOAO_RCSPSOLVER_H

#include "InputUser.h"

#include <vector>

#include <bcModelRCSPSolver.hpp>

namespace cvrp_joao
{
    class PackSets
    {
    public:
        int psId1;
        int psId2;
        bool together;

        explicit PackSets(int id1 = 0, int id2 = 0, bool together = false) : psId1(id1), psId2(id2), together(together)
        {}
    };

    class RCSPSolver : public InputUser
    {
    public:
        RCSPSolver(BcFormulation spForm, const std::vector<PackSets> & packingSetsRFTobeFixed);
        virtual ~RCSPSolver() {}

        BcRCSPFunctor * getOracle() { return oracle; }

    private:
        // BcNetwork network;
        BcFormulation spForm;
        BcNetworkResource cap_res; // Capacity resource
        BcNetworkResource dist_res; // Distance resource - DCVRP

        std::vector<BcVertex> toVertices;
        std::vector<BcVertex> fromVertices;
        BcRCSPFunctor * oracle;

        void buildVertices(BcNetwork & network, bool enableCovSets = false);
        void buildArcs(BcNetwork & network);
        void buildElemSetDistanceMatrix(BcNetwork & network);
    };
}

#endif
