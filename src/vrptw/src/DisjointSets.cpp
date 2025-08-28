/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */


#include "DisjointSets.h"

#include <iostream>
#include <utility>
#include <algorithm>
#include <cfloat>

bool ds::Utils::sortByWeight(const Edge & a, const Edge & b)
{
    return (a.second < b.second);
}

void ds::Utils::printSets(const std::vector<std::vector<int>> & sets, int firstId)
{
    auto count = firstId;
    for (const auto & set : sets) {
        std::cout << "C" << count;
        for (const auto & vertex : set)
        {
            std::cout << " " << vertex;
        }
        std::cout << std::endl;
        count++;
    }
}

bool ds::Utils::compare(std::vector<int> & vec1, std::vector<int> & vec2)
{
    // If they have different sizes they aren't equals
    if (vec1.size() != vec2.size())
        return false;
    // Sorting both vectors
    std::sort(vec1.begin(), vec1.end());
    std::sort(vec2.begin(), vec2.end());
    // Comparing corresponding values
    return vec1 == vec2;
}

bool ds::Utils::intersection(std::vector<int> & vec1, std::vector<int> & vec2)
{
    // Comparing values
    for (auto & i : vec1)
        for (auto & j : vec2)
            if (i == j)
                return true;
    // Comparing corresponding values
    return false;
}


ds::DisjointSets::DisjointSets(int n) : parent(new int[n+1]), rank(new int[n+1]), sets()
{
    for (int i = 1; i < n + 1; i++)
    {
        parent[i] = -1;
        rank[i] = 1;
    }
}

int ds::DisjointSets::findParent(int i)
{
    if (parent[i] == -1)
        return i;

    return parent[i] = findParent(parent[i]);
}

void ds::DisjointSets::unionRank(int x, int y)
{
    int parX = findParent(x);
    int parY = findParent(y);

    if (parX != parY)
    {
        if (rank[parX] < rank[parY])
        {
            parent[parX] = parY;
        }
        else if (rank[parX] > rank[parY])
        {
            parent[parY] = parX;
        }
        else
        {
            parent[parY] = parX;
            rank[parX] += 1;
        }
    }
}

void ds::DisjointSets::printDisjointSets(int firstSetId) {

    if (sets.empty())
        std::cerr << "The sets are not defined yet! Please use getDisjointSets() first!" << std::endl;

    std::cout << "+------ Printing disjoint sets ------+" << std::endl;
    printSets(sets, firstSetId);
    std::cout << "+----------- End of print -----------+" << std::endl;
}

std::vector<std::vector<int>> ds::DisjointSets::getDisjointSets(int n, bool verbose, int firstSetId)
{
    sets.clear();
    std::vector<bool> inSet(n+1, false);
    for (int firstVertex = 1; firstVertex <= n; firstVertex++) {
        if (inSet[firstVertex]) continue;
        std::vector<int> setAux;
        setAux.push_back(firstVertex);
        inSet[firstVertex] = true;

        for (int secondVertex = firstVertex + 1; secondVertex <= n ; secondVertex++)
        {
            if (findParent(firstVertex) == findParent(secondVertex))
            {
                setAux.push_back(secondVertex);
                inSet[secondVertex] = true;
            }
        }
        sets.push_back(setAux);
    }

    if (verbose)
        printDisjointSets(firstSetId);

    return sets;
}


//ds::Graph::Graph(double (vrptw::Data::*costFunction)(int, int), vrptw::Data &obj, int n_)
ds::Graph::Graph(const vrptw::Data *data) : n(data->nbCustomers), edges() {
    for (int firstNodeId = 1; firstNodeId < n; ++firstNodeId)
        for (int secondNodeId = firstNodeId + 1; secondNodeId <= n; ++secondNodeId)
        {
            double weight = data->getCustToCustDistance(firstNodeId, secondNodeId);
            addEdge(firstNodeId, secondNodeId, weight);
        }
}

void ds::Graph::addEdge(int x, int y, double weight)
{
    edges.push_back({{x, y}, weight});
}


ds::MinimumCut::MinimumCut(std::vector<std::vector<double>> weight_, int n_) : weight(std::move(weight_)),
                                                                               n(n_), nbNodes(n_), cuts(),
                                                                               shrunk(), bin()
{
    minCut();
}

void ds::MinimumCut::minCut()
{
    // Initialize aux vectors
    shrunk = std::vector<bool> (n + 1, false);
    bin.resize(n + 1);
    for (int i = 0; i <= n; i++)
    {
        bin[i].push_back(i);
        weight[i][i] = 0.0;
    }
    // Computes the min cut among all st-cuts
    auto minVal = FLT_MAX;
    cuts.clear();
    while (nbNodes > 1)
    {
        auto cutAux = minCutPhase(1);
        if (cutAux.second < minVal)
        {
            cuts.clear();
            minVal = cutAux.second;
            cuts.emplace_back(bin[cutAux.first], minVal);
        }
        else if (cutAux.second == minVal)
        {
            cuts.emplace_back(bin[cutAux.first], minVal);
        }
    }
}

std::pair<int, double> ds::MinimumCut::minCutPhase(int arbitraryVertex)
{
    std::vector<bool> assigned(n + 1, false);
    assigned[arbitraryVertex] = true;
    // Initial vertices weight
    std::vector<double> vertexWeight(n + 1, 0.0);
    for (int i = 1; i <= n; i++)
        if (!shrunk[i])
            vertexWeight[i] = weight[arbitraryVertex][i];

    auto prev = arbitraryVertex;
    for (int i = 1; i < nbNodes; i++)
    {
        // The most tightly connected vertex
        int tight = -1;
        for (int j = 1; j <= n; j++)
            if (!shrunk[j] && !assigned[j] && (tight < 0 || vertexWeight[tight] < vertexWeight[j]))
                tight = j;

        assigned[tight] = true;

        if (i == nbNodes - 1)
        {
            // Update edges weight
            for (int j = 1; j <= n; j++)
                weight[j][prev] = weight[prev][j] += weight[tight][j];

            // Shrink the tightly connected vertex into the bin of the prev vertex
            shrunk[tight] = true;
            for (auto k : bin[tight])
                bin[prev].push_back(k);

            nbNodes--;
            return std::make_pair(tight, vertexWeight[tight]);
        }

        prev = tight;
        // Update vertices weight
        for (int j = 1; j <= n; j++)
            if (!assigned[j] && !shrunk[j])
                vertexWeight[j] += weight[tight][j];
    }
}

void ds::MinimumCut::printCuts()
{
    if (nbMinCuts() == 0)
    {
        std::cout << "There are no cuts to print!" << std::endl;
    }
    else
    {
        auto count = 1;
        for (auto & cut : cuts)
        {
            std::cout << "Cut#" << count;
            for (auto i : cut.first)
                std::cout << " " << i;
            std::cout << std::endl;
            count++;
        }
    }
}