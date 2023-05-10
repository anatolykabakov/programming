#include <iostream>
#include <unordered_map>

int main() {
    std::unordered_map<std::string, int> mapping{{"a", 1}, {"b", 2}, {"c", 3}};

    // Destructure by reference.
    for (const auto &[key, value] : mapping) {
        // Do something with key and value
        std::cout << key << " " << value << std::endl;
    }
    return 0;
}
