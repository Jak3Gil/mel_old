#pragma once
#include "uca_types.h"

namespace uca {
class FeedbackBus {
public:
  // Combine internal heuristics + external feedback (if any)
  Feedback collect(const ReasoningResult& r);
};
}
