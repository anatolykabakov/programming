#include <iostream>
#include <tuple>
#include <utility>
#include <string>
#include <vector>
#include <map>

std::tuple<int, std::string, double> returnTuple() { return std::make_tuple(0, "anatoly", 29.6); }

int main()
{
  std::pair<int, std::string> p{1, "2"};
  std::cout << p.first << " " << p.second << std::endl;
  auto& [value1, value2] = p;
  std::cout << value1 << " " << value2 << std::endl;
  auto res = returnTuple();
  std::string name;
  double age{0};
  std::tie(std::ignore, name, age) = res;
  std::cout << name << " " << age << std::endl;

  return 0;
}
