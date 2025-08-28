/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

#ifndef CVRP_JOAO_DATA_H
#define CVRP_JOAO_DATA_H

#include "Singleton.h"

#include <vector>
#include <string>
#include <set>
#include <limits>
#include <cmath>

#include <iostream>

namespace cvrp_joao
{
    class Customer
    {
    public:
        Customer(int id = -1, int demand = 0, double x = 0.0, double y = 0.0) :
                id(id), demand(demand), x(x), y(y)
        {}

        int id;
        int demand;
        double x;
        double y;
    };

    class Data : public Singleton<Data>
    {
        friend class Singleton<Data>;
    public:

        std::string name;

        /// customers and depots are indexed starting from 1, customers[0] and depots[0] are fictive
        int nbCustomers;
        std::vector<Customer> customers;
        int veh_capacity;
        int minNumVehicles;
        int maxNumVehicles;
        double depot_x;
        double depot_y;
        /// For DCVRP
        double serv_time;
        double max_distance;

        enum RoundType
        {
            NO_ROUND,
            ROUND_CLOSEST
        };

        RoundType roundType;

        double getCustToCustDistance(int firstCustId, int secondCustId) const
        {
            return getDistance(customers[firstCustId].x, customers[firstCustId].y,
                               customers[secondCustId].x, customers[secondCustId].y);
        }

        double getDepotToCustDistance(int custId) const
        {
            return getDistance(depot_x, depot_y, customers[custId].x, customers[custId].y);
        }

    private:
        Data() :
            name(), nbCustomers(0), customers(1, Customer()), veh_capacity(0), minNumVehicles(0), maxNumVehicles(10000),
            depot_x(0.0), depot_y(0.0), roundType(RoundType::NO_ROUND), serv_time(0.0), max_distance(1e6)
        {}

        double getDistance(double x1, double y1, double x2, double y2) const
        {
            double distance = sqrt((x2-x1) * (x2-x1) + (y2-y1) * (y2-y1) );
            if (roundType == ROUND_CLOSEST)
                return round(distance);
            return distance;
        }
    };
}

#endif
