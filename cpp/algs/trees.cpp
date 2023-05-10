#include <iostream>
#include <stack>
#include <vector>

struct Node {
    int value;
    std::vector<Node> childs;
};

int sum_iterative(Node &node) {
    int sum = 0;
    std::stack<Node> nodes;
    nodes.push(node);
    while (!nodes.empty()) {
        auto curr = nodes.top();
        nodes.pop();
        std::cout << curr.value << std::endl;
        sum += curr.value;

        for (auto &clild : curr.childs) {
            nodes.push(clild);
        }
    }
    return sum;
}

int sum_recursive(Node &node) {
    if (node.childs.empty()) {
        return node.value;
    }
    int sum = node.value;

    for (auto &child : node.childs) {
        sum += sum_recursive(child);
    }

    return sum;
}

int main() {
    Node root;
    root.value = 0;
    Node level1_node1;
    level1_node1.value = 5;
    Node level1_node2;
    level1_node2.value = 6;
    Node level1_node3;
    level1_node3.value = 1;
    Node level1_node4;
    level1_node4.value = 8;

    Node level2_node1;
    level2_node1.value = 5;

    Node level2_node2;
    level2_node2.value = 5;
    Node level2_node3;
    level2_node3.value = 5;
    Node level2_node4;
    level2_node4.value = 5;

    Node level2_node5;
    level2_node5.value = 5;
    Node level2_node6;
    level2_node6.value = 5;

    Node level2_node7;
    level2_node7.value = 5;

    Node level3_node1;
    level3_node1.value = 5;
    Node level3_node2;
    level3_node2.value = 5;

    Node level3_node3;
    level3_node3.value = 5;

    level2_node1.childs.push_back(level3_node1);
    level2_node1.childs.push_back(level3_node2);

    level2_node7.childs.push_back(level3_node3);

    level1_node1.childs.push_back(level2_node1);

    level1_node2.childs.push_back(level2_node2);
    level1_node2.childs.push_back(level2_node3);
    level1_node2.childs.push_back(level2_node4);

    level1_node3.childs.push_back(level2_node5);
    level1_node3.childs.push_back(level2_node6);

    level1_node4.childs.push_back(level2_node7);

    root.childs.push_back(level1_node1);
    root.childs.push_back(level1_node2);
    root.childs.push_back(level1_node3);
    root.childs.push_back(level1_node4);

    std::cout << "sum_iterative " << sum_iterative(root) << std::endl;
    std::cout << "sum_recursive " << sum_recursive(root) << std::endl;

    return 0;
}
