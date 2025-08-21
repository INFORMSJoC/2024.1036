
#include "CutSeparation.h"
#include "bcModelNetworkFlow.hpp"
#include "Data.h"

cvrp_joao::Path::Path(const Data & data, const BcSolution & bcSolution, double varValue, int id) :
        id(id), cost(bcSolution.cost()), varPathValue(varValue), vertIds(), vertAcmConsumption()
{
    const BcNetwork network(bcSolution.formulation().network());
    vertIds.reserve(bcSolution.orderedIds().size() + 1);
    vertAcmConsumption.reserve(bcSolution.orderedIds().size() + 1);

    auto arcIdIt = bcSolution.orderedIds().begin();
    int routeCumDemand = 0;
    vertIds.push_back(network.getArc(*arcIdIt).tail().ref());
    vertAcmConsumption.push_back(routeCumDemand);

    std::cout << "Path ";
    std::cout << network.getArc(*arcIdIt).tail().ref();
    while (arcIdIt != bcSolution.orderedIds().end())
    {
        int vertId = network.getArc(*arcIdIt).head().ref();
        vertIds.push_back(vertId);
        std::cout << " -> " << vertId;

        routeCumDemand += data.customers[vertId].demand;
        vertAcmConsumption.push_back(routeCumDemand);
        ++arcIdIt;
    }
    std::cout << " with value " << varValue << std::endl;
}

cvrp_joao::UserNonRobustFunctor::UserNonRobustFunctor(const BcFormulation & formulation, double priority,
                                                      const Data & data_, const Parameters & params_):
        BcCustomNonLinearCutArrayFunctor(formulation, "USR", 'F', SelectionStrategy::MostViolated, priority),
        data(data_), params(params_), cutCount(0), cutRound(0)
{
}

cvrp_joao::UserNonRobustFunctor::~UserNonRobustFunctor() = default;

bool cvrp_joao::UserNonRobustFunctor::createCMSTFile(const std::string & file_name,
                                                     std::vector<utils::Path> solution,
                                                     std::map< std::pair<int, int>, double> & xVarListValues)
{
    std::ofstream ofs(file_name.c_str(), std::ofstream::out);
    if (!ofs)
    {
        std::cerr << "CMST file error : cannot open file " << file_name << std::endl;
        return false;
    }

    ofs << "CMST" << std::endl;
    ofs << data.nbCustomers + 1 << std::endl;
    ofs << data.depot_x << "  " << data.depot_y << std::endl;
    for (int i = 1; i <= data.nbCustomers; i++)
    {
        ofs << data.customers[i].x << "  " << data.customers[i].y << std::endl;
    }

    ofs << xVarListValues.size() << std::endl;
    for (auto & pair: xVarListValues) {
        ofs << pair.first.first << "  " << pair.first.second << "  " << pair.second << std::endl;
    }

    ofs << "BEGINTREE" << std::endl;
    ofs << solution.size() << std::endl;
    for (auto & path : solution)
    {
        ofs << path.id << " " << path.varPathValue << std::endl;
        for (int custId = path.vertIds.size() - 2; custId >= 0; custId--)
        {
            ofs << "(" << path.vertIds[custId] << "," << path.vertAcmConsumption[custId] << ") ";
        }
        for (int custId = path.vertIds.size() - 2; custId >= 0; custId--)
        {
            ofs << "END ";
        }
        ofs << "END " << std::endl;
    }
    ofs << "ENDTREE" << std::endl << std::endl;

    ofs << "BEGINDEMAND" << std::endl;
    ofs << "0  0" << std::endl;
    for (int custId = 1; custId <= data.nbCustomers; custId++)
    {
        ofs << data.customers[custId].id << "  " << data.customers[custId].demand << std::endl;
    }
    ofs << "ENDDEMAND" << std::endl << std::endl;

    ofs << "ROOT 0" << std::endl;

    ofs << "BEGINEDGECOSTS" << std::endl;
//    ofs << ((data.nbCustomers + 1) * data.nbCustomers)/2 << std::endl;
//    for (int firstCustId = 0; firstCustId <= data.nbCustomers; ++firstCustId )
//        for (int secondCustId = firstCustId + 1; secondCustId <= data.nbCustomers; ++secondCustId)
//        {
//            ofs << firstCustId << " " << secondCustId << " "
//                << ((firstCustId == 0) ? data.getDepotToCustDistance(secondCustId)
//                                       : data.getCustToCustDistance(firstCustId, secondCustId)) << std::endl;
//        }
    ofs << xVarListValues.size() << std::endl;
    for (auto & pair: xVarListValues) {
        int firstCustId = pair.first.first, secondCustId = pair.first.second;
        ofs << firstCustId << "  " << secondCustId << "  "
            << ((firstCustId == 0) ? data.getDepotToCustDistance(secondCustId)
                                   : data.getCustToCustDistance(firstCustId, secondCustId)) << std::endl;
    }
    ofs << "ENDEDGECOSTS" << std::endl << std::endl;

    ofs << "CAPACITY " << data.veh_capacity;
    return true;
}

bool cvrp_joao::UserNonRobustFunctor::createTeXFile(const std::string & file_name,
                                                     std::vector<utils::Path> solution,
                                                     std::map< std::pair<int, int>, double> & xVarListValues)
{
    std::ofstream ofs(file_name.c_str(), std::ofstream::out);
    if (!ofs)
    {
        std::cerr << "CMST file error : cannot open file " << file_name << std::endl;
        return false;
    }

    // get drawing limits
    auto maxX = std::max_element(data.customers.begin(), data.customers.end(),
                                 [](const Customer & a,const Customer & b) { return a.x < b.x;});
    auto maxY = std::max_element(data.customers.begin(), data.customers.end(),
                                 [](const Customer & a,const Customer & b) { return a.y < b.y;});
    auto scaleFactor = 1 / (std::max(maxX->x, maxY->y) / 10);

    // LaTeX code
    ofs << "\\documentclass[crop,tikz]{standalone}" << std::endl
        << "\\begin{document}" << std::endl
        << "\\usetikzlibrary{shapes,backgrounds,calc,quotes}" << std::endl;

    ofs << "\\begin{tikzpicture}[thick, scale=1, every node/.style={scale=0.3}]" << std::endl;
    ofs << "\t\\node[draw, line width=0.1mm, rectangle, fill=yellow, inner sep=0.05cm, scale=1.4] (v0) at "
        << "(" << scaleFactor * data.depot_x << "," << scaleFactor * data.depot_y << ") {\\footnotesize 0};" << std::endl;
    for (int i = 1; i <= data.nbCustomers; i++)
    {
        ofs << data.customers[i].x << "  " << data.customers[i].y << std::endl;
        ofs << "\t\\node[draw, line width=0.1mm, circle, fill=white, inner sep=0.05cm, circle split] (v"
            << data.customers[i].id << ") at (" << scaleFactor * data.customers[i].x << ","
            << scaleFactor * data.customers[i].y << ") {\\small " << data.customers[i].id
            << " \\nodepart{lower} \\small " << data.customers[i].demand << "};" << std::endl;
    }

    std::default_random_engine defEng;
    std::uniform_int_distribution<int> inRange(20, 250);
    for (auto & path : solution)
    {
        int prevNode = 0;
        int R = inRange(defEng), G = inRange(defEng), B = inRange(defEng);
        for (int custId = path.vertIds.size() - 2; custId >= 0; custId--)
        {
            ofs << "\t\\draw[-,line width=0.8pt, color={rgb:red," << R << ";green," << G << ";blue," << B
                << "}] (v" << prevNode << ") edge [\"" << path.varPathValue << "\"] (v" << path.vertIds[custId] << ");" << std::endl;
            prevNode = path.vertIds[custId];
        }
    }
    ofs << "\\end{tikzpicture}" << std::endl
        << "\\end{document}" << std::endl;

    return true;
}

int cvrp_joao::UserNonRobustFunctor::cutSeparationRoutine(BcFormulation formPtr,
                                                          BcSolution & projectedSol,
                                                          std::list<std::pair<double, BcSolution>> & columnsInSol,
                                                          const double & violationTolerance,
                                                          std::list<BcCustomNonLinearCut> & cutList)
{
    int n = data.nbCustomers + 1;
    std::map< std::pair<int, int>, double> xVarListValues;
    std::vector<utils::Path> paths;
    int pathId = 0;

    for (auto & pair : columnsInSol)
    {
        auto value = pair.first;
        auto & solution = pair.second;
        auto & arcIds = solution.orderedIds();

        if (arcIds.empty())
        {
            continue;
        }
        else
        {
            paths.emplace_back(data, solution, value, pathId++);
        }

        //  std::cout << "Fractional solution for the cut separation:" << std::endl;
        const std::set< BcVar > & xVarList = solution.extractVar("X");
        for (std::set< BcVar >::const_iterator varIt = xVarList.begin(); varIt != xVarList.end(); ++varIt)
        {
            if (xVarListValues.count(std::make_pair((*varIt).id().first(), (*varIt).id().second())) > 0)
            {
                xVarListValues[std::make_pair((*varIt).id().first(), (*varIt).id().second())] += value; // (*varIt).solVal();
            }
            else
            {
                xVarListValues[std::make_pair((*varIt).id().first(), (*varIt).id().second())] = value; // (*varIt).solVal();
            }
            // std::cout << "Var X[" << (*varIt).id().first() << "," << (*varIt).id().second() << "] = " << xVarListValues[std::make_pair(i,j)]  << ", ";
        }
        // std::cout << std::endl;
    }

    auto success = createCMSTFile("CMSTfile.txt", paths, xVarListValues);
    success = createTeXFile("TeXfile.tex", paths, xVarListValues);

    return 0;
}
