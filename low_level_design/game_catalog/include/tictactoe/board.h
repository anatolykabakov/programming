#pragma once
#include <vector>
#include <ostream>

enum class cell_state { empty, zero, cross };

struct Board {
  std::vector<cell_state> cells;
};
