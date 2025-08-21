#ifndef VRPTW_RCSPSOLVER_H
#define VRPTW_RCSPSOLVER_H

#include "InputUser.h"

#include <vector>

#include <bcModelRCSPSolver.hpp>

namespace vrptw
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
        BcFormulation spForm;
        BcNetworkResource cap_res; // Capacity resource

        std::vector<BcVertex> toVertices;
        std::vector<BcVertex> fromVertices;
        BcRCSPFunctor * oracle;

        void buildVertices(BcNetwork & network, BcNetworkResource & time_res);
        void buildArcs(BcNetwork & network, BcNetworkResource & time_res);
        void buildElemSetDistanceMatrix(BcNetwork & network);
    };
}

#endif
