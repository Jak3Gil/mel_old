#include "attention_dynamics.h"
#include <algorithm>

namespace melvin {

AttentionDynamics::AttentionDynamics()
    : current_focus_(0)
    , frames_on_current_(0)
    , stable_context_frames_(0)
{}

void AttentionDynamics::set_config(const Config& config) {
    config_ = config;
}

// ============================================================================
// ANTI-STICKING MECHANISMS
// ============================================================================

void AttentionDynamics::record_focus(uint64_t node_id) {
    // Add to history
    focus_history_.push_back(node_id);
    if (focus_history_.size() > 50) {
        focus_history_.pop_front();
    }
    
    // Track consecutive frames
    if (node_id == current_focus_) {
        frames_on_current_++;
    } else {
        frames_on_current_ = 1;
        current_focus_ = node_id;
    }
    
    // Update focus record
    auto& record = focus_records_[node_id];
    record.node_id = node_id;
    
    if (node_id == current_focus_) {
        record.consecutive_frames = frames_on_current_;
        
        // Accumulate boredom
        if (frames_on_current_ > 10) {
            record.boredom_level += config_.boredom_rate;
            record.boredom_level = std::min(1.0f, record.boredom_level);
        }
    }
}

float AttentionDynamics::get_boredom_penalty(uint64_t node_id) const {
    auto it = focus_records_.find(node_id);
    if (it == focus_records_.end()) {
        return 0.0f;  // Never focused, no boredom
    }
    
    const auto& record = it->second;
    
    // Boredom increases with consecutive frames
    float penalty = 0.0f;
    
    if (record.consecutive_frames > 15) {
        // Really bored now
        penalty = -0.3f;
    } else if (record.consecutive_frames > 10) {
        // Getting bored
        penalty = -0.15f;
    } else if (record.consecutive_frames > 5) {
        // Slightly bored
        penalty = -0.05f;
    }
    
    // Add accumulated boredom
    penalty -= record.boredom_level * 0.2f;
    
    return std::max(-0.5f, penalty);  // Cap at -0.5
}

float AttentionDynamics::get_exploration_bonus(uint64_t node_id) const {
    // Check recent focus history
    int recent_count = 0;
    for (auto it = focus_history_.rbegin(); 
         it != focus_history_.rend() && recent_count < 20; 
         ++it) {
        if (*it == node_id) {
            return 0.0f;  // Recently focused, no bonus
        }
        recent_count++;
    }
    
    // Not recently focused - give exploration bonus!
    return config_.exploration_bonus_rate * recent_count;  // Up to 0.2
}

bool AttentionDynamics::should_force_exploration() const {
    // Force switch if focused too long
    return frames_on_current_ > config_.max_focus_duration;
}

bool AttentionDynamics::is_context_saturated(const std::vector<uint64_t>& active_concepts) {
    // Track context size over time
    context_size_history_.push_back(active_concepts.size());
    if (context_size_history_.size() > 20) {
        context_size_history_.pop_front();
    }
    
    // Check if context is stable (not changing much)
    if (context_size_history_.size() < 10) {
        return false;  // Not enough data
    }
    
    // Compute variance in context size
    float mean = 0;
    for (size_t s : context_size_history_) {
        mean += s;
    }
    mean /= context_size_history_.size();
    
    float variance = 0;
    for (size_t s : context_size_history_) {
        float diff = s - mean;
        variance += diff * diff;
    }
    variance /= context_size_history_.size();
    
    // Low variance = stable context = saturation
    if (variance < 2.0f) {
        stable_context_frames_++;
    } else {
        stable_context_frames_ = 0;
    }
    
    return stable_context_frames_ > config_.saturation_threshold;
}

float AttentionDynamics::get_saturation_curiosity_boost() const {
    if (stable_context_frames_ > config_.saturation_threshold) {
        // Context saturated - boost curiosity to break out!
        return config_.saturation_curiosity_mult;
    }
    return 1.0f;  // No boost
}

void AttentionDynamics::update(float dt) {
    // Decay boredom for unfocused objects
    for (auto& [node_id, record] : focus_records_) {
        if (node_id != current_focus_) {
            record.boredom_level *= 0.95f;  // Decay boredom
            record.consecutive_frames = 0;
        }
    }
}

void AttentionDynamics::reset() {
    focus_records_.clear();
    focus_history_.clear();
    context_size_history_.clear();
    current_focus_ = 0;
    frames_on_current_ = 0;
    stable_context_frames_ = 0;
}

} // namespace melvin

