#include "melvin_focus.h"
#include <algorithm>

namespace melvin {

FocusManager::FocusManager(AtomicGraph& graph)
    : graph_(graph)
    , current_focus_(0)
    , current_focus_score_(0)
{}

FocusSelection FocusManager::select(std::span<const std::pair<uint64_t, VisionSystem::Scores>> candidates) {
    if (candidates.empty()) {
        return FocusSelection(0, 0);
    }
    
    // Compute focus scores: F = α*S + β*G + γ*C
    float best_score = -1;
    uint64_t best_node = 0;
    
    for (const auto& [node_id, scores] : candidates) {
        // Check inhibition
        if (inhibition_timers_.count(node_id) && inhibition_timers_[node_id] > 0) {
            continue; // Inhibited
        }
        
        float F = constants::ALPHA_SALIENCY * scores.saliency +
                  constants::BETA_GOAL * scores.goal +
                  constants::GAMMA_CURIOSITY * scores.curiosity;
        
        // Inertia: prefer current focus (15% boost)
        if (node_id == current_focus_) {
            F *= 1.15f;
        }
        
        if (F > best_score) {
            best_score = F;
            best_node = node_id;
        }
    }
    
    // Update focus
    if (best_node != 0) {
        // Remove old ACTIVE_FOCUS edge
        if (current_focus_ != 0) {
            // Would remove old edge here
        }
        
        // Add new ACTIVE_FOCUS edge
        graph_.add_or_bump_edge(best_node, best_node, Rel::ACTIVE_FOCUS, 1.0f);
        
        current_focus_ = best_node;
        current_focus_score_ = best_score;
    }
    
    return FocusSelection(best_node, best_score);
}

void FocusManager::inhibit(uint64_t node_id, float seconds) {
    inhibition_timers_[node_id] = seconds;
}

void FocusManager::update(float dt) {
    // Update inhibition timers
    std::vector<uint64_t> to_remove;
    for (auto& [node_id, remaining] : inhibition_timers_) {
        remaining -= dt;
        if (remaining <= 0) {
            to_remove.push_back(node_id);
        }
    }
    
    for (uint64_t id : to_remove) {
        inhibition_timers_.erase(id);
    }
}

} // namespace melvin


