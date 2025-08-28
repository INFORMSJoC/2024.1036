/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef CVRP_JOAO_SUBTREE_H
#define CVRP_JOAO_SUBTREE_H

#include "Singleton.h"

#include <vector>
#include <string>
#include <set>
#include <limits>
#include <cmath>

#include <iostream>

namespace cvrp_joao
{
    class Branch
    {
    public:
        enum OperType
        {
            EQUAL,
            LESS,
            GREATER,
            LEQ,
            GEQ,
            RFT, // R&F branch together
            RFNOT // R&F branch not together
        };

        enum VarType
        {
            X,
            W, // Edges agg between Clusters
            Z, // Degree of a cluster
            RF // R&F branch
        };

        VarType varType;
        OperType operType;
        int i;
        int j;
        std::vector<std::pair<int, int>> varIndices;
        double rhs;

        Branch() : varType(), operType(), i(), j(), varIndices(), rhs()
        {}
    };

    class Subtree : public Singleton<Subtree>
    {
        friend class Singleton<Subtree>;
    public:
        bool enabled;
        bool atRoot;
        std::vector<Branch> branch;

        bool enabledAtRoot;
        std::vector<Branch> branchAtRoot;

        Subtree() : enabled(false), atRoot(false), branch(), enabledAtRoot(false), branchAtRoot() {}

    };
}

#endif
