#pragma once
#include "uca_types.h"

namespace uca {
class OutputManager {
public:
  bool configure(const DynamicGenome& g);
  // Returns true if emitted (printed/sent)
  bool emit(const ReasoningResult& r);
};
}
