#include <iostream>
#include <string>
#include <vector>

std::vector<int> tokenize(const std::string& source);

int main(int argc, char** argv) {
  if (argc != 3) {
    return 2;
  }
  const int iterations = std::stoi(argv[1]);
  const int expected = std::stoi(argv[2]);
  const std::string unit = "if ifa == = 42 @ ";
  std::string input;
  input.reserve(unit.size() * static_cast<std::size_t>(iterations));
  for (int index = 0; index < iterations; ++index) {
    input += unit;
  }
  const auto tokens = tokenize(input);
  std::cout << tokens.size() << '\n';
  return static_cast<int>(tokens.size()) == expected ? 0 : 1;
}
