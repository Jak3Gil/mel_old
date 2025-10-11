#pragma once
#include <string>
#include <vector>
#include "text_norm.h"

enum class QType { WHAT_IS, WHAT_DO_VERB, OTHER };

struct QParse {
  QType type = QType::OTHER;
  std::string subj;   // "cats", "artificial intelligence"
  std::string verb;   // "drink" for WHAT_DO_VERB
};

static inline bool starts_with(const std::string& s, const char* p){
  return s.rfind(p, 0) == 0; // prefix
}

inline QParse parse_question(const std::string& raw){
  auto s = norm(raw);
  // "what is X" / "what's X"
  if (starts_with(s, "what is "))  return {QType::WHAT_IS, s.substr(8), ""};
  if (starts_with(s, "whats "))     return {QType::WHAT_IS, s.substr(6), ""};

  // "what do <subj> <verb>"  e.g., "what do cats drink"
  if (starts_with(s, "what do ")) {
    // naive split: what do | cats | drink
    auto rest = s.substr(8);
    auto p = rest.rfind(' ');
    if (p!=std::string::npos) {
      return {QType::WHAT_DO_VERB, rest.substr(0,p), rest.substr(p+1)};
    }
  }
  return {};
}
