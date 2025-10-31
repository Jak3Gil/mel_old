/**
 * @file cm_binding.cpp
 */

#include "cm_binding.h"
#include <algorithm>

namespace melvin {
namespace crossmodal {

void CMBindings::Upsert(const Binding& b) {
    std::lock_guard<std::mutex> lock(mu_);
    auto& vc = by_concept_[b.concept_id];
    bool found = false;
    for (auto& x : vc) {
        if (x.key == b.key && x.mod == b.mod) { x.weight = b.weight; x.source = b.source; found = true; break; }
    }
    if (!found) vc.push_back(b);
    // Cap per-concept bindings (top by weight)
    if (vc.size() > 64) {
        std::sort(vc.begin(), vc.end(), [](const Binding& a, const Binding& b){ return a.weight > b.weight; });
        vc.resize(64);
    }
    auto& vk = by_key_[b.key];
    found = false;
    for (auto& x : vk) {
        if (x.concept_id == b.concept_id && x.mod == b.mod) { x.weight = b.weight; x.source = b.source; found = true; break; }
    }
    if (!found) vk.push_back(b);
}

std::vector<Binding> CMBindings::ForConcept(int64_t id) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = by_concept_.find(id);
    if (it == by_concept_.end()) return {};
    return it->second;
}

std::vector<Binding> CMBindings::ForKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = by_key_.find(key);
    if (it == by_key_.end()) return {};
    return it->second;
}

void CMBindings::PruneConcept(int64_t id, size_t max_keep) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = by_concept_.find(id);
    if (it == by_concept_.end()) return;
    auto& vc = it->second;
    if (vc.size() <= max_keep) return;
    std::sort(vc.begin(), vc.end(), [](const Binding& a, const Binding& b){ return a.weight > b.weight; });
    vc.resize(max_keep);
}

} // namespace crossmodal
} // namespace melvin


