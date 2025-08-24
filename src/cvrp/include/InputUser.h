/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef CVRP_JOAO_INPUTUSER_H
#define CVRP_JOAO_INPUTUSER_H

#include "Data.h"
#include "Parameters.h"
#include "Subtree.h"
#include "Clustering.h"
#include "FractionalClustering.h"
#include "BranchingFeatures.h"

namespace cvrp_joao
{
    class InputUser
    {
    protected:
        InputUser() : data(Data::getInstance()), params(Parameters::getInstance()), subtree(Subtree::getInstance()),
                      clustering(cluster::Clustering::getInstance()),
                      fractionalClustering(fracclu::FractionalClustering::getInstance()),
                      branchingFeatures(BranchingFeatures::getInstance())
        {}

        const Data & data;
        const Parameters & params;
        const Subtree & subtree;
        const cluster::Clustering & clustering;
        fracclu::FractionalClustering & fractionalClustering;
        BranchingFeatures & branchingFeatures;
    };
}

#endif
