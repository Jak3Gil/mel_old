#pragma once
#include "uca_types.h"
#include <deque>

namespace uca {
class ReflectionEngine {
public:
  bool configure(const DynamicGenome& g);
  // pass recent confidences/rewards; decide if we need to tweak genome
  ReflectReport reflect(const std::deque<float>& confidences, const std::deque<float>& rewards);
};
}
