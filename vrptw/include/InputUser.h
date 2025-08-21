#ifndef VRPTW_INPUTUSER_H
#define VRPTW_INPUTUSER_H

#include "Data.h"
#include "Parameters.h"
#include "Clustering.h"
#include "Subtree.h"

namespace vrptw
{
    class InputUser
    {
    protected:
        InputUser() : data(Data::getInstance()), params(Parameters::getInstance()), subtree(Subtree::getInstance()),
        clustering(cluster::Clustering::getInstance()) {}

        const Data & data;
        const Parameters & params;
        const cluster::Clustering & clustering;
        const Subtree & subtree;
    };
}

#endif
