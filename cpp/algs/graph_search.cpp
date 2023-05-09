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

    std::vector<std::pair<int, int>> neighbors(int i) const { return adj_list_[i]; }

private:
    std::vector<std::vector<std::pair<int, int>>> adj_list_;

    bool root_;
    int root_id_;
};

bool breadth_first_search(const Graph &graph, int start, int stop) {
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

        for (const auto &[v_id, w] : graph.neighbors(current_v_id)) {
            if (!visited[v_id]) {
                not_visited.push(v_id);
            }
        }
    }

    return false;
}

bool depth_first_search(const Graph &graph, int start, int stop) {
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

        for (const auto &[v_id, w] : graph.neighbors(current_v_id)) {
            if (!visited[v_id]) {
                nodes.push(v_id);
            }
        }
    }

    return false;
}

bool dijkstra(const Graph &g, int start, int stop) {
    std::vector<bool> visited(g.num_vertices(), false);
    std::vector<int> costs(g.num_vertices(), 1000);
    std::queue<int> q;
    q.push(g.root_id());
    costs[g.root_id()] = 0;

    while (!q.empty()) {
        int node = q.front();
        q.pop();
        visited[node] = true;
        int cost = costs[node];
        std::cout << "current vertex id " << node << std::endl;

        if (node == stop) {
            return true;
        }

        // update neighbors costs
        for (const auto &[id, weight] : g.neighbors(node)) {
            int cost_from_start_to_neighbour = cost + weight;
            if (cost_from_start_to_neighbour < costs[id]) {
                costs[id] = cost_from_start_to_neighbour;
            }
        }

        // find node with lowerst cost and not visited
        int min_cost = 1000;
        int lowerst_cost_node_id = 0;
        bool found = false;
        for (int i = 0; i < costs.size(); ++i) {
            if (min_cost > costs[i] && !visited[i]) {
                min_cost = costs[i];
                found = true;
                lowerst_cost_node_id = i;
            }
        }

        if (found) {
            q.push(lowerst_cost_node_id);
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

    dijkstra(g, 0, 6);
    std::cout << "breadth_first_search: " << breadth_first_search(g, 0, 6) << std::endl;
    std::cout << "depth_first_search: " << depth_first_search(g, 0, 6) << std::endl;
    return 0;
}
