#include "Cutsets.h"
#include "cnstrmgr.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>

vrptw::Cutsets::Cutsets(int nbCustomers_, int vehCapacity, std::vector<int> demands,
                            int nbEdges, std::vector<int> edgeTail,
                            std::vector<int> edgeHead, std::vector<double> edgeLPValue,
                            double target_, int nbSets_, bool verbose_)
                            : setsCMP(), nbCustomers(nbCustomers_), boundaryTarget(setTarget(target_)), nbSets(setNbOfSets(nbSets_)), verbose(verbose_)
{
    /// Initializing the cutsets data structure
    CMGR_FreeMemCMgr(&setsCMP);
    CMGR_CreateCMgr(&setsCMP,nbSets);
    /// Required to store previously separated cutssets
    CnstrMgrPointer MyOldCutsCMP; /// Not used in practice here
    CMGR_CreateCMgr(&MyOldCutsCMP,0);
    /// Generating candidates list (stored in setsCMP) - Function from CVRPSEP lib
    /// the CVRPSEP default assume that the depot is numbered nbCustomers + 1
    /// Only information on those edges e with LP value x_e > 0 should be passed
    std::cout << "Searching for cutsets" << std::endl;
    BRNCHING_GetCandidateSets(nbCustomers,
                              demands.data(),
                              vehCapacity,
                              nbEdges,
                              edgeTail.data(),
                              edgeHead.data(),
                              edgeLPValue.data(),
                              MyOldCutsCMP,
                              boundaryTarget,
                              nbSets,
                              setsCMP);
}

double vrptw::Cutsets::setTarget(double target_) const {
    /// Cutsets target value
    if ((target_ <= 2) || (target_ >= 4))
        std::cerr << "The cutset target value must been between 2 and 4!!!" << std::endl;

    if (verbose) std::cout << "Cutset target value: " << target_ << std::endl;
    return target_;
}

int vrptw::Cutsets::setNbOfSets(int nbSets_) const {
    /// The maximum number of identiﬁed sets
    int nb;
    if ((nbSets_ > 0) && (nbSets_ <= nbCustomers))
        nb = nbSets_;
    else /// if not specified by the user set it equal to the nb of customers
        nb = nbCustomers;

    if (verbose) std::cout << "Max number of cutsets: " << nb << std::endl;
    return nb;
}

vrptw::cutsetsMap vrptw::Cutsets::getCutsets() {
    /// The same as setCMP but using unordered_map datastructures
    /// The string is hash name and the pair.fist = cutset, pair.second = cutset rhs
    cutsetsMap sets;
    for (int i = 0; i < setsCMP->Size; i++) {
        std::vector<int> list;
        for (int j = 1; j <= setsCMP->CPL[i]->IntListSize; j++)
            list.push_back(setsCMP->CPL[i]->IntList[j]);
        /// Now list contains the numbers of the customers in S
        /// The boundary x^*(\delta(S)) of this S is RHS
        auto rhs = setsCMP->CPL[i]->RHS;
        std::sort(list.begin(), list.end());
        auto name = generateUniqueName(list);
        sets[name] = std::make_pair(list, rhs);
    }
    /// CMGR_FreeMemCMgr(&setsCMP); /// Deallocate the memory

    std::cout << "Number of generated cutsets: " << sets.size() << std::endl;

    return sets;
}

/// Function to generate a unique name for the vector based on its elements
std::string vrptw::Cutsets::generateUniqueName(const std::vector<int> & vec) {
    /// Use a hash function to generate a unique identifier based on the elements of the vector
    std::hash<std::string> hasher;
    std::stringstream ss;
    for (int elem : vec) {
        ss << elem << ",";
    }
    std::string identifier = ss.str();
    return "cutset_" + std::to_string(hasher(identifier));
}

void vrptw::Cutsets::printCutsets(const cutsetsMap & myMap) {
    for (const auto & pair : myMap) {
        std::cout << "Cutset[" << pair.first << "] ";
        for (const auto & i : pair.second.first)
            std::cout << i << " ";
        std::cout << std::endl;
    }
}


