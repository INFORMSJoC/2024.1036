
#ifndef CVRP_JOAO_BRANCHING_H
#define CVRP_JOAO_BRANCHING_H

#include "Data.h"
#include "Parameters.h"
#include "Clustering.h"
#include "Cutsets.h"
#include "FractionalClustering.h"
#include "BranchingFeatures.h"

#include "bcModelBranchingConstrC.hpp"
#include <random>
#include <utility>

namespace cvrp_joao
{
	class UserBranchingFunctor : public BcDisjunctiveBranchingConstrSeparationFunctor
	{
		const Data & data;
        const Parameters & params;
        const cluster::Clustering & clusters;
        fracclu::FractionalClustering & fractionalClusters;
        BranchingFeatures & branchingFeatures;
        int constrCount;
        std::unordered_map<std::string, size_t> cutsetsIdMap;

	public:
        UserBranchingFunctor(const Data & data_, const Parameters & params_,
                             const cluster::Clustering & clusters_,
                             fracclu::FractionalClustering & fractionalClusters_,
                             BranchingFeatures & branchingFeatures_);

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

        static bool costSort(const std::pair<std::pair<int,int>, double> a, const std::pair<std::pair<int,int>, double> & b)
        {
            return a.second > b.second;
        };

        std::vector<std::vector<double>> getReducedCosts(const BcFormulation & spForm) const;
        std::vector<std::vector<int>> getVarNbColumns(const std::list<std::pair<double, BcSolution>> & columnsInSol) const;

        bool branchingOverDefaultClusters(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                         const std::vector<std::vector<double> > & xSolution,
                                         std::list<std::pair<BcConstr, std::string> > & returnBrConstrList);
        bool branchingOverFractionalClusters(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                          const std::vector<std::vector<double> > & xSolution,
                                          std::list<std::pair<BcConstr, std::string> > & returnBrConstrList);
        bool branchingOverRouteClusters(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                       const std::vector<std::vector<double> > & xSolution,
                                       std::list<std::pair<BcConstr, std::string> > & returnBrConstrList);
        bool branchingOverCostlyEdges(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                      const std::vector<std::vector<double> > & xSolution,
                                      std::list<std::pair<BcConstr, std::string> > & returnBrConstrList,
                                      const int & candListMaxSize);
        bool branchingOverCutsets(BcBranchingConstrArray & userBranching, BcVarArray & xVar,
                                          const std::vector<std::vector<double> > & xSolution,
                                          std::list<std::pair<BcConstr, std::string> > & returnBrConstrList);

        bool computeMinCut(BcVarArray & xVar, const std::vector<std::vector<double> > & xSolution);

        static bool rootProcessed;
        static void saveRootFracSolution(std::set<BcVar> & xVarSet, const std::string & instaceName, const std::string & filePath);
	};
}

#endif
