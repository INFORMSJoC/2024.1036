/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

// #####################################################
// It is a wrapper facilitating the invocation of functions
// from the cutsets branching module within the Lysgaard CVRPSEP code
// #####################################################

#ifndef CVRP_JOAO_CUTSETS_H
#define CVRP_JOAO_CUTSETS_H

#include <unordered_map>

#include "brnching.h"
#include "cnstrmgr.h"

#include "Data.h"

namespace cvrp_joao
{
    using cutsetsMap = std::unordered_map<std::string, std::pair<std::vector<int>, double>>;

    class Cutsets
    {
        int nbCustomers;
        CnstrMgrPointer setsCMP; /// Data structure containing the cutsets
        const bool verbose;

    public:
        Cutsets(int nbCustomers_, int vehCapacity, std::vector<int> demands,
                int nbEdges, std::vector<int> edgeTail,
                std::vector<int> edgeHead, std::vector<double> edgeLPValue,
                double target_ = 3.0, int nbSets_ = 0, bool verbose_ = false);

        ~Cutsets() = default;

        cutsetsMap getCutsets();
        static void printCutsets(const cutsetsMap & myMap);

    private:
        double boundaryTarget; /// Target value for x(delta(S)), it must be between 2.0 and 4.0
        int nbSets; /// The maximum number of identiﬁed sets

        double setTarget(double target_) const;
        int setNbOfSets(int nbSets_) const;
        static std::string generateUniqueName(const std::vector<int> & vec);
    };
}

#endif