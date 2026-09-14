#include <iostream>
#include <vector>

std::vector<std::string> split(const std::string& s, std::string delimeter)
{
  if (delimeter == "") {
    return {s};
  }
  std::vector<std::string> words;
  int start{0};  // hello,world,!
  while (true) {
    auto end = s.find(delimeter, start);
    if (end == std::string::npos) {
      words.push_back(s.substr(start));
      break;
    }
    words.push_back(s.substr(start, end - start));
    start = end + delimeter.size();
  }

  return words;
}

int main()
{
  std::string s = "hello";
  s += ",world";
  std::cout << s << " " << s.size() << " " << s.front() << " " << s.back() << " " << s.empty() << " "
            << s.compare("hello,world") << " " << std::endl;
  s = s + ",!";

  char c = 'a';
  const char* lit = "a";
  std::cout << c << " " << lit << std::endl;
  auto words = split(s, ",");
  for (const auto& w : words) {
    std::cout << w << " + ";
  }
  std::cout << std::endl;

  int i = std::stoi("123");
  double d = std::stod("3.14");

  std::string ns = std::to_string(42);

  std::cout << i << " " << d << " " << ns << std::endl;

  std::string_view sv = ns;  // ok view, без копии
                             // std::string_view svub = std::string("temp") UB error
}
