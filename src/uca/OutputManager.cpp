#include "src/uca/OutputManager.hpp"
#include <iostream>
namespace uca {
bool OutputManager::configure(const DynamicGenome&) { return true; }
bool OutputManager::emit(const ReasoningResult& r) {
  switch (r.type) {
    case OutputType::Text:
      std::cout << r.text << std::endl;
      return true;
    default:
      return false;
  }
}
}
