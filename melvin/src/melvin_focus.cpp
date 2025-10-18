#include "melvin_focus.h"
#include <algorithm>

namespace melvin {

FocusManager::FocusManager(AtomicGraph& graph)
    : graph_(graph)
    , current_focus_(0)
    , current_focus_score_(0)
    , frames_on_current_(0)
{}

FocusSelection FocusManager::select(std::span<const std::pair<uint64_t, VisionSystem::Scores>> candidates) {
    if (candidates.empty()) {
        return FocusSelection(0, 0);
    }
    
    // Check if forcing exploration (anti-staring mechanism)
    bool force_switch = dynamics_.should_force_exploration();
    
    // Compute focus scores: F = α*S + β*G + γ*C
    float best_score = -1;
    uint64_t best_node = 0;
    
    for (const auto& [node_id, scores] : candidates) {
        // Check inhibition
        if (inhibition_timers_.count(node_id) && inhibition_timers_[node_id] > 0) {
            continue; // Inhibited
        }
        
        // Force skip current if staring too long
        if (force_switch && node_id == current_focus_) {
            continue;  // FORCE exploration!
        }
        
        float F = constants::ALPHA_SALIENCY * scores.saliency +
                  constants::BETA_GOAL * scores.goal +
                  constants::GAMMA_CURIOSITY * scores.curiosity;
        
        // Apply BOREDOM penalty (prevents staring)
        float boredom = dynamics_.get_boredom_penalty(node_id);
        F += boredom;  // boredom is negative (-0.05 to -0.5)
        
        // Apply EXPLORATION bonus (encourages novel targets)
        float exploration = dynamics_.get_exploration_bonus(node_id);
        F += exploration;  // exploration is positive (0 to 0.2)
        
        // Inertia: prefer current focus (15% boost) - but weaken if staring
        if (node_id == current_focus_) {
            if (frames_on_current_ < 15) {
                F *= 1.15f;  // Normal inertia
            } else if (frames_on_current_ < 25) {
                F *= 1.05f;  // Reduced inertia (getting bored)
            }
            // else: no inertia bonus (bored!)
        }
        
        if (F > best_score) {
            best_score = F;
            best_node = node_id;
        }
    }
    
    // Update focus
    if (best_node != 0) {
        // Track focus duration
        if (best_node == current_focus_) {
            frames_on_current_++;
        } else {
            frames_on_current_ = 1;
        }
        
        // Add new ACTIVE_FOCUS edge
        graph_.add_or_bump_edge(best_node, best_node, Rel::ACTIVE_FOCUS, 1.0f);
        
        current_focus_ = best_node;
        current_focus_score_ = best_score;
        
        // Record for boredom/exploration tracking
        dynamics_.record_focus(best_node);
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
    
    // Update attention dynamics (boredom decay, etc.)
    dynamics_.update(dt);
}

} // namespace melvin


