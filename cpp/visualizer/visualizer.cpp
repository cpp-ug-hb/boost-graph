#include <iostream>
#include <fstream>
#include <stack>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

#include <boost/algorithm/string.hpp>


using Vertex_p = boost::property< boost::vertex_name_t, std::string>;
using Edge_p = boost::property< boost::edge_weight_t, unsigned,
  boost::property< boost::edge_name_t, std::string>
>;
using Graph_p = boost::property< boost::graph_name_t, std::string>;

using Graph = boost::adjacency_list<
  boost::vecS,
  boost::vecS,
  boost::undirectedS,
  Vertex_p,
  Edge_p,
  Graph_p
>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;


void parse(Graph& g, const char* filename)
{
  std::ifstream input (filename);
  std::string line;

  std::vector<std::string> items;
  std::vector<Vertex> nodes;

  std::map<std::string, Vertex> node_lookup;

  while (std::getline(input, line)) {
    items.clear();
    nodes.clear();

    boost::algorithm::split(items, line, boost::is_any_of(","), boost::token_compress_on);
    for (auto item : items) {
      boost::algorithm::trim(item);

      auto iter = node_lookup.find(item);
      if (iter == node_lookup.end()) {
        auto v = boost::add_vertex(g);
        put(boost::vertex_name, g, v, item);

        node_lookup[item] = v;
        nodes.push_back(v);
      } else {
        nodes.push_back(iter->second);
      }
    }

    for (int i = 0; i < nodes.size(); ++i) {
      for (int j = i+1; j < nodes.size(); ++j) {

        // returns edge, bool [exists]
        auto edge_p = boost::edge(nodes[i], nodes[j], g);

        if (not edge_p.second) {
          edge_p = boost::add_edge(nodes[i], nodes[j], g);
        }

        auto edge = edge_p.first;

        auto weight = get(boost::edge_weight,g , edge);
        put(boost::edge_weight, g, edge, weight +1);
      }
    }
  }
}


void print_stats(Graph const& g)
{
  BOOST_FOREACH( auto edge, boost::edges(g)) {
    std::cout
        << get(boost::edge_weight, g, edge)
        << " - '"
        << get(boost::vertex_name, g, source(edge, g))
        << "' - '"
        << get(boost::vertex_name, g, target(edge, g))
        << "'\n";
  }
}

void write(Graph const& g, const char* output_file)
{
  std::ofstream output(output_file);
  boost::write_graphviz(
        output,
        g,
        boost::make_label_writer( get(boost::vertex_name, g) ),
        boost::make_label_writer( get(boost::edge_weight, g) )
  );
}



struct min_edge_weight {
  min_edge_weight() { }
  min_edge_weight(const Graph* graph, unsigned min) : m_graph(graph), m_min(min) { }

  template <typename Edge>
  bool operator()(const Edge& e) const {
    return m_min <= get(boost::edge_weight, *m_graph, e);
  }

  bool operator()(const Vertex& v) const {
    BOOST_FOREACH(auto edge, boost::out_edges(v, *m_graph)) {
      if ((*this)(edge)) {
        return true;
      }
    }
    return false;
  }

  const Graph * m_graph = nullptr;
  unsigned m_min = 0;
};



void write(Graph const& g, const char* output_file, unsigned min_weight)
{
  min_edge_weight filter(&g, min_weight);
  boost::filtered_graph<Graph, min_edge_weight, min_edge_weight> filtered(g, filter, filter);


  std::ofstream output(output_file);
  boost::write_graphviz(
        output,
        filtered,
        boost::make_label_writer( get(boost::vertex_name, g) ),
        boost::make_label_writer( get(boost::edge_weight, g) )
  );
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " list.txt" << std::endl;
    return 1;
  }

  Graph g;

  parse(g, argv[1]);

  print_stats(g);

  write(g, "graph.dot");

  write(g, "filtered_2.dot", 2);

  write(g, "filtered_5.dot", 5);

  return 0;
}
