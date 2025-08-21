#ifndef DISJOINTSETS_H
#define DISJOINTSETS_H

#include <vector>
#include "Data.h"

namespace ds
{
    class Utils
    {
    public:
        Utils() = default;

        typedef std::pair<std::pair<int, int>, double> Edge;
        static bool sortByWeight(const Edge & a, const Edge & b);
        static void printSets(const std::vector<std::vector<int>> & sets, int firstId);
        static bool compare(std::vector<int> & vec1, std::vector<int> & vec2);
        static bool intersection(std::vector<int> & vec1, std::vector<int> & vec2);
    };


    class Graph : public Utils
    {
    public:
        int n;
        std::vector<Edge> edges;

        //explicit Graph(double (*costFunction)(int, int), int n_);
        // Graph(double (vrptw::Data::*costFunction)(int, int), vrptw::Data &obj, int n_);
        explicit Graph(const vrptw::Data *data);
        void addEdge(int x, int y, double weight);
    };


    class DisjointSets : public Utils
    {
    public:
        explicit DisjointSets(int n);
        int findParent(int i);
        void unionRank(int x, int y);
        void printDisjointSets(int firstSetId = 1);
        std::vector<std::vector<int>> getDisjointSets(int n, bool verbose = false, int firstSetId = 1);

    private:
        int* parent;
        int* rank;
        std::vector<std::vector<int>> sets;
    };


    class MinimumCut : public Utils
    {
    public:
        MinimumCut(std::vector<std::vector<double>> weight_, int n_);
        int nbMinCuts() { return (int) cuts.size(); }
        double value() { return cuts[0].second; }
        void printCuts();

    private:
        int n;
        int nbNodes;
        std::vector<bool> shrunk;
        std::vector<std::vector<int>> bin;
        std::vector<std::vector<double>> weight;
        std::vector<std::pair<std::vector<int>, double>> cuts;

        void minCut();
        std::pair<int, double> minCutPhase(int arbitraryVertex);
    };
}


#endif