#pragma once
#include <functional>
#include <unordered_map>
#include <memory>
// ref https://dxuuu.xyz/cpp-static-registration.html
#include "IGame.h"

// #define REGISTER_PLUGIN(plugin_name, create_func)                                                                      \
//   bool plugin_name##_entry = PluginRegistry<IGame>::add(#plugin_name, (create_func))

template <typename T>
class PluginRegistry {
public:
  typedef std::function<T*()> FactoryFunction;
  typedef std::unordered_map<std::string, FactoryFunction> FactoryMap;

  static bool add(const std::string& name, FactoryFunction fac)
  {
    auto map = getFactoryMap();
    if (map.find(name) != map.end()) {
      return false;
    }

    getFactoryMap()[name] = fac;
    return true;
  }

  static T* create(const std::string& name)
  {
    auto map = getFactoryMap();
    if (map.find(name) == map.end()) {
      return nullptr;
    }

    return map[name]();
  }

  static std::vector<std::string> keys()
  {
    auto map = getFactoryMap();
    std::vector<std::string> keys;
    for (const auto& [key, value] : map) {
      keys.push_back(key);
    }
    return keys;
  }

private:
  // Use Meyer's singleton to prevent SIOF
  static FactoryMap& getFactoryMap()
  {
    static FactoryMap map;
    return map;
  }
};
