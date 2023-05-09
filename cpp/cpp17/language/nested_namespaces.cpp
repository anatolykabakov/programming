
// before c++17
// namespace A {
//   namespace B {
//     namespace C {
//       void hello() {}
//     }
//   }
// }
#include <iostream>

namespace A::B::C {
void hello() { std::cout << "hello" << std::endl; }
}  // namespace A::B::C

int main() { A::B::C::hello(); }
