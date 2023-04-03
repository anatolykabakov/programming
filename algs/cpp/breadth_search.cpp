// https://www.programiz.com/dsa/graph-bfs
#include <iostream>
#include <list>
#include <queue>
#include <vector>

class Graph {
public:
  Graph(int num_vertices): root_{true}, root_id_{0} {
    adj_list_.resize(num_vertices);
  }

  void add_edge(int src, int dest) {
    if (root_) {
      root_id_ = src;
      root_ = false;
    }
    adj_list_[src].push_back(dest);
  }

  std::size_t num_vertices() const  {
    return adj_list_.size();
  }

  int root_id() const {
    return root_id_;
  }

  std::vector<int> connected_vertices(int i) const {
    return adj_list_[i];
  }
private:
  std::vector<std::vector<int>> adj_list_;

  bool root_;
  int root_id_;
};



class BreadthSearch {
public:
  BreadthSearch() {

  }

  void run(const Graph &graph) {
    std::queue<int> not_visited;
    
    std::vector<bool> visited(graph.num_vertices(), false);

    not_visited.push(graph.root_id());

    while (!not_visited.empty()) {
      int current_v_id = not_visited.front();
      not_visited.pop();

      visited[current_v_id] = true;

      std::cout << "current vertex id " << current_v_id << std::endl;

      for (const auto& v_id: graph.connected_vertices(current_v_id)) {
        if (!visited[v_id]) {
          not_visited.push(v_id);
        }
      }
    }
    
  }
};

int main() {
  Graph graph(7);
  graph.add_edge(0, 1);
  graph.add_edge(0, 3);
  graph.add_edge(0, 2);
  graph.add_edge(2, 1);
  graph.add_edge(2, 4);

  BreadthSearch search;
  search.run(graph);
  return 0;
}