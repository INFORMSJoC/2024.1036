#ifndef CVRP_JOAO_BRANCHINGFEATURES_H
#define CVRP_JOAO_BRANCHINGFEATURES_H

#include "Data.h"
#include "Parameters.h"
#include "Clustering.h"
#include "FractionalClustering.h"

#include "Singleton.h"

#include <iostream>
#include <map>
#include <stack>

namespace cvrp_joao
{
    class EdgeFeatures
    {
    public:
        EdgeFeatures(double fracValue = 0.0, double avgFracValue = 0.0, double cost = 0.0, double reducedCost = 0.0,
                  double distDepot = 0.0, double distConvexHull = 0.0, double distNearestNeighbor = 0.0,
                  double sumDemandsEndpoints = 0.0, double sumDemandsNeighbors = 0.0, int nbBranchingOn = 0,
                  int nbRoutesIn = 0, int nbSBEval = 0) :
                  fracValue(fracValue), avgFracValue(avgFracValue), cost(cost), reducedCost(reducedCost),
                  distDepot(distDepot), distConvexHull(distConvexHull), distNearestNeighbor(distNearestNeighbor),
                  sumDemandsEndpoints(sumDemandsEndpoints), nbSBEval(nbSBEval),
                  sumDemandsNeighbors(sumDemandsNeighbors), nbBranchingOn(nbBranchingOn), nbRoutesIn(nbRoutesIn)
        {}

        double fracValue;
        double avgFracValue;
        double cost;
        double reducedCost;
        double distDepot;
        double distConvexHull;
        double distNearestNeighbor;
        double sumDemandsEndpoints;
        double sumDemandsNeighbors; // endpoints + neighbors
        int nbBranchingOn;
        int nbRoutesIn;
        int nbSBEval;
    };

    // Find the convex hull of a set of points using Graham Scan algorithm
    // Code based on https://github.com/kartikkukreja/blog-codes/blob/master/src/Graham%20Scan%20Convex%20Hull.cpp
    class ConvexHull
    {
    public:
        std::vector<std::pair<int, std::pair<double, double>>> hull;

        explicit ConvexHull(const Data & data)
        {
            getConvexHull(data);
        };

        static double distancePointToLine(double px1, double py1, double lx1, double ly1, double lx2, double ly2)
        {
            auto num = std::abs((lx2-lx1) * (ly1-py1) - (lx1-px1) * (ly2-ly1));
            auto den = std::sqrt(std::pow(lx2-lx1, 2) - std::pow(ly2-ly1, 2));
            return num/den;
        }

    private:

        // the bottommost point
        std::pair<int, std::pair<double, double>> pivot;

        double squareDistance(std::pair<double, double> p1, std::pair<double, double> p2)
        {
            return (p2.first - p1.first) * (p2.first - p1.first) + (p2.second - p1.second) * (p2.second - p1.second);
        }

        int orientation(std::pair<double, double> p1, std::pair<double, double> p2, std::pair<double, double> p3)
        {
            double area = (p2.first - p1.first) * (p3.second - p1.second) - (p2.second - p1.second) * (p3.first - p1.first);
            if (area > 0)
                return -1; // if a -> b -> c forms a counter-clockwise turn
            else if (area < 0)
                return 1; // if a -> b -> c forms a clockwise turn
            return 0; // if a -> b -> c are collinear
        }

        // comparison is done first on y coordinate and then on x coordinate
        bool pointComp(std::pair<double, double> p1, std::pair<double, double> p2)
        {
            if (p1.second != p2.second)
                return p1.second < p2.second;
            return p1.first != p2.first;
        }

        // sort function rule used for sorting points according to the pivot
        bool compare(std::pair<int, std::pair<double, double>> p1, std::pair<int, std::pair<double, double>> p2)
        {
            int order = orientation(pivot.second, p1.second, p2.second);
            if (order == 0)
                return squareDistance(pivot.second, p1.second) < squareDistance(pivot.second, p2.second);
            return (order == -1);
        }

        std::stack<std::pair<int, std::pair<double, double>>> grahamScan(std::vector<std::pair<int, std::pair<double, double>>> & points, int N)
        {
            std::stack<std::pair<int, std::pair<double, double>>> hull_;

            if (N < 3)
                return hull_;

            // find the pivot bottommost point (lowest y coordinate)
            int leastY = 0;
            for (int i = 1; i < N; i++)
                if (pointComp(points[i].second, points[leastY].second))
                    leastY = i;

            // swap the pivot with the first point
            auto temp = points[0];
            points[0] = points[leastY];
            points[leastY] = temp;

            // sort the remaining point according to polar order about the pivot
            pivot = points[0];
            std::sort(points.begin(), points.end(),
                      std::bind(&ConvexHull::compare, this, std::placeholders::_1, std::placeholders::_2));

            hull_.push(points[0]);
            hull_.push(points[1]);
            hull_.push(points[2]);

            for (int i = 3; i < N; i++) {
                std::pair<int, std::pair<double, double>> top = hull_.top();
                hull_.pop();
                while (hull_.size() > 1 && orientation(hull_.top().second, top.second, points[i].second) != -1)   {
                    top = hull_.top();
                    hull_.pop();
                }
                hull_.push(top);
                hull_.push(points[i]);
            }
            return hull_;
        }

        void getConvexHull(const Data & data)
        {
            std::vector<std::pair<int, std::pair<double, double>>> points;
            points.emplace_back(0, std::make_pair(data.depot_x, data.depot_y)); // depot point
            for (int i = 1; i <= data.nbCustomers; ++i)
                points.emplace_back(i, std::make_pair(data.customers[i].x, data.customers[i].y));

            auto hull_ = grahamScan(points, data.nbCustomers + 1);
            while (!hull_.empty())
            {
                auto p = hull_.top();
                hull.push_back(p);
                hull_.pop();
            }
        }
    };

    class BranchingFeatures : public Singleton<BranchingFeatures>
    {
        friend class Singleton<BranchingFeatures>;

    public:
        void loadBranchingFeatures(const Data & data, int nbCandidates_);
        void updateBranchingFeatures(const Data & data, const std::vector<std::vector<double> > & xSolution,
                                  const std::vector<std::vector<double> > & xReducedCost,
                                  const std::vector<std::vector<int> > & xNbColumns);

    private:
        std::map<std::string, EdgeFeatures> edgeFeatures;
        std::vector<std::pair<int, std::pair<double, double>>> convexHull;
        int nbCandidates;
        std::vector<std::pair<int, int>> neighbors;

        BranchingFeatures() : edgeFeatures(), convexHull(), nbCandidates(0), neighbors() {}

        static bool sortRule(const std::pair<std::pair<int, int>, double> & a, const std::pair<std::pair<int, int>, double> & b);
        static void printFeatures(const std::string & name, const EdgeFeatures & features);
        static double getSumDemandsNeighbors(int firstNode, int secondNode, const Data & data,
                                          const std::vector<std::vector<double> > & xSolution);

    };

}

#endif