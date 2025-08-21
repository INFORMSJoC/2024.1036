#ifndef BRANCH_SCORE
#define BRANCH_SCORE

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>

using namespace std;

class dotFileNode{
public:
    int id;
    int fatherId;
    int level;
    double LB;
    double UB;
    double score;
    vector<int> children;

    dotFileNode() : id(-1), fatherId(-1), level(0), LB(0), UB(1e8), score(-1), children() {};
};

class BranchScore
{
    vector<dotFileNode> nodes;
    string fileName;

public:
    BranchScore(const string & baPTreeDotFile, const int nbTreeNodes) : nodes(), fileName(baPTreeDotFile)
    {
        includeScoreToBaPTreeDotFile(baPTreeDotFile, nbTreeNodes);
    }

private:
    void includeScoreToBaPTreeDotFile(const string & baPTreeDotFile, const int nbTreeNodes){
        ifstream infile(fileName.c_str(), ios::in);
        if (!infile)
        {
            cout << "DotFile reader error : cannot open file! " << fileName << endl;
            exit(0);
        }

        if (infile.eof())
        {
            cout << "DotFile reader error : empty input file! " << fileName << endl;
            infile.close();
            exit(0);
        }

        nodes = vector<dotFileNode> (nbTreeNodes + 1, dotFileNode());
        auto success = calculateBranchScore(infile);

        // Return to the beginning of the file
        infile.clear(); // Clear any error flags
        infile.seekg(0, ios::beg); // Set the file position to the beginning

        if (success)
            success = updateBaPTreeDotFile(infile);
        else
            cout << "Branch score error : cannot calculate the branch score! " << fileName << endl;

        if (success)
            cout << "DotFile updated : branch score included in BaPdotfile! " << fileName << endl;
        else
            cout << "DotFile not updated : branch score not included in BaPdotfile! " << fileName << endl;
    }

    bool calculateBranchScore(ifstream & infile) {
        double BKS = 1e8;
        string line;
        // Define a regular expression pattern to match "nX" where X is any number
        regex pattern1("n(\\d+)");
        // Define a regular expression pattern to match two decimal numbers within brackets
        regex pattern2("\\[(\\d+\\.\\d+),\\s*(\\d+\\.\\d+)\\]");
		regex pattern3("BOUND \\[(\\d+\\.\\d+)\\]");
        // Get nodes info
        while (getline(infile, line))
        {
            // Search for the patterns 'n[number]' in the input string
            sregex_iterator it(line.begin(), line.end(), pattern1);
            sregex_iterator end;
            vector<int> ids; // Vector to save the nodes ids
            if (it != end) {
                while (it != end) {
                    smatch match = *it;
                    ids.push_back(stoi(match[1]));
                    ++it;
                }
            }
            // else {
            //   cout << "Pattern not found in the input string." << endl;
            // }

            if (ids.size() == 1) { // dot file line of nodes code
                nodes[ids[0]].id = ids[0];

                // Get node bounds
                smatch match;
                if (regex_search(line, match, pattern2)) {
                    if (match.size() >= 2) {
                        // The first captured number will be in match[1]
                        string number1Str = match[1];
                        nodes[ids[0]].LB = stod(number1Str); // node LB

                        if (match.size() >= 3) {
                            // The second captured number (if present) will be in match[2]
                            string number2Str = match[2];
                            nodes[ids[0]].UB = stod(number2Str); // node UB
                        } else {
                            nodes[ids[0]].UB = nodes[ids[0]].LB;
                        }

                        if (ids[0] == 1) // If root node [id = 1]: UB = BKS
                            BKS = nodes[ids[0]].UB;
                    }
                } else if (regex_search(line, match, pattern3)) {
					string number2Str = match[1];
                    nodes[ids[0]].LB = nodes[ids[0]].UB = stod(number2Str); // node UB

					if (ids[0] == 1) // If root node [id = 1]: UB = BKS
                        BKS = nodes[ids[0]].UB;
				}
				// else {
                //    cout << "Pattern not found in the input string." << endl;
                // }
            } else if (ids.size() == 2){ // dot file line of edges code
                nodes[ids[0]].children.push_back(ids[1]); // ids[0] is the father node
                nodes[ids[1]].level = nodes[ids[0]].level + 1; // child level equal to the father level + 1
                nodes[ids[1]].fatherId = ids[0]; // ids[1] is the child node
            }
        }

        int nodesWithNoScoreCounter = 0;
        double epsilon = 0.0;
        for (auto & node : nodes) {
            if (node.children.size() == 2){
                auto ch1 = node.children[0], ch2 = node.children[1];
                auto scoreCh1 = max(((nodes[ch1].LB - node.LB) / (BKS - nodes[1].LB)) * 100, epsilon);
                auto scoreCh2 = max(((nodes[ch2].LB - node.LB) / (BKS - nodes[1].LB)) * 100, epsilon);
                node.score = sqrt(scoreCh1 * scoreCh2);
            } else {
                nodesWithNoScoreCounter++;
            }
        }
        return true;
    }

    bool updateBaPTreeDotFile(ifstream & infile){
        // Define a regular expression pattern to match "nX" where X is any number
        regex pattern1("n(\\d+)");

        stringstream os;
        string line;
        while (getline(infile, line))
        {
            // Search for the patterns 'n[number]' in the input string
            sregex_iterator it(line.begin(), line.end(), pattern1);
            sregex_iterator end;
            vector<int> ids; // Vector to save the nodes ids
            while (it != end) {
                smatch match = *it;
                ids.push_back(stoi(match[1]));
                ++it;
            }
            if ((ids.size() == 1) && (nodes[ids[0]].children.size() == 2)) {
                ostringstream score;
                score << "{" << fixed << setprecision(2) << nodes[ids[0]].score << "} \\n[";
                // Replace '\n' with '<Score> \n'
                line = std::regex_replace(line, std::regex(R"(\\n\[)"), score.str());
            }
            os << line << endl;
        }
        infile.close();

        size_t dotPos = fileName.find_last_of('.');
        std::string newFileName;
        if (dotPos != std::string::npos) {
            std::string nameWithoutExt = fileName.substr(0, dotPos);
            std::string extension = fileName.substr(dotPos);
            newFileName = nameWithoutExt + "_score" + extension;
        } else {
            // No extension found, just append "_score"
            newFileName = fileName + "_score";
        }

        ofstream outfile(newFileName.c_str());
        outfile << os.str();
        outfile.close();

        return true;
    }
};

#endif
