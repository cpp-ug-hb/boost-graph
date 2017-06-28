#include <iostream>
#include <fstream>
#include <stack>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/topological_sort.hpp>
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
using Edge = boost::graph_traits<Graph>::edge_descriptor;


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

template<typename G>
struct reachable {
  reachable() { }
  reachable(const G* graph, std::set<Vertex> reachable)
    : m_graph(graph)
    , m_reachable(reachable)
  { }

  bool operator()(const Edge& e) const {
    return true;
  }

  bool operator()(const Vertex& v) const {
    return m_reachable.count(v);
  }

  const G* m_graph = nullptr;
  std::set<Vertex> m_reachable;
};


template< typename G>
std::set<Vertex>
reachable_part(G const & g, Vertex start)
{

  // record execution order in this vector
  std::set<Vertex> reachable_vertices;

  // execute this visitor on every new node ...
  auto visitor = boost::make_bfs_visitor(
      lambda_visitor(
        boost::on_discover_vertex{},
        [&] (auto v) {
          reachable_vertices.insert(v);
        }
      ));

  // ... when doing a bfs visit staring from the target
  boost::breadth_first_search(
        g,
        boost::vertex(start, g),
        boost::visitor(visitor)
  );

  return reachable_vertices;
}


template<typename G>
std::vector<Vertex> schedule(G const& g)
{
  std::vector<Vertex> order;
  boost::topological_sort(g, std::back_inserter(order));

  return order;
}

void run(Graph const& g, std::string target_name)
{
  // "execute" dependencies in order to build a target

  // find the target in the graph
  auto start = find_start(g, target_name);

  // reverse the graph, because the arrows go the wrong direction
  auto reversed_g = boost::make_reverse_graph(g);

  auto reachable_vertices = reachable_part(reversed_g, start);

//  auto execution_order = schedule(reachable_g);
  auto execution_order = schedule(reversed_g);

  for (auto & v : execution_order) {
    if  (reachable_vertices.count(v)) {
      std::cout << "build " <<  get(boost::vertex_name, g, v) << '\n';
    }
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
  run(g, argv[2]);

  return 0;
}
