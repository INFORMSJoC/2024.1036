#ifndef VRPTW_DATA_H
#define VRPTW_DATA_H

#include "Singleton.h"

#include <vector>
#include <string>
#include <set>
#include <limits>
#include <cmath>

#define EPS 1e-6

namespace vrptw
{

    class Customer
    {
    public:
        Customer(int id = -1, int demand = 0, double x = 0.0, double y = 0.0,
                 double tw_start = -1e12, double tw_end = 1e12, double service_time = 0.0) :
                id(id), demand(demand), x(x), y(y), tw_start(tw_start), tw_end(tw_end), service_time(service_time)
        {}

        int id;
        int demand;
        double x;
        double y;
        double tw_start;
        double tw_end;
        double service_time;
    };

    class Data : public Singleton<Data>
    {
        friend class Singleton<Data>;
    public:

        std::string name;

        /// customers are indexed starting from 1, so customers[0] is fictive
        int nbCustomers;
        std::vector<Customer> customers;
        int veh_capacity;
        int maxNumVehicles;
        int minNumVehicles;
        double depot_x;
        double depot_y;
        double depot_tw_start;
        double depot_tw_end;

        enum RoundType
        {
            NO_ROUND,
            ROUND_CLOSEST,
            ROUND_UP,
            ROUND_ONE_DECIMAL
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
            name(), nbCustomers(0), customers(1, Customer()), veh_capacity(0), minNumVehicles(0),
            maxNumVehicles(10000), depot_x(0.0), depot_y(0.0), depot_tw_start(0.0), depot_tw_end(1e12), 
            roundType(RoundType::NO_ROUND)
        {}

        double getDistance(double x1, double y1, double x2, double y2) const
        {
            double distance = sqrt((x2-x1) * (x2-x1) + (y2-y1) * (y2-y1) );
            if (roundType == ROUND_CLOSEST)
                return round(distance);
            else if (roundType == ROUND_UP)
                return ceil(distance);
            else if (roundType == ROUND_ONE_DECIMAL)
                return floor(distance * 10) / 10;
            return distance;
        }
    };
}

#endif
