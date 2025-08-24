/*
 *  Dependencies:
 *  - BaPCod v0.82.5
 *  - VRPSolver extension (RCSP solver) v0.6.10
 */


#include "Utils.h"

utils::Path::Path(const vrptw::Data & data, const BcSolution & bcSolution, double varValue, int id) :
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

utils::AuxiliaryFiles::AuxiliaryFiles(const BcSolution & bcSolution_, const vrptw::Data & data_,
                                      const std::string & fileName_, const std::string & type_) :
                                      data(data_), type(type_), fileName(fileName_)
{}

utils::AuxiliaryFiles::AuxiliaryFiles(const std::vector<double> & edgesList, const vrptw::Data & data_,
                                      const std::string & fileName_, const std::string & type_) :
                                      data(data_), type(type_), fileName(fileName_)
{}


bool utils::AuxiliaryFiles::createSolutionTeXfile(const BcSolution & bcSolution)
{
    std::ofstream ofs(fileName.c_str(), std::ofstream::out);
    if (!ofs)
    {
        std::cerr << "Tex file error : cannot open file " << fileName << std::endl;
        return false;
    }

    vrptw::Solution solution(bcSolution);

    // get drawing limits
    auto maxX = std::max_element(data.customers.begin(), data.customers.end(),
                                 [](const vrptw::Customer & a,const vrptw::Customer & b) { return a.x < b.x;});
    auto maxY = std::max_element(data.customers.begin(), data.customers.end(),
                                 [](const vrptw::Customer & a,const vrptw::Customer & b) { return a.y < b.y;});
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

    for (auto & route : solution.routes)
    {
        if (route.vertIds.empty())
            continue;

        auto vertIt = route.vertIds.begin();
        int prevVertId = *vertIt;

        for (++vertIt; vertIt != route.vertIds.end(); ++vertIt)
        {
            bool vertexIsDepot = (*vertIt > data.nbCustomers);
            bool prevVertexIsDepot = (prevVertId == 0) || (prevVertId > data.nbCustomers);
            std::string style = (vertexIsDepot) || (prevVertexIsDepot) ? "dashed,-,line width=0.2pt,opacity=.2" : "-,line width=0.8pt";

            if (vertexIsDepot)
                ofs << "\t\\draw[" << style << "] (v" << prevVertId << ") -- (v" << 0 << ");" << std::endl;
            else
                ofs << "\t\\draw[" << style << "] (v" << prevVertId << ") -- (v" << *vertIt << ");" << std::endl;
            prevVertId = *vertIt;
        }
    }
    ofs << "\\end{tikzpicture}" << std::endl
        << "\\end{document}" << std::endl;

    return true;
}


bool utils::AuxiliaryFiles::createFracPathTeXFile(const std::vector<Path> & paths,
                                                  const std::map< std::pair<int, int>, double> & xVarListValues)
{
    std::ofstream ofs(fileName.c_str(), std::ofstream::out);
    if (!ofs)
    {
        std::cerr << "Tex file error : cannot open file " << fileName << std::endl;
        return false;
    }

    // get drawing limits
    auto maxX = std::max_element(data.customers.begin(), data.customers.end(),
                                 [](const vrptw::Customer & a,const vrptw::Customer & b) { return a.x < b.x;});
    auto maxY = std::max_element(data.customers.begin(), data.customers.end(),
                                 [](const vrptw::Customer & a,const vrptw::Customer & b) { return a.y < b.y;});
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
    for (auto & path : paths)
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

bool utils::AuxiliaryFiles::createCMSTFile(const std::vector<Path> & paths,
                                           const std::map< std::pair<int, int>, double> & xVarListValues)
{
    std::ofstream ofs(fileName.c_str(), std::ofstream::out);
    if (!ofs)
    {
        std::cerr << "CMST file error : cannot open file " << fileName << std::endl;
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
    ofs << paths.size() << std::endl;
    for (auto & path : paths)
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