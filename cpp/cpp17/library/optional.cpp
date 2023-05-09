#include <iostream>
#include <optional>

std::optional<std::string> create(bool b) {
    if (b) {
        return "Godzilla";
    } else {
        return {};
    }
}

int main() {
    create(false).value_or("empty");  // == "empty"
    create(true).value();             // == "Godzilla"
    // optional-returning factory functions are usable as conditions of while and if
    const auto result = create(true);
    if (result.has_value()) {
        std::cout << result.value() << std::endl;
    }
    return 0;
}
