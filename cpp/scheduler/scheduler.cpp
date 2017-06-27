#include <iostream>
#include <fstream>
#include <stack>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include <boost/graph/reverse_graph.hpp>

using Vertex_p = boost::property< boost::vertex_name_t, std::string>;
using Edge_p = boost::property< boost::edge_name_t, std::string>;
using Graph_p = boost::property< boost::graph_name_t, std::string>;

using Graph = boost::adjacency_list<
  boost::vecS,
  boost::vecS,
  boost::bidirectionalS,
  Vertex_p,
  Edge_p,
  Graph_p
>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;


void parse(Graph& g, const char* filename)
{
  boost::dynamic_properties props(boost::ignore_other_properties);

  auto name = get(boost::vertex_name, g);
  props.property("label", name);


  std::ifstream input (filename);
  bool success = boost::read_graphviz(input, g, props);

  if (not success) {
    std::cout << "parsing graph failed\n";
    exit(2);
  }
}


Vertex find_start(Graph const& g, std::string target_name) {

  BOOST_FOREACH(auto vertex, boost::vertices(g)) {
    if (get(boost::vertex_name, g, vertex) == target_name) {
      return vertex;
    }
  }

  std::cout << "start not found\n";
  exit(3);
}


template <typename Tag, typename Callable>
struct LambdaVisitor {
    using event_filter = Tag;

    LambdaVisitor(Callable c)
      : m_callback(std::move(c))
    {
    }

    template<typename Elem, typename Graph_>
    void operator() (Elem const& elem, Graph_ const& g) {
      m_callback(elem);
    }

  private:
    Callable m_callback;
};

template <typename Tag, typename Callable>
LambdaVisitor<Tag, Callable>
lambda_visitor(Tag const&, Callable c) {
  return { std::move(c) };
}


void schedule(Graph const& g, std::string target_name)
{
  // "execute" dependencies in order to build a target

  // find the target in the graph
  auto start = find_start(g, target_name);


  // reverse the graph, because the arrows go the wrong direction
  auto reversed_g = boost::make_reverse_graph(g);

  // record execution order in this vector
  std::vector<Vertex> order;

  // execute this visitor on every new node ...
  auto visitor = boost::make_bfs_visitor(
      lambda_visitor(
        boost::on_discover_vertex{},
        [&] (auto v) {
          order.push_back(v);
        }
      ));

  // ... when doing a bfs visit staring from the target
  boost::breadth_first_search(
        reversed_g,
        boost::vertex(start, reversed_g),
        boost::visitor(visitor)
  );

  //
  std::reverse(begin(order), end(order));

  //
  for (auto & v : order) {
    std::cout << "build " <<  get(boost::vertex_name, g, v) << '\n';
  }

}


int main(int argc, char *argv[])
{
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " graph.dot target" << std::endl;
    return 1;
  }

  Graph g;
  parse(g, argv[1]);
  schedule(g, argv[2]);

  return 0;
}
