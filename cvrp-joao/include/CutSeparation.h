
#ifndef CVRP_JOAO_CUTSEPARATION_H
#define CVRP_JOAO_CUTSEPARATION_H

#include "Data.h"
#include "Parameters.h"
#include "Utils.h"
#include "bcModelCutConstrC.hpp"

#include <random>
#include <utility>
#include <iostream>

namespace cvrp_joao
{
    struct Path
    {
        int id;
        double cost;
        double varPathValue;
        std::vector<int> vertIds;
        std::vector<int> vertAcmConsumption;

        Path(const Data & data, const BcSolution & bcSolution, double varValue, int id);
    };

    class UserNonRobustFunctor : public BcCustomNonLinearCutArrayFunctor
	{
		const Data & data;
        const Parameters & params;
		int cutCount;
		int cutRound;

	public:
        UserNonRobustFunctor(const BcFormulation & formulation, double priority, const Data & data_,
                             const Parameters & params_);

		~UserNonRobustFunctor() override;

        int cutSeparationRoutine(BcFormulation formPtr,
                                 BcSolution & projectedSol,
                                 std::list<std::pair<double, BcSolution> > & columnsInSol,
                                 const double & violationTolerance,
                                 std::list<BcCustomNonLinearCut> & cutList) override;

    private:
        bool createCMSTFile(const std::string & file_name,
                            std::vector<utils::Path> solution,
                            std::map< std::pair<int, int>, double> & xVarListValues);
        bool createTeXFile(const std::string & file_name,
                            std::vector<utils::Path> solution,
                            std::map< std::pair<int, int>, double> & xVarListValues);
	};
}

#endif
