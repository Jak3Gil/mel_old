#pragma once
#include "uca_types.h"
#include <deque>

namespace uca {
class ReasoningEngine {
public:
  bool configure(const DynamicGenome& g);
  ReasoningResult infer(const Percept& p);
};

// Gate decision system functions
Gate decide_gate(float conf, float entropy, float top2_margin);
Gate hysteresis_gate(Gate raw, GateState& st);
float threshold_for_intent(Intent t);
Intent detect_intent(const std::vector<std::string>& tokens);
float rolling_quantile(const std::deque<float>& hist, float q);
bool safety_check(const ReasoningResult& rr);

}
