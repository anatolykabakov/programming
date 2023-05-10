#include <array>
#include <chrono>
#include <iostream>
#include <tuple>
#include <utility>

template <typename Array, std::size_t... I>
decltype(auto) a2t_impl(const Array &a, std::integer_sequence<std::size_t, I...>) {
    return std::make_tuple(a[I]...);
}

template <typename T, std::size_t N, typename Indices = std::make_index_sequence<N>>
decltype(auto) a2t(const std::array<T, N> &a) {
    return a2t_impl(a, Indices());
}

int main() {
    using namespace std::chrono_literals;
    auto day = 24h;
    std::cout << day.count() << std::endl;  // == 24
    std::cout << std::chrono::duration_cast<std::chrono::minutes>(day).count()
              << std::endl;  // == 1440

    std::array<std::size_t, 5> a = {0, 1, 2, 3, 4};
    decltype(auto) result = a2t(a);

    // std::make_unique
    /***1. Avoid having to use the new operator.
        2. Prevents code repetition when specifying the underlying type the pointer shall
       hold.
        3. Most importantly, it provides exception-safety. Suppose we were calling a
       function foo like so: foo(std::unique_ptr<T>{new T{}}, function_that_throws(),
       std::unique_ptr<T>{new T{}}); The compiler is free to call new T{}, then
       function_that_throws(), and so on... Since we have allocated data on the heap in
       the first construction of a T, we have introduced a leak here. With
       std::make_unique, we are given exception-safety: foo(std::make_unique<T>(),
       function_that_throws(), std::make_unique<T>());
    */
}
