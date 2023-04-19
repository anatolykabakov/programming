// https://www.programiz.com/dsa/graph-bfs
#include <iostream>
#include <list>
#include <queue>
#include <stack>
#include <vector>

class Graph {
public:
  Graph(int num_vertices) : root_{true}, root_id_{0} { adj_list_.resize(num_vertices); }

  void add_edge(int src, int dest, int w = 1) {
    if (root_) {
      root_id_ = src;
      root_ = false;
    }
    adj_list_[src].push_back({dest, w});
  }

  std::size_t num_vertices() const { return adj_list_.size(); }

  int root_id() const { return root_id_; }

  std::vector<std::pair<int, int>> connected_vertices(int i) const { return adj_list_[i]; }

private:
  std::vector<std::vector<std::pair<int, int>>> adj_list_;

  bool root_;
  int root_id_;
};

bool breadth_first_search(const Graph& graph, int start, int stop) {
  std::queue<int> not_visited;

  std::vector<bool> visited(graph.num_vertices(), false);

  not_visited.push(start);

  while (!not_visited.empty()) {
    int current_v_id = not_visited.front();
    not_visited.pop();

    visited[current_v_id] = true;

    std::cout << "current vertex id " << current_v_id << std::endl;
    if (current_v_id == stop) {
      return true;
    }

    for (const auto& [v_id, w] : graph.connected_vertices(current_v_id)) {
      if (!visited[v_id]) {
        not_visited.push(v_id);
      }
    }
  }

  return false;
}

bool depth_first_search(const Graph& graph, int start, int stop) {
  std::stack<int> nodes;

  std::vector<bool> visited(graph.num_vertices(), false);

  nodes.push(start);

  while (!nodes.empty()) {
    int current_v_id = nodes.top();
    nodes.pop();

    visited[current_v_id] = true;

    std::cout << "current vertex id " << current_v_id << std::endl;
    if (current_v_id == stop) {
      return true;
    }

    for (const auto& [v_id, w] : graph.connected_vertices(current_v_id)) {
      if (!visited[v_id]) {
        nodes.push(v_id);
      }
    }
  }

  return false;
}

bool dijkstra_search(const Graph& graph, int start, int stop) {
  std::vector<int> costs(graph.num_vertices(), 1000000000);
  std::vector<bool> visited(graph.num_vertices(), false);
  std::queue<int> queue;

  queue.push(start);
  costs[start] = 0;

  while (!queue.empty()) {
    int node_id = queue.front();
    queue.pop();
    visited[node_id] = true;
    std::cout << "current vertex id " << node_id << std::endl;
    int node_cost = costs[node_id];

    if (node_id == stop) {
      std::cout << "path from " << start << " to " << stop << " found with cost " << node_cost << std::endl;
      return true;
    }

    for (const auto& [id, w] : graph.connected_vertices(node_id)) {
      int new_cost = node_cost + w;
      if (new_cost < costs[id]) {
        costs[id] = new_cost;
      }
    }

    int min_cost_node_id = 0;
    int lowest_cost = 1000000000;
    bool lowest_cost_node_found = false;
    for (int i = 0; i < costs.size(); ++i) {
      int node_cost = costs[i];
      if (node_cost < lowest_cost && !visited[i]) {
        min_cost_node_id = i;
        lowest_cost = node_cost;
        lowest_cost_node_found = true;
      }
    }
    if (lowest_cost_node_found) {
      queue.push(min_cost_node_id);
    }
  }
  return false;
}

int main() {
  Graph g(7);
  g.add_edge(0, 1, 2);  // A->B 2
  g.add_edge(0, 2, 1);  // A->C 2
  g.add_edge(1, 5, 7);  // B->F 7
  g.add_edge(2, 3, 5);  // C->D 5
  g.add_edge(2, 4, 2);  // C->E 2
  g.add_edge(3, 5, 2);  // D->F 2
  g.add_edge(4, 5, 1);  // E->F 1
  g.add_edge(5, 6, 1);  // F->G 1

  std::cout << "dijkstra_test: " << dijkstra_search(g, 0, 6) << std::endl;
  std::cout << "breadth_first_search: " << breadth_first_search(g, 0, 6) << std::endl;
  std::cout << "depth_first_search: " << depth_first_search(g, 0, 6) << std::endl;
  return 0;
}
