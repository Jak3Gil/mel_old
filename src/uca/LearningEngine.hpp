#pragma once
#include "uca_types.h"

namespace uca {
class LearningEngine {
public:
  bool configure(const DynamicGenome& g);
  LearningDelta reinforce(const std::vector<ReasoningPath>& paths, const Feedback& fb);
};
}
