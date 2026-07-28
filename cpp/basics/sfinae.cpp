#include <iostream>
#include <type_traits>
#include <utility>
#include <sstream>

struct Frame {
  int nums{42};

  std::string serialize() const { return "Frame{nums=" + std::to_string(nums) + "}"; }
};

template <typename T, typename = void>
struct HasSerialize : std::false_type {};

template <typename T>
struct HasSerialize<T, std::void_t<decltype(std::declval<const T&>().serialize())>> : std::true_type {};

template <typename T>
std::enable_if_t<HasSerialize<T>::value, std::string> dump(const T& x)
{
  return x.serialize();
}

template <typename T>
std::enable_if_t<!HasSerialize<T>::value, std::string> dump(const T& x)
{
  std::ostringstream oss;
  oss << x;
  return oss.str();
}

template <typename T>
std::string dump2(const T& x)
{
  if constexpr (HasSerialize<T>::value) {
    return x.serialize();
  } else {
    return std::to_string(x);
  }
}

int main()
{
  std::cout << dump(std::string("foo")) << std::endl;
  std::cout << dump(Frame{42}) << std::endl;
  std::cout << dump(42) << std::endl;
  std::cout << dump2(Frame{42}) << std::endl;
  std::cout << dump2(42) << std::endl;
}
