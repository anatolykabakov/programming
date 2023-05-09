#include <iostream>
#include <vector>

// http://scrutator.me/post/2016/12/12/sfinae.aspx

template <bool condition, typename Type>
struct EnableIf;

template <typename Type>
struct EnableIf<true, Type> {
    using type = Type;
};

template <typename T, typename U>
struct IsSame {
    static constexpr bool value = false;
};

template <typename T>
struct IsSame<T, T> {
    static constexpr bool value = true;
};

template <typename T>
typename EnableIf<!IsSame<T, int>::value, void>::type print_container(T values) {
    for (auto &value : values) {
        std::cout << value << std::endl;
    }
}

int main() {
    std::vector<int> foo(4, 1);
    print_container(foo);
    // print_container(4);

    return 0;
}
