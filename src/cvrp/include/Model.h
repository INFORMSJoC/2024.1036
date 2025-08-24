/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef CVRP_JOAO_MODEL_H
#define CVRP_JOAO_MODEL_H

#include <bcModelPointerC.hpp>
#include <bcModelMasterC.hpp>
#include <bcModelRCSPSolver.hpp>
#include "InputUser.h"
#include "RCSPSolver.h"
#include "bcModelingLanguageC.hpp"

namespace cvrp_joao
{
    class Model : public BcModel, InputUser
    {
    public:
        Model(const BcInitialisation& bc_init);
        virtual ~Model() {}

    private:
        // If applicable, return the R&F packing sets to be fixed inside the RCSP function
        std::vector<PackSets> addSubtree(BcMaster & master, BcVarArray & xVar, const std::vector<Branch> & branchs, bool atRoot = false);
    };
}

#endif
