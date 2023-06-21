#include <iostream>
#include <unordered_map>

class Trie {
public:
  struct Node {
    std::unordered_map<char, Node*> children;
    bool word{false};
  };
  Trie() { root_ = new Node{}; }
  void insert(const std::string& word)
  {
    Node* cur = root_;
    for (const auto& c : word) {
      if (cur->children.find(c) == cur->children.end()) {
        cur->children.insert(std::make_pair(c, new Node{}));
      }
      cur = cur->children[c];
    }
    cur->word = true;
  }

  bool search(const std::string& word)
  {
    Node* cur = root_;
    for (const auto& c : word) {
      if (cur->children.find(c) == cur->children.end()) {
        return false;
      }
      cur = cur->children[c];
    }
    return cur->word;
  }

  bool start_with(const std::string& prefix)
  {
    Node* cur = root_;
    for (const auto& c : prefix) {
      if (cur->children.find(c) == cur->children.end()) {
        return false;
      }
      cur = cur->children[c];
    }
    return true;
  }

private:
  Node* root_;
};

int main()
{
  Trie trie;
  trie.insert("apple");
  std::cout << trie.search("apple") << std::endl;    // True
  std::cout << trie.search("nope") << std::endl;     // False
  std::cout << trie.start_with("app") << std::endl;  // True
  return 0;
}
