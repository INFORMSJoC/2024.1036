/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */

/**
 *
 * This file Loader.cpp is a part of BaPCod - a generic Branch-And-Price Code.
 * https://bapcod.math.u-bordeaux.fr
 *
 * Inria Bordeaux, All Rights Reserved. 
 * See License.pdf file for license terms.
 * 
 * This file is available only for academic use. 
 * For other uses, please contact stip-bso@inria.fr.
 *
 **/

#include "Loader.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <locale>
#include <cmath>
#include <cstring>

#include "Data.h"
#include "Parameters.h"
#include "Clustering.h"
#include "Subtree.h"

vrptw::Loader::Loader() : data(Data::getInstance()), parameters(Parameters::getInstance()), subtree(Subtree::getInstance()),
                          clustering(cluster::Clustering::getInstance()) {}

bool vrptw::Loader::loadParameters(const std::string & file_name, int argc, char* argv[])
{
    return parameters.loadParameters(file_name, argc, argv);
}

bool vrptw::Loader::loadData(const std::string & file_name)
{
    std::ifstream ifs(file_name.c_str(), std::ios::in);
    if (!ifs)
    {
        std::cerr << "Instance reader error : cannot open file " << file_name << std::endl;
        return false;
    }

    if (ifs.eof())
    {
        std::cout << "Instance reader error : empty input file " << file_name << std::endl;
        ifs.close();
        return false;
    }

    data.name = file_name;

    bool success = false;
    success = loadVRPTWFile(ifs);
    ifs.close();

    return success;
}


bool vrptw::Loader::loadVRPTWFile(std::ifstream & ifs)
{
    if (parameters.roundDistances())
        data.roundType = Data::ROUND_ONE_DECIMAL;

    std::string line;
    std::getline(ifs, line);

    switch (line.at(0)) {
        case 'r':
        case 'R':
        case 'c':
        case 'C':
            break;
        default:
            return false;
    }
    std::getline(ifs, line);
    std::getline(ifs, line);
    std::getline(ifs, line);
    ifs >> data.maxNumVehicles >> data.veh_capacity;

    if ((data.maxNumVehicles <= 0) || (data.veh_capacity <= 0) || ifs.eof() || ifs.bad())
        return false;

    std::getline(ifs, line);
    std::getline(ifs, line);
    std::getline(ifs, line);
    std::getline(ifs, line);

    auto sumDemand = 0.0;
    while (true)
    {
        int customerId = -1, demand, readyTime, dueDate, serviceTime;
        double xCoord, yCoord;
        ifs >> customerId >> xCoord >> yCoord >> demand >> readyTime >> dueDate >> serviceTime;
        if ((customerId < 0) || ifs.eof() || ifs.bad())
            break;
        if (customerId == 0)
        {
            data.depot_x = xCoord;
            data.depot_y = yCoord;
            data.depot_tw_start = readyTime;
            data.depot_tw_end = dueDate;
        }
        else
        {
            sumDemand += demand;
            data.customers.emplace_back(customerId, demand, xCoord, yCoord, readyTime, dueDate, serviceTime);
        }
    }
    if (data.customers.empty())
        return false;

    data.nbCustomers = data.customers.size() - 1;
    data.minNumVehicles = std::ceil(sumDemand/data.veh_capacity);

    std::cout << "VRPTW data file detected" << std::endl;

    return true;
}

void vrptw::Loader::loadClustering()
{
    clustering.loadClustering(data, parameters);
}

void vrptw::Loader::loadSubtree()
{
    auto file_name = parameters.subTree();
    if (!file_name.empty()) {
        std::ifstream ifs(file_name.c_str(), std::ios::in);
        if (!ifs) {
            std::cerr << "Subtree file reader Error: cannot open file " << file_name << std::endl;
            exit(1);
        }

        if (ifs.eof()) {
            std::cout << "Subtree file reader Error: empty input file " << file_name << std::endl;
            ifs.close();
            exit(1);
        }

        subtree.enabled = true;
        auto success = loadSubtreeFile(ifs);
        ifs.close();
    }
}

bool vrptw::Loader::loadSubtreeFile(std::ifstream &ifs)
{
    std::string line;
    while (std::getline(ifs, line))
    {
        char var[20], oper[3], oper2[2];
        int i, j;
        double rhs;
        Branch::VarType varType;
        Branch::OperType operType;
        bool flag = false;

        if (line.find("x") != std::string::npos)
        {
            if (sscanf(line.c_str(), "%c %d %d %2s %lf", var, &i, &j, oper, &rhs) != 5)
            {
                std::cerr << "Error : cannot read X variable" << std::endl;
                exit(1);
            }
            varType = Branch::VarType::X;
            flag = true;
        }
        if (line.find("w") != std::string::npos)
        {
            if (sscanf(line.c_str(), "%c %d %d %2s %lf", var, &i, &j, oper, &rhs) != 5)
            {
                std::cerr << "Error : cannot read W variable" << std::endl;
                exit(1);
            }
            varType = Branch::VarType::W;
            flag = true;
        }
        if (line.find("z") != std::string::npos)
        {
            if (sscanf(line.c_str(), "%c %d %2s %lf", var, &i, oper, &rhs) != 4)
            {
                std::cerr << "Error : cannot read Z variable" << std::endl;
                exit(1);
            }
            varType = Branch::VarType::Z;
            j = -1;
            flag = true;
        }
        if (line.find("Pack.sets") != std::string::npos)
        {
            if ((sscanf(line.c_str(), "%s %d %d %s", var, &i, &j, oper2) != 4))
            {
                std::cerr << "Error : cannot read R&F packing set ids" << std::endl;
                exit(1);
            }
            varType = Branch::VarType::RF;
            flag = true;
        }

        if (flag)
        {
            if (strcmp(oper, "==") == 0)
                operType = Branch::OperType::EQUAL;
            else if (strcmp(oper, "<=") == 0)
                operType = Branch::OperType::LEQ;
            else if (strcmp(oper, ">=") == 0)
                operType = Branch::OperType::GEQ;
            else if (strcmp(oper, "<") == 0)
                operType = Branch::OperType::LESS;
            else if (strcmp(oper, ">") == 0)
                operType = Branch::OperType::GREATER;
            else if ((varType == Branch::VarType::RF) && (strcmp(oper2, "not") == 0))
                operType = Branch::OperType::RFNOT;
            else if ((varType == Branch::VarType::RF) && (strcmp(oper2, "together") == 0))
                operType = Branch::OperType::RFT;
            else {
                std::cerr << "Error : cannot read subtree operator: " << var << " , " << oper << std::endl;
                exit(1);
            }

            Branch branch;
            branch.varType = varType;
            branch.operType = operType;
            branch.i = i;
            branch.j = j;
            branch.rhs = rhs;
            subtree.branch.push_back(branch);

            //std::cout << "[" << branch.i << "," << branch.j << "]" << oper << branch.rhs << " ";
        }
    }
    ifs.close();

    for (auto & branch : subtree.branch)
    {
        if ((branch.varType == Branch::VarType::X) || (branch.varType == Branch::VarType::RF))
        {
            branch.varIndices.emplace_back(branch.i, branch.j);
        }
        else if (branch.varType == Branch::VarType::W)
        {
            if (clustering.clusters.empty() || clustering.clusters.size() < branch.i
                || clustering.clusters.size() < branch.j)
            {
                std::cerr << "Error subtree: no clusters defined or clusters id do not match!" << std::endl;
                exit(1);
            }

            std::vector<int> cluster1, cluster2;
            for (auto & cluster : clustering.clusters) {
                if (cluster.first == branch.i)
                    cluster1 = cluster.second;
                if (cluster.first == branch.j)
                    cluster2 = cluster.second;
            }

            for (const int & i : cluster1)
            {
                for (const int & j : cluster2)
                {
                    if (i < j)
                        branch.varIndices.emplace_back(i, j);
                    else
                        branch.varIndices.emplace_back(j, i);
                }
            }
        }
        else if (branch.varType == Branch::VarType::Z)
        {
            if (clustering.clusters.empty() || clustering.clusters.size() < branch.i)
            {
                std::cerr << "Error subtree: no clusters defined or clusters id do not match!" << std::endl;
                exit(1);
            }

            std::vector<int> cluster1;
            for (auto & cluster : clustering.clusters) {
                if (cluster.first == branch.i)
                {
                    cluster1 = cluster.second;
                    break;
                }
            }

            for (int i = 0; i <= data.nbCustomers; i++)
            {
                if (clustering.customerInCluster(i, branch.i))
                    continue;

                for (const int & j : cluster1)
                {
                    if (i < j)
                        branch.varIndices.emplace_back(i, j);
                    else
                        branch.varIndices.emplace_back(j, i);
                }
            }
        }
    }
    return true;
}
