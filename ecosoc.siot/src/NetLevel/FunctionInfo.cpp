#include "FunctionInfo.h"

#include <boost/graph/graph_utility.hpp>
#include <boost/property_map/property_map.hpp>

#include "llvm/ADT/StringRef.h"

void FunctionInfo::addUsedFunction(Vertex v, FunctionInfo *functionInfo) {
    //cerr << "FunctionInfo ADD " << v << " in " << this->functionName << " call " << functionInfo->getFunctionName() << "\n";
    usedFunctions.insert(pair<Vertex, FunctionInfo*>(v, functionInfo));
}

void FunctionInfo::addUser(FunctionInfo *functionInfo) {
    //cerr << "FunctionInfo ADD USER" << v << " in " << this->functionName << " called by " << functionInfo->getFunctionName() << "\n";
    users.insert(functionInfo);
}

FunctionInfo::FunctionInfo(string functionName) : functionName(functionName), onCallHierarchy(false) {
    names = get(boost::vertex_name, graph);
    index = get(boost::vertex_index, graph);
    colors = get(boost::vertex_color, graph);
    codeLines = get(boost::vertex_code_line, graph);
    codeNames = get(boost::vertex_code_name, graph);
    distances = get(boost::edge_weight, graph);
}

void FunctionInfo::merging(BoostGraph &g1, Vertex startSource, Vertex endDest, const BoostGraph &g2, Vertex startDestInG2, Vertex endSourceInG2)
{
    //for simple adjacency_list<> this type would be more efficient:
    typedef boost::iterator_property_map<typename std::vector<Vertex>::iterator,
            GraphIndex,Vertex,Vertex&> IsoMap;

    //for more generic graphs, one can try  //typedef std::map<vertex_t, vertex_t> IsoMap;
    IsoMap mapV;
    boost::copy_graph( g2, g1, boost::orig_to_copy(mapV) ); //means g1 += g2

    // TODO refatorar de modo a ligar todos os nos iniciais do g2 com o nó fonte g1, e todos os nós finais de g2 com o nó destino G1

    Vertex startDest = mapV[startDestInG2];
    boost::add_edge(startSource, startDest, g1);

    Vertex endSource = mapV[endSourceInG2];
    boost::add_edge(endSource, endDest, g1);
}

void FunctionInfo::getGraphEntryAndEndVertices(list<Vertex> &endVerticesOther, list<Vertex> &entryVerticesOther, BoostGraph otherGraph)
{
    BoostGraph::vertex_iterator vertexIt, vertexEnd;
    boost::tuples::tie(vertexIt, vertexEnd) = boost::vertices(otherGraph);
    for (; vertexIt != vertexEnd; ++vertexIt) {
        BoostGraph::inv_adjacency_iterator invAdjIt, invAdjEnd;
        tie(invAdjIt, invAdjEnd) = boost::inv_adjacent_vertices(*vertexIt, otherGraph);
        // its an entry vertex
        if (distance(invAdjIt, invAdjEnd) <= 1) {
            bool onlySelfEdge = true;
            for(;invAdjIt != invAdjEnd;++invAdjIt) {
                if(*vertexIt != *invAdjIt) {
                    onlySelfEdge = false;
                    break;
                }
            }
            if(onlySelfEdge) {
                //cerr << "FunctionInfo GRAPH_ENTRY " << index[*vertexIt] << "\n";
                entryVerticesOther.push_back(*vertexIt);
            }
        }
        BoostGraph::adjacency_iterator adjIt, adjEnd;
        boost::tuples::tie(adjIt, adjEnd) = boost::adjacent_vertices(*vertexIt, otherGraph);
        // its an end vertex
        if (distance(adjIt, adjEnd) == 0) {
            //cerr << "FunctionInfo GRAPH_END " << index[*vertexIt] << "\n";
            endVerticesOther.push_back(*vertexIt);
        }
    }
}

void FunctionInfo::getVertexInsAndOuts(Vertex vertex, list<Vertex> &inVertices, list<Vertex> &outVertices)
{
    BoostGraph::inv_adjacency_iterator invAdjIt, invAdjEnd;
    tie(invAdjIt, invAdjEnd) = boost::inv_adjacent_vertices(vertex, graph);
    // its an entry vertex
    for (; invAdjIt != invAdjEnd; ++invAdjIt) {
        //cerr << "FunctionInfo VERTEX_IN " << index[*invAdjIt] << "\n";
        inVertices.push_back(*invAdjIt);
    }
    BoostGraph::adjacency_iterator adjIt, adjEnd;
    tie(adjIt, adjEnd) = boost::adjacent_vertices(vertex, graph);
    // its an end vertex
    for (; adjIt != adjEnd; ++adjIt) {
        //cerr << "FunctionInfo VERTEX_OUT " << index[*adjIt] << "\n";
        outVertices.push_back(*adjIt);
    }
}

void FunctionInfo::writeToDot(ostream &out)
{
    boost::write_graphviz(out, graph, make_properties_writer(colors, names, index), boost::default_writer());
}

void FunctionInfo::writeOnlyNetToDot(ostream &out)
{
    BoostGraph graph = this->graph;
    GraphNames names = get(boost::vertex_name, graph);
    GraphIndex index = get(boost::vertex_index, graph);
    GraphColors colors = get(boost::vertex_color, graph);

    list<Vertex> toRemove;
    BoostGraph::vertex_iterator it, iend;
    for (boost::tuples::tie(it, iend) = boost::vertices(graph); it != iend; ++it) {
        if(colors[*it] != boost::green_color) {
            //cerr << "ins it=" << *it << endl;
            toRemove.push_back(*it);
        }
    }

    list<Vertex> removed;
    for(list<Vertex>::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
        int subtract = 0;
        for(list<Vertex>::iterator removedIt = removed.begin(); removedIt != removed.end(); ++removedIt) {
            if(*it > *removedIt){
                ++subtract;
            }
        }
        //cerr << functionName << " rem it=" << (*it)-subtract << " " << names[(*it)-subtract] << endl;
        GraphStructure::removeVertexFromGraph(graph, (*it)-subtract);

        removed.push_back(*it);

        //boost::write_graphviz(cerr, graph, make_properties_writer(colors, names, index), boost::default_writer());
    }

    if(boost::num_vertices(graph)>0) {
        boost::write_graphviz(out, graph, make_properties_writer(colors, names, index), boost::default_writer());
    }
}

BoostGraph FunctionInfo::writeOnlyPatternToDot(ostream &out, const string pattern)
{
    BoostGraph graph = this->graph;
    GraphNames names = get(boost::vertex_name, graph);
    GraphIndex index = get(boost::vertex_index, graph);
    GraphColors colors = get(boost::vertex_color, graph);

    list<Vertex> toRemove;
    BoostGraph::vertex_iterator it, iend;
    for (boost::tuples::tie(it, iend) = boost::vertices(graph); it != iend; ++it) {
        if (names[*it].find(pattern) == llvm::StringRef::npos) {
            //cerr << "ins pt=" << *it << endl;
            toRemove.push_back(*it);
        }
    }

    list<Vertex> removed;
    for(list<Vertex>::reverse_iterator it = toRemove.rbegin(); it != toRemove.rend(); ++it) {
        int subtract = 0;
        for(list<Vertex>::iterator removedIt = removed.begin(); removedIt != removed.end(); ++removedIt) {
            if(*it > *removedIt){
                ++subtract;
            }
        }
        //cerr << "rem pt=" << (*it)-subtract << " " << names[(*it)-subtract] << endl;
        GraphStructure::removeVertexFromGraph(graph, (*it)-subtract);

        removed.push_back(*it);

        //boost::write_graphviz(cerr, graph, make_properties_writer(colors, names, index), boost::default_writer());
    }

    if(boost::num_vertices(graph)>0) {
        boost::write_graphviz(out, graph, make_properties_writer(colors, names, index), boost::default_writer());
    }
    return graph;
}

void FunctionInfo::replaceVertexWithGraph(Vertex vertex, BoostGraph &otherGraph) {
    cerr << "FunctionInfo REPLACE WITH GRAPH " << vertex << " in " << getFunctionName() << " call " << names[vertex] << "\n";

    BoostGraph other = otherGraph;

    typedef boost::iterator_property_map<typename std::vector<Vertex>::iterator,
            GraphIndex,Vertex,Vertex&> IsoMap;
    std::vector<Vertex> iso_vec(boost::num_vertices(other));
    IsoMap mapV(iso_vec.begin(), get(boost::vertex_index, other));
    boost::copy_graph( other, graph, orig_to_copy(mapV)); //means g1 += g2

    list<Vertex> entryVerticesOther;
    list<Vertex> endVerticesOther;
    getGraphEntryAndEndVertices(endVerticesOther, entryVerticesOther, other);

    list<Vertex> inVertices;
    list<Vertex> outVertices;
    getVertexInsAndOuts(vertex, inVertices, outVertices);


    for(list<Vertex>::iterator source = inVertices.begin(); source != inVertices.end(); ++source) {
        for(list<Vertex>::iterator target = entryVerticesOther.begin(); target != entryVerticesOther.end(); ++target) {
            //cerr << "FunctionInfo in EDGE " << *source << "->" << mapV[*target]  << "(ex-" << *target << ")" << "\n";
            add_edge(*source, mapV[*target], graph);
        }
    }
    for(list<Vertex>::iterator source = endVerticesOther.begin(); source != endVerticesOther.end(); ++source) {
        for(list<Vertex>::iterator target = outVertices.begin(); target != outVertices.end(); ++target) {
            //cerr << "FunctionInfo out EDGE " << mapV[*source] << "(ex-" << *source << ")" << "->" <<  *target<< "\n";
            add_edge(mapV[*source], *target, graph);
        }
    }

    //cerr << "FunctionInfo APAGA " << vertex << "\n";
    boost::clear_vertex(vertex, graph);
    boost::remove_vertex(vertex, graph);

//    list<Vertex> users;
    // percorre lista com todos os vertices que representam funcoes
//    for (map<Vertex, FunctionInfo* >::iterator it = this->functionsUsers.begin();
//         it != this->functionsUsers.end(); ++it) {
//        // verifica se o vertice tem indice maior do que o que foi removido
//        Vertex v = (*it).first;
//        if(v > vertex) {
//            // se tiver, remove da lista e insere novo vertice
//            addFunctionUser(v-1, (*it).second);
//            users.push_back(vertex);
//            users.remove(vertex-1);
//        }
//    }
//    for(list<Vertex>::iterator it = users.begin(); it != users.end(); ++it) {
//        this->functionsUsers.erase((Vertex)*it);
//    }

//    int initialNumVertices = 0;
//    boost::graph_traits<Graph>::vertex_iterator it, iend;
//    boost::property_map<Graph, boost::vertex_index_t>::type indexmap = boost::get(boost::vertex_index, graph);
//    for (boost::tuples::tie(it, iend) = boost::vertices(graph); it != iend; ++it, ++initialNumVertices) {
//        indexmap[*it] = initialNumVertices;
//    }
    //writeToDot(std::cerr);
    //printGraph(std::cerr);
}

void FunctionInfo::printGraph(ostream &out) {

    out << "digraph G {\n";
    BoostGraph::vertex_iterator vertexIt, vertexEnd;
    boost::tuples::tie(vertexIt, vertexEnd) = boost::vertices(graph);
    static const char* color_names[] = { "white", "gray", "green", "red", "black" };
    for (; vertexIt != vertexEnd; ++vertexIt) {
        unsigned int x;
        std::stringstream ss2;
        ss2 << std::hex << *vertexIt;
        ss2 >> x;
        out << x << " [ label=<" << names[*vertexIt] << "> ]";
        out << " [ style=\"filled\", fillcolor=\"" << color_names[colors[*vertexIt] ] << "\" ];\n";
    }

    typedef boost::graph_traits<BoostGraph>::edge_iterator edge_iter;
    std::pair<edge_iter, edge_iter> es = boost::edges(graph);
    for (edge_iter eit = es.first; eit != es.second; ++eit) {
        unsigned int s, t;
        stringstream src;
        stringstream tgt;
        src << std::hex << boost::source(*eit, graph);
        tgt << std::hex << boost::target(*eit, graph);
        src >> s;
        tgt >> t;
        out << s << "->" << t << ";\n";
    }

    out << "}\n";
}

void FunctionInfo::printCallStack(ostream &out, unsigned int level, set<FunctionInfo*> &entryFunctions) {
    this->onCallHierarchy = true;
    out << std::string(level*4, '-') << getFunctionName() << endl;
    if(this->users.empty()) {
        entryFunctions.insert(this);
        return;
    }
    for (set<FunctionInfo* >::iterator it = this->users.begin();
         it != this->users.end(); ++it) {
        FunctionInfo* fi = (*it);
        fi->printCallStack(out, level+1, entryFunctions);
    }

}
