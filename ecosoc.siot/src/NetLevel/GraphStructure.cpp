#include "GraphStructure.h"

unsigned int GraphStructure::vertexCount = 0;

GraphStructure::GraphStructure() {
	// TODO Auto-generated constructor stub

}

GraphStructure::~GraphStructure() {
	// TODO Auto-generated destructor stub
}

void GraphStructure::removeVertexFromGraph(BoostGraph &graph, Vertex vertex) {
//    GraphNames names = get(boost::vertex_name, graph);
    BoostGraph::inv_adjacency_iterator invAdjIt, invAdjEnd;
    tie(invAdjIt, invAdjEnd) = boost::inv_adjacent_vertices(vertex, graph);
    BoostGraph::adjacency_iterator adjIt, adjEnd;
    list < pair <Vertex, Vertex> > newEdges;
    for (; invAdjIt != invAdjEnd; ++invAdjIt) {
        tie(adjIt, adjEnd) = boost::adjacent_vertices(vertex, graph);
        for (; adjIt != adjEnd; ++adjIt) {
//            cerr << "iv=" << *invAdjIt << " " << names[*invAdjIt] << " av=" << *adjIt << " " << names[*adjIt] << "\n";
            newEdges.push_back( pair<Vertex, Vertex>(*invAdjIt, *adjIt));
        }
    }

    for (list<pair<Vertex, Vertex> >::iterator newEdge = newEdges.begin(); newEdge != newEdges.end(); ++newEdge) {
        add_edge(newEdge->first, newEdge->second, graph);
//        distances[e1] = 1;
    }

    clear_vertex(vertex, graph);
    remove_vertex(vertex, graph);
}

Vertex GraphStructure::createVertex(BoostGraph &graph, string name, boost::default_color_type color, VertexExtraData *extraData) {
    Vertex vertex = add_vertex(graph);
    stringstream ss;
    ss << name;
    if(extraData != NULL) {
        ss << "<br/>" << extraData->codeName << ":" << extraData->codeLine;
        get(boost::vertex_code_name, graph)[vertex] = extraData->codeName;
        get(boost::vertex_code_line, graph)[vertex] = extraData->codeLine;
    }
    get(boost::vertex_name, graph)[vertex] = ss.str();
    //get(boost::vertex_index, graph)[vertex] = GraphStructure::genVertexIndex(graph);
    get(boost::vertex_color, graph)[vertex] = color;
    //cerr << "CFG|" << ss.str() << "\n";
    return vertex;
}

unsigned int GraphStructure::genVertexIndex(BoostGraph &graph){
    return vertexCount++;
}

void GraphStructure::createLevelsForGraph(BoostGraph &graph, vector<set<Vertex> > &levelSets) {
    BoostGraph::vertex_iterator vertexIt, vertexEnd;
    boost::tuples::tie(vertexIt, vertexEnd) = vertices(graph);
    for (; vertexIt != vertexEnd; ++vertexIt) {
        //cerr << "vert " << *vertexIt << "\n";
        BoostGraph::inv_adjacency_iterator invAdjIt, invAdjEnd;
        tie(invAdjIt, invAdjEnd) = boost::inv_adjacent_vertices(*vertexIt, graph);
        //cerr << "size " << distance(invAdjIt, invAdjEnd) << "\n";
        if (distance(invAdjIt, invAdjEnd) <= 1) {
            bool onlySelfEdge = true;
            for(;invAdjIt != invAdjEnd;++invAdjIt) {
                if(*vertexIt != *invAdjIt) {
                    onlySelfEdge = false;
                    break;
                }
            }
            if(onlySelfEdge) {
                if (levelSets.size() > 0) {
                    //cerr << "maior q zero\n";
                    levelSets[0].insert(*vertexIt);
                } else {
                    set<Vertex> tempSet;
                    tempSet.insert(*vertexIt);
                    levelSets.push_back(tempSet);
                }
            }
        }
    }
    for (int level = 0;level<levelSets.size(); ++level) {
//        cerr << "setsize "
//               << distance(levelSets[level].begin(),
//                           levelSets[level].end()) << "\n";
        set<Vertex> tempSet;
        for (set<Vertex>::iterator it = levelSets[level].begin();
             it != levelSets[level].end(); ++it) {
            cerr << "  vert " << *it << "\n";
            BoostGraph::adjacency_iterator adjIt, adjEnd;
            tie(adjIt, adjEnd) = adjacent_vertices(*it, graph);
            for (; adjIt != adjEnd; ++adjIt) {
                cerr << "    nivel " << (level + 1) << " " << *adjIt << "\n";
                tempSet.insert(*adjIt);
            }
        }
        if (tempSet.size() > 0) {
            vector<set<Vertex> >::iterator aSet = find(levelSets.begin(),
                                                       levelSets.end(), tempSet);
            if (aSet == levelSets.end()) {
//                cerr << "nao tem" << "\n";
                levelSets.push_back(tempSet);
            } else {
//                cerr << "ja tem" << "\n";
                break;
            }
        } else {
            break;
        }
    }
    int level = 0;
    for (vector<set<Vertex> >::iterator it = levelSets.begin();
         it != levelSets.end(); ++it) {
        cerr << "\nlevel=" << level << " , itens: ";
        for (set<Vertex>::iterator vertIt = levelSets[level].begin();
             vertIt != levelSets[level].end(); ++vertIt) {
            cerr << *vertIt << " ";
        }
        ++level;
    }
}

