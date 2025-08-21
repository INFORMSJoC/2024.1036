#ifndef VRPTW_MODEL_H
#define VRPTW_MODEL_H

#include <bcModelPointerC.hpp>
#include <bcModelMasterC.hpp>
#include <bcModelRCSPSolver.hpp>
#include "InputUser.h"
#include "RCSPSolver.h"
#include "bcModelingLanguageC.hpp"

namespace vrptw
{
    class Model : public BcModel, InputUser
    {
    public:
        Model(const BcInitialisation& bc_init);
        virtual ~Model() {}

    private:
        // If applicable, return the R&F packing sets to be fixed inside the RCSP function
        std::vector<PackSets> addSubtree(BcMaster & master, BcVarArray & xVar);
    };
}

#endif
