#pragma once
#include "uca_types.h"

namespace uca {
class PerceptionEngine {
public:
  bool configure(const DynamicGenome& g);
  Percept perceive(const InputConcept& in);
};
}
