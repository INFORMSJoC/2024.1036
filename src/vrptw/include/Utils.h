/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef UTILS_H
#define UTILS_H

#include "Data.h"
#include "SolutionChecker.h"

#include <vector>
#include <random>


namespace utils
{
    struct Path
    {
        int id;
        double cost;
        double varPathValue;
        std::vector<int> vertIds;
        std::vector<int> vertAcmConsumption;

        Path(const vrptw::Data & data, const BcSolution & bcSolution, double varValue, int id);
    };

    class AuxiliaryFiles
    {
        const vrptw::Data & data;
        const std::string & type;
        const std::string & fileName;

    public:
        AuxiliaryFiles(const BcSolution & bcSolution, const vrptw::Data & data_, const std::string & fileName_,
                       const std::string & type_);
        AuxiliaryFiles(const std::vector<double> & edgesList, const vrptw::Data & data_, const std::string & fileName_,
                       const std::string & type_);

    private:
        bool createMSTTeXfile();
        bool createClusterTexFile();
        bool createSolutionTeXfile(const BcSolution & bcSolution);
        bool createFracPathTeXFile(const std::vector<Path> & paths,
                                   const std::map< std::pair<int, int>, double> & xVarListValues);
        bool createCMSTFile(const std::vector<Path> & paths,
                            const std::map< std::pair<int, int>, double> & xVarListValues);

    };
}


#endif