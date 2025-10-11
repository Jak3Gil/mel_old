#pragma once
#include "melvin_types.h"
#include "text_norm.h"
#include <optional>

// Forward declarations - these are now defined in melvin.cpp
uint64_t node_id(const std::string& text);
std::string node_text(uint64_t node_id);
std::optional<uint64_t> best_neighbor(uint64_t node_id, melvin::Rel rel);
std::optional<uint64_t> two_hop(uint64_t from, uint64_t middle, melvin::Rel rel);

std::optional<std::pair<uint64_t,uint64_t>> find_isa(uint64_t x){
  // prefer direct ISA; fallback to best temporal "is → Y"
  if (auto y = best_neighbor(x, melvin::Rel::ISA)) return {{x,*y}};
  if (auto is_node = node_id("is")) { // if you store "is" as a token node
    if (auto y = two_hop(x, is_node, melvin::Rel::TEMPORAL)) return {{x,*y}};
  }
  return std::nullopt;
}

std::optional<std::pair<uint64_t,uint64_t>> find_consumes(uint64_t x, const std::string& verb){
  // prefer explicit REL_CONSUMES
  if (auto y = best_neighbor(x, melvin::Rel::CONSUMES)) return {{x,*y}};
  // fallback: temporal chain x -> verb -> y
  if (auto v = node_id(verb)) {
    if (auto y = two_hop(x, v, melvin::Rel::TEMPORAL)) return {{x,*y}};
  }
  return std::nullopt;
}
