#pragma once

#include <string>
#include <vector>

struct Service {
    std::string name;
    int port;
};

extern std::vector<Service> services;
