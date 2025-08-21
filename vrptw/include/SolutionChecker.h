#ifndef VRPTW_SOLUTIONCHECKER_H
#define VRPTW_SOLUTIONCHECKER_H

#include <bcModelPointerC.hpp>
#include <bcModelRCSPSolver.hpp>
#include "InputUser.h"

#include <vector>

namespace vrptw
{
    struct Route
    {
        int id;
        double cost;
        std::vector<int> vertIds;
        double capConsumption;
        double timeConsumption;

        Route(const BcSolution & solution, int id);
    };

    struct Solution
    {
        std::vector<Route> routes;
        double cost;
        bool feasible;

        explicit Solution(const BcSolution & solution);
    };

    class SolutionChecker : public BcSolutionFoundCallback, public InputUser
    {
    public:
        bool isFeasible(const BcSolution& solution, bool printSolution = false, bool best = false,
                        bool printDebugSolution = false) const;
        bool operator()(BcSolution new_solution) const override;
    };
}

#endif
