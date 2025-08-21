
#ifndef vrptw_BRANCHING_H
#define vrptw_BRANCHING_H

#include "Data.h"
#include "Parameters.h"
#include "Clustering.h"
#include "Cutsets.h"

#include "bcModelBranchingConstrC.hpp"
#include <random>
#include <utility>

namespace vrptw
{
	class UserBranchingFunctor : public BcDisjunctiveBranchingConstrSeparationFunctor
	{
		const Data & data;
        const Parameters & params;
        const cluster::Clustering & clusters;
        int constrCount;
        std::unordered_map<std::string, size_t> cutsetsIdMap;

	public:
        UserBranchingFunctor(const Data & data_, const Parameters & params_, const cluster::Clustering & clusters_);

		~UserBranchingFunctor() override;

        virtual bool operator() (BcFormulation master, BcSolution & primalSol,
                                 std::list<std::pair<double, BcSolution>> & columnsInSol,
                                 const int & candListMaxSize,
                                 std::list<std::pair<BcConstr, std::string> > & returnBrConstrList) override;

    private:
        static bool sortRule(const std::pair<int, double> a, const std::pair<int, double> & b)
        {
            return std::abs(0.5 - a.second) < std::abs(0.5 - b.second);
        };

        bool branchingOverDefaultClusters(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                         const std::vector<std::vector<double> > & xSolution,
                                         std::list<std::pair<BcConstr, std::string> > & returnBrConstrList);
        bool branchingOverCutsets(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                  const std::vector<std::vector<double> > & xSolution,
                                  std::list<std::pair<BcConstr, std::string> > & returnBrConstrList);
	};
}

#endif
