#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include "melvin_vision.h"
#include <span>
#include <unordered_map>

namespace melvin {

/**
 * FocusManager - FEF + Superior Colliculus
 * 
 * Biological analog:
 * - Frontal Eye Fields (FEF): Top-down attention control
 * - Superior Colliculus (SC): Saccade generation
 * - Pulvinar: Inhibition of return
 */
class FocusManager {
public:
    explicit FocusManager(AtomicGraph& graph);
    
    /**
     * Select focus target from candidates
     * Uses formula: F = α*saliency + β*goal + γ*curiosity
     * With inertia and inhibition of return
     * 
     * @param candidates List of (node_id, scores) pairs
     * @return Selected focus target
     */
    FocusSelection select(std::span<const std::pair<uint64_t, VisionSystem::Scores>> candidates);
    
    /**
     * Inhibit node from selection (inhibition of return)
     * @param node_id Node to inhibit
     * @param seconds Duration of inhibition
     */
    void inhibit(uint64_t node_id, float seconds = 0.8f);
    
    /**
     * Update inhibition timers
     * @param dt Time delta in seconds
     */
    void update(float dt);
    
    /**
     * Get current focus
     */
    uint64_t get_current_focus() const { return current_focus_; }
    
private:
    AtomicGraph& graph_;
    uint64_t current_focus_;
    float current_focus_score_;
    std::unordered_map<uint64_t, float> inhibition_timers_; // node_id -> remaining seconds
};

} // namespace melvin


