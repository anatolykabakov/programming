#include <iostream>
// 3! = 1 * 2 * 3 = 6
constexpr int factorial(int n) {
  if (n <= 1) {
    return n;
  }
  return n * factorial(n - 1);
}

template <typename T>
constexpr T PI{3.1415926535897932385};

[[deprecated]] void old_method() {

}[[deprecated("Use new_method instead")]] void legacy_method() {}

int main() {
  // Relaxing constraints on constexpr functions ( don't need to write constexpr for every vars in function )
  constexpr int f = factorial(3);

  // Variable templates

  std::cout << PI<double> << std::endl;
  std::cout << PI<float> << std::endl;
  std::cout << PI<int> << std::endl;

  // [[deprecated]]
  // old_method(); // warning: ‘void old_method()’ is deprecated
  // legacy_method(); // warning: ‘void legacy_method()’ is deprecated: Use new_method instead
  // [-Wdeprecated-declarations]

  return 0;
}
