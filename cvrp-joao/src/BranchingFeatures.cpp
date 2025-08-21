#include "BranchingFeatures.h"
#include "DisjointSets.h"

#include <utility>
#include <algorithm>
#include <cfloat>

void cvrp_joao::BranchingFeatures::loadBranchingFeatures(const Data & data, int nbCandidates_)
{
    // Number of candidates of the SB 1º phase
    nbCandidates = nbCandidates_;
    // Identifying the two neighbors (the nearest) of each customer
    neighbors = std::vector<std::pair<int, int>> (data.nbCustomers + 1, std::make_pair(-1, -1));
    for (int i = 0; i <= data.nbCustomers; i++) {
        double dist1 = FLT_MAX, dist2 = FLT_MAX;
        auto firstNeighbor = -1, secondNeighbor = -1;
        for (int j = 0; j <= data.nbCustomers; j++) {
            if (i == j)
                continue;

            auto distAux = (i == 0) ? data.getDepotToCustDistance(j) : data.getCustToCustDistance(i, j);
            if (distAux < dist1)
            {
                dist2 = dist1;
                secondNeighbor = firstNeighbor;
                dist1 = distAux;
                firstNeighbor = j;
            }
            else if (distAux < dist2)
            {
                dist2 = distAux;
                secondNeighbor = j;
            }
        }
        neighbors[i] = std::make_pair(firstNeighbor, secondNeighbor);
    }

    ConvexHull hull(data);
    convexHull = hull.hull;
    std::cout << "Convex Hull:";
    for (auto & p : convexHull)
        std::cout << " " << p.first << " (" << p.second.first << "," << p.second.second << ")" ;
    std::cout << std::endl;
}

void cvrp_joao::BranchingFeatures::updateBranchingFeatures(const Data & data,
                                                     const std::vector<std::vector<double>> & xSolution,
                                                     const std::vector<std::vector<double> > & xReducedCost,
                                                     const std::vector<std::vector<int> > & xNbColumns)
{
    std::vector<std::pair<std::pair<int, int>, double>> candidates;
    for (int i = 0; i < xSolution.size(); i++) {
        for (int j = i + 1; j < xSolution.size(); j++) {
            double fractionalPart = xSolution[i][j] - (int) xSolution[i][j];
            candidates.emplace_back(std::make_pair(i,j), fractionalPart);
        }
    }

    // Sort the first nbCandidates (SB list size) candidates
    int sz = std::min((int) (1.25 * nbCandidates), (int) (xSolution.size() * (xSolution.size() - 1) / 2));
    std::partial_sort(candidates.begin(), candidates.begin() + sz, candidates.end(), sortRule);
    auto count = 1;
    for (auto & cand : candidates)
    {
        if (count > nbCandidates)
            break;
        count++;

        auto firstNode = cand.first.first, secondNode = cand.first.second;
        std::string name = "EDGE[" + std::to_string(firstNode) + "," + std::to_string(secondNode) + "]";

        auto it = edgeFeatures.find(name);
        if (it != edgeFeatures.end()) // if exists in list, update Features
        {
            it->second.fracValue = xSolution[firstNode][secondNode];
            it->second.avgFracValue = (it->second.nbSBEval * it->second.avgFracValue + it->second.fracValue) /
                                      (it->second.nbSBEval + 1);
            it->second.nbSBEval += 1;
            it->second.nbRoutesIn = xNbColumns[firstNode][secondNode];
            it->second.reducedCost = xReducedCost[firstNode][secondNode];
            it->second.sumDemandsNeighbors = getSumDemandsNeighbors(firstNode, secondNode, data, xSolution);
        }
        else // insert in the list
        {
            auto cost = (firstNode == 0) ? data.getDepotToCustDistance(secondNode) :
                                           data.getCustToCustDistance(firstNode, secondNode);

            auto sumDemands = data.customers[firstNode].demand + data.customers[secondNode].demand;
            auto sumDemandsNeighbors = getSumDemandsNeighbors(firstNode, secondNode, data, xSolution);

            auto distDepotToFirstNode = (firstNode == 0) ? 0.0 : data.getDepotToCustDistance(firstNode);
            auto distDepot = std::min(distDepotToFirstNode, data.getDepotToCustDistance(secondNode));

            double coordFN_x, coordFN_y;
            auto neighbor1 = (neighbors[firstNode].first != secondNode) ? neighbors[firstNode].first : neighbors[firstNode].second;
            auto neighbor2 = (neighbors[secondNode].first != firstNode) ? neighbors[secondNode].first : neighbors[secondNode].second;
            auto distNearest = (neighbor2 == 0) ? data.getDepotToCustDistance(secondNode) : data.getCustToCustDistance(secondNode, neighbor2);
            if (firstNode == 0) {
                coordFN_x = data.depot_x;
                coordFN_y = data.depot_y;
                distNearest = std::min(distNearest, data.getDepotToCustDistance(neighbor1));
            } else if (neighbor1 == 0) {
                coordFN_x = data.customers[firstNode].x;
                coordFN_y = data.customers[firstNode].y;
                distNearest = std::min(distNearest, data.getDepotToCustDistance(firstNode));
            } else {
                coordFN_x = data.customers[firstNode].x;
                coordFN_y = data.customers[firstNode].y;
                distNearest = std::min(distNearest, data.getCustToCustDistance(firstNode, neighbor1));
            }
            // get distance to the nearest facet of the convex hull
            auto coordSN_x = data.customers[secondNode].x, coordSN_y = data.customers[secondNode].y;
            auto prevConvP_x = convexHull.back().second.first, prevConvP_y = convexHull.back().second.second;
            double distConvexHull = 1e8;
            for (auto & i : convexHull)
            {
                auto nextConvP_x = i.second.first, nextConvP_y = i.second.second;

                auto distFirstNode = ConvexHull::distancePointToLine(coordFN_x, coordFN_y, prevConvP_x,
                                                                     prevConvP_y, nextConvP_x, nextConvP_y);
                auto distSecondNode = ConvexHull::distancePointToLine(coordSN_x, coordSN_y, prevConvP_x,
                                                                      prevConvP_y, nextConvP_x, nextConvP_y);
                auto aux = std::min(distFirstNode, distSecondNode);
                if (aux < distConvexHull)
                    distConvexHull = aux;

                prevConvP_x = nextConvP_x;
                prevConvP_y = nextConvP_y;
            }

            EdgeFeatures FeaturesAux = EdgeFeatures(xSolution[firstNode][secondNode], // fracValue
                                           xSolution[firstNode][secondNode], // avgFracValue
                                           cost, // cost
                                           xReducedCost[firstNode][secondNode], // reduced cost
                                           distDepot, // distDepot
                                           distConvexHull, // distConvexHull
                                           distNearest, // distNearestNeighbor
                                           sumDemands, // sumDemandsEndpoints
                                           sumDemandsNeighbors, // sumDemandsNeighbors
                                           0, // nbBranchingOn
                                           xNbColumns[firstNode][secondNode], // nbRoutesIn
                                           1); // nbSBEval

            edgeFeatures.emplace(name, FeaturesAux);
        }

        printFeatures(name, edgeFeatures[name]);
    }
}

bool cvrp_joao::BranchingFeatures::sortRule(const std::pair<std::pair<int, int>, double> & a, const std::pair<std::pair<int, int>, double> & b)
{
    return std::abs(0.5 - a.second) < std::abs(0.5 - b.second);
};

void cvrp_joao::BranchingFeatures::printFeatures(const std::string & name, const cvrp_joao::EdgeFeatures & features)
{
    std::cout << std::setprecision(4) << std::fixed
              << "BranchingFeatures: " << std::setw(10) << name
              << ", fV = " << std::setw(7) << features.fracValue
              << ", afV = " << std::setw(7) << features.avgFracValue
              << ", cost = " << std::setw(7) << features.cost
              << std::setprecision(2) << std::fixed
              << ", redC = " << std::setw(4) << features.reducedCost
              << ", dD = " << std::setw(4) << features.distDepot
              << ", dCH = " << std::setw(4) << features.distConvexHull
              << ", dN = " << std::setw(4) << features.distNearestNeighbor
              << std::setprecision(1) << std::fixed
              << ", sDE = " << std::setw(5) << features.sumDemandsEndpoints
              << ", sDN = " << std::setw(5) << features.sumDemandsNeighbors
              << std::setprecision(0) << std::fixed
              // << ", nbB = " << std::setw(2) << features.nbBranchingOn
              << ", nbRI = " << std::setw(2) << features.nbRoutesIn
              << ", nbSB = " << std::setw(2) << features.nbSBEval
              << std::endl;
}

double cvrp_joao::BranchingFeatures::getSumDemandsNeighbors(int firstNode, int secondNode, const Data & data,
                                                         const std::vector <std::vector<double>> & xSolution)
{
    double sumDemandsNeighbors = data.customers[firstNode].demand + data.customers[secondNode].demand;
    for (int i = 0; i < firstNode; i++)
    {
        if (xSolution[i][firstNode] > 0)
            sumDemandsNeighbors += data.customers[i].demand + data.customers[firstNode].demand;

        if (xSolution[i][secondNode] > 0)
            sumDemandsNeighbors += data.customers[i].demand + data.customers[firstNode].demand;
    }

    for (int i = firstNode + 1; i < secondNode; i++)
    {
        if (xSolution[firstNode][i] > 0)
            sumDemandsNeighbors += data.customers[i].demand + data.customers[firstNode].demand;

        if (xSolution[i][secondNode] > 0)
            sumDemandsNeighbors += data.customers[i].demand + data.customers[firstNode].demand;
    }

    for (int i = secondNode + 1; i <= data.nbCustomers; i++)
    {
        if (xSolution[firstNode][i] > 0)
            sumDemandsNeighbors += data.customers[i].demand + data.customers[firstNode].demand;

        if (xSolution[secondNode][i] > 0)
            sumDemandsNeighbors += data.customers[i].demand + data.customers[firstNode].demand;
    }

    return sumDemandsNeighbors;
}