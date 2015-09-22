/*
 * GraphStructure.h
 *
 *  Created on: Mar 1, 2014
 *      Author: pablo
 */

#ifndef GRAPHSTRUCTURE_H_
#define GRAPHSTRUCTURE_H_

#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <list>

using namespace std;

namespace boost {
enum vertex_code_line_t { vertex_code_line = 128 };
// a unique #
BOOST_INSTALL_PROPERTY(vertex, code_line);
enum vertex_code_name_t { vertex_code_name = 129 };
// a unique #
BOOST_INSTALL_PROPERTY(vertex, code_name);
}

struct VertexExtraData {
    int codeLine;
    string codeName;
    set<int> *levels;
};

// Property types
typedef boost::property<boost::edge_weight_t, int> EdgeWeightProperty;
typedef boost::property<boost::vertex_name_t, std::string,
boost::property<boost::vertex_index_t, int,
boost::property<boost::vertex_code_line_t, int,
boost::property<boost::vertex_code_name_t, string,
boost::property<boost::vertex_color_t, boost::default_color_type> > > > > VertexProperties;

// Graph type
typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS, VertexProperties,
EdgeWeightProperty> BoostGraph;

// Create the vertices
typedef boost::graph_traits<BoostGraph>::vertex_descriptor Vertex;

//typedef pair<int,int> Edge;
typedef boost::graph_traits<BoostGraph>::edge_descriptor Edge;

typedef boost::property_map<BoostGraph, boost::vertex_name_t>::type GraphNames;
typedef boost::property_map<BoostGraph, boost::vertex_index_t>::type GraphIndex;
typedef boost::property_map<BoostGraph, boost::vertex_code_line_t>::type GraphCodeLines;
typedef boost::property_map<BoostGraph, boost::vertex_code_name_t>::type GraphCodeNames;
typedef boost::property_map<BoostGraph, boost::vertex_color_t>::type GraphColors;
typedef boost::property_map<BoostGraph, boost::edge_weight_t>::type GraphDistances;

class GraphStructure {
    static unsigned int vertexCount;

public:
    GraphStructure();
    virtual ~GraphStructure();

    static void removeVertexFromGraph(BoostGraph &graph, Vertex vertex);

    static Vertex createVertex(BoostGraph &graph, string name, boost::default_color_type color,
                               VertexExtraData *extraData = NULL);

    static unsigned int genVertexIndex(BoostGraph &graph);

    static void createLevelsForGraph(BoostGraph& graph, vector<set<Vertex> >& levelSets);

};

template<class Color, class Label, class Index>
class properties_writer {
public:
    properties_writer(Color _color, Label _label, Index _index) :
        color(_color), label(_label), index(_index) {
    }
    template<class VertexOrEdge>
    void operator()(std::ostream& out, const VertexOrEdge& v) const {
        static const char* color_names[] = { "white", "gray", "green", "red",
                                             "black" };
        const int colors = sizeof(color_names) / sizeof(color_names[0]);
        out << " [ label=<["<< get(index, v) << "]" << "[" << v << "]" << get(label, v) << "> ]";
        if (get(color, v) < colors) {
            out << " [ style=\"filled\", fillcolor=\""
                << color_names[get(color, v)] << "\" ]";
        }
    }
private:
    Color color;
    Label label;
    Index index;
};
template<class Color, class Label, class Index>
inline properties_writer<Color, Label, Index> make_properties_writer(Color n,
                                                                     Label l, Index i) {
    return properties_writer<Color, Label, Index>(n, l, i);
}



#endif /* GRAPHSTRUCTURE_H_ */
