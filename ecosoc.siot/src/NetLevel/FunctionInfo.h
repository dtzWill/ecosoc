/*
 * FunctionInfo2.h
 *
 *  Created on: Mar 1, 2014
 *      Author: pablo
 */

#ifndef FUNCTIONINFO2_H_
#define FUNCTIONINFO2_H_

#include "GraphStructure.h"

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>

using namespace std;

class FunctionInfo {
private:
    string functionName;
    void printGraph(ostream &out);
public:
    map<Vertex, FunctionInfo*> usedFunctions;
    set<FunctionInfo*> users;
    bool onCallHierarchy;
	BoostGraph graph;

	// Property accessors
	GraphNames names;
	GraphIndex index;
    GraphCodeLines codeLines;
    GraphCodeNames codeNames;
    GraphColors colors;
	GraphDistances distances;

    const string getFunctionName() {
        return this->functionName;
    }

    void addUsedFunction(Vertex v, FunctionInfo* functionInfo);

    FunctionInfo(string functionName);

    void merging(BoostGraph & g1, Vertex startSource, Vertex endDest, const BoostGraph & g2, Vertex startDestInG2, Vertex endSourceInG2);

    void replaceVertexWithGraph(Vertex vertex, BoostGraph &otherGraph);
    void getGraphEntryAndEndVertices(list<Vertex> &endVerticesOther, list<Vertex> &entryVerticesOther, BoostGraph otherGraph);
    void getVertexInsAndOuts(Vertex vertex, list<Vertex> &inVertices, list<Vertex> &outVertices);
    void writeToDot(ostream &out);
    void writeOnlyNetToDot(ostream &out);
    BoostGraph writeOnlyPatternToDot(ostream &out, const string pattern);
    void printCallStack(ostream &out, unsigned int level, set<FunctionInfo *> &entryFunctions);
    void addUser(FunctionInfo *functionInfo);
};

#endif /* FUNCTIONINFO2_H_ */
