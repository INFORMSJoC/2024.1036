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
#include "Subtree.h"
#include "Clustering.h"
#include "FractionalClustering.h"
#include "BranchingFeatures.h"

cvrp_joao::Loader::Loader() :
    data(Data::getInstance()), parameters(Parameters::getInstance()), subtree(Subtree::getInstance()),
    clustering(cluster::Clustering::getInstance()), fractionalClustering(fracclu::FractionalClustering::getInstance()),
    branchingFeatures(BranchingFeatures::getInstance())
{}

bool cvrp_joao::Loader::loadParameters(const std::string & file_name, int argc, char* argv[])
{
    return parameters.loadParameters(file_name, argc, argv);
}

bool cvrp_joao::Loader::loadData(const std::string & file_name)
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
    success = loadCVRPFile(ifs);
    ifs.close();

    return success;
}

bool cvrp_joao::Loader::loadCVRPFile(std::ifstream & ifs)
{
    if (parameters.roundDistances()) {
        data.roundType = Data::ROUND_CLOSEST;
    } else {
        data.roundType = Data::NO_ROUND;
    }
    int maxNbVehicles = INT_MAX, capacity = 0, dimension = 0, dimvehicle = 0;
    double distance = INT_MAX, servicetime = 0.0;

    std::string line;
    while (std::getline(ifs, line)) {
        if (parameters.exactNumVehicles() && line.find("NAME") != std::string::npos) {
            char instname;
            int temp;
            if (strncmp(line.c_str(), "NAME", strlen("NAME")) == 0) {
                sscanf(line.c_str(), "NAME : %c-n%d-k%d", &instname, &temp, &dimvehicle);
            }
        }
        if (line.find("DIMENSION") != std::string::npos) {
            if ((sscanf(line.c_str(), "DIMENSION : %d", &dimension) != 1)
                    && (sscanf(line.c_str(), "DIMENSION: %d", &dimension) != 1)) {
                std::cerr << "Error : cannot read DIMENSION" << std::endl;
                exit(1);
            }
            maxNbVehicles = dimension;
        }
        if (line.find("CAPACITY") != std::string::npos) {
            if ((sscanf(line.c_str(), "CAPACITY : %d", &capacity) != 1)
                    && (sscanf(line.c_str(), "CAPACITY: %d", &capacity) != 1)) {
                std::cerr << "Error : cannot read CAPACITY" << std::endl;
                exit(1);
            }
            if (!parameters.dcvrp()) break;
        }
        if ((parameters.dcvrp()) && (line.find("DISTANCE") != std::string::npos)) {
            if ((sscanf(line.c_str(), "DISTANCE : %lf", &distance) != 1)
                && (sscanf(line.c_str(), "DISTANCE: %lf", &distance) != 1)) {
                std::cerr << "Error : cannot read DISTANCE" << std::endl;
                exit(1);
            }
        }
        if ((parameters.dcvrp()) && (line.find("SERVICE_TIME") != std::string::npos)) {
            if ((sscanf(line.c_str(), "SERVICE_TIME : %lf", &servicetime) != 1)
                && (sscanf(line.c_str(), "SERVICE_TIME: %lf", &servicetime) != 1)) {
                std::cerr << "Error : cannot read SERVICE_TIME" << std::endl;
                exit(1);
            }
            break;
        }
    }

    if (capacity == 0) {
        std::cerr << "Instance reader Error : no CAPACITY specified" << std::endl;
        exit(1);
    }

    if (dimension == 0) {
        std::cerr << "Instance reader Error : no DIMENSION specified" << std::endl;
        exit(1);
    }

    if (ifs.eof() || ifs.bad())
        return false;

    std::vector<double> xCoord, yCoord;
    std::vector<int> demand;
    while (std::getline(ifs, line)) {
        if (line.find("NODE_COORD_SECTION") != std::string::npos) {
            while (std::getline(ifs, line)) {
                double x,y; int n;
                if (sscanf(line.c_str(), "%d %lf %lf", &n, &x, &y) == 3) {
                    xCoord.push_back(x);
                    yCoord.push_back(y);
                }
                if (strncmp(line.c_str(), "DEMAND_SECTION", strlen("DEMAND_SECTION")) == 0) {
                    while (std::getline(ifs, line)) {
                        int d;
                        if (sscanf(line.c_str(), "%d %d", &n, &d) == 2)
                            demand.push_back(d);

                        if (strncmp(line.c_str(), "DEPOT_SECTION", strlen("DEPOT_SECTION")) == 0)
                            break;
                    }
                    break;
                }
            }
            break;
        }
    }

    data.veh_capacity = capacity;
    data.minNumVehicles = std::max(dimvehicle, parameters.minNumVehicles());
    data.maxNumVehicles = std::min(maxNbVehicles, parameters.maxNumVehicles());
    data.depot_x = xCoord[0];
    data.depot_y = yCoord[0];

    if (parameters.dcvrp())
    {
        data.serv_time = servicetime;
        data.max_distance = distance;
        std::cout << "[DCVRP] ServiceTime, MaxDistance: " << data.serv_time << ", " << data.max_distance << std::endl;
    }

    int totalDemand = 0;
    for (int i = 1; i < demand.size(); i++)
    {
        data.customers.push_back(Customer(i, demand[i], xCoord[i], yCoord[i]));
        totalDemand += demand[i];
    }

    if (data.minNumVehicles == 0 && totalDemand > 0)
        data.minNumVehicles = (totalDemand - 1) / capacity + 1;

    if (data.customers.empty())
        return false;

    data.nbCustomers = data.customers.size() - 1;

    std::cout << "CVRP data file detected" << std::endl;

    return true;
}

void cvrp_joao::Loader::loadClustering()
{
    clustering.loadClustering(data, parameters);
}

void cvrp_joao::Loader::loadFractionalClustering()
{
    fractionalClustering.loadFractionalClustering(parameters.thresholdValueFracClustering, data.nbCustomers + 1);
}

void cvrp_joao::Loader::loadSubtree()
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
        subtree.enabledAtRoot = false;
        auto success = loadSubtreeFile(ifs, subtree, clustering.clusters);
        ifs.close();
    }

    auto file_name2 = parameters.subTreeAtRoot();
    if (!file_name2.empty()) {
        std::ifstream ifs(file_name2.c_str(), std::ios::in);
        if (!ifs) {
            std::cerr << "Subtree at root file reader Error: cannot open file " << file_name2 << std::endl;
            exit(1);
        }

        if (ifs.eof()) {
            std::cout << "Subtree at root file reader Error: empty input file " << file_name2 << std::endl;
            ifs.close();
            exit(1);
        }
        subtree.enabledAtRoot = true;
        auto success = loadSubtreeFile(ifs, subtree, clustering.clustersAtRoot);
        ifs.close();
    }
}

bool cvrp_joao::Loader::loadSubtreeFile(std::ifstream &ifs, Subtree & subtree_, const std::vector<std::pair<int,std::vector<int>>> & clusters_)
{
    std::string line;
    std::vector<Branch> branchAux;
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

            branchAux.push_back(branch);
            //std::cout << "[" << branch.i << "," << branch.j << "]" << oper << branch.rhs << " ";
        }
    }
    ifs.close();

    for (auto & branch : branchAux)
    {
        if ((branch.varType == Branch::VarType::X) || (branch.varType == Branch::VarType::RF))
        {
            branch.varIndices.emplace_back(branch.i, branch.j);
        }
        else if (branch.varType == Branch::VarType::W)
        {
            if (clusters_.empty() || clusters_.size() < branch.i
                || clusters_.size() < branch.j)
            {
                std::cerr << "Error subtree: no clusters defined or clusters id do not match!" << std::endl;
                exit(1);
            }

            std::vector<int> cluster1, cluster2;
            for (auto & cluster : clusters_) {
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
            if (clusters_.empty() || clusters_.size() < branch.i)
            {
                std::cerr << "Error subtree: no clusters defined or clusters id do not match!" << std::endl;
                exit(1);
            }

            std::vector<int> clusterAux;
            for (auto & cluster : clusters_) {
                if (cluster.first == branch.i)
                {
                    clusterAux = cluster.second;
                    break;
                }
            }

            for (int i = 0; i <= data.nbCustomers; i++)
            {
                auto it = std::find(clusterAux.begin(), clusterAux.end(), i);
                if (it != clusterAux.end())
                    continue;

                for (const int & j : clusterAux)
                {
                    if (i < j)
                        branch.varIndices.emplace_back(i, j);
                    else
                        branch.varIndices.emplace_back(j, i);
                }
            }
        }
    }

    if (subtree_.enabledAtRoot)
        subtree_.branchAtRoot = branchAux;
    else
        subtree_.branch = branchAux;

    return true;
}

void cvrp_joao::Loader::loadBranchingFeatures(int nbSBcandidates)
{
    if (parameters.enableBranchingFeatures())
        branchingFeatures.loadBranchingFeatures(data, nbSBcandidates);
}