#include "topdown.h"
#include <algorithm>
#include <cmath>

namespace melvin::v2::attention {

TopDownBias::TopDownBias(const Config& config)
    : config_(config) {}

std::vector<float> TopDownBias::compute(
    const std::vector<PerceivedObject>& objects,
    const GlobalWorkspace::Snapshot& gw_snapshot,
    memory::SemanticBridge* semantic) {
    
    std::vector<float> relevance_scores;
    
    for (const auto& obj : objects) {
        float score = compute_object_relevance(obj.object_id, gw_snapshot, semantic);
        relevance_scores.push_back(score);
    }
    
    // Normalize to 0-1
    if (!relevance_scores.empty()) {
        float max_score = *std::max_element(relevance_scores.begin(), relevance_scores.end());
        if (max_score > 1e-6f) {
            for (float& score : relevance_scores) {
                score /= max_score;
            }
        }
    }
    
    return relevance_scores;
}

float TopDownBias::compute_object_relevance(
    NodeID object_id,
    const GlobalWorkspace::Snapshot& gw_snapshot,
    memory::SemanticBridge* semantic) {
    
    if (!semantic) return 0.0f;
    
    float total_relevance = 0.0f;
    
    // 1. Goal matching
    if (!gw_snapshot.current_goal.empty()) {
        float goal_match = goal_similarity(object_id, gw_snapshot.current_goal, semantic);
        total_relevance += config_.goal_weight * goal_match * gw_snapshot.goal_priority;
    }
    
    // 2. Active concept matching
    float concept_match = concept_similarity(object_id, gw_snapshot.thoughts, semantic);
    total_relevance += config_.concept_weight * concept_match;
    
    // 3. Working memory context
    for (const auto& wm_slot : gw_snapshot.wm_slots) {
        if (!wm_slot.is_active) continue;
        
        // Check if object related to WM item
        float weight = semantic->get_edge_weight(object_id, wm_slot.item_ref);
        if (weight > config_.min_similarity) {
            total_relevance += config_.context_weight * weight * wm_slot.precision;
        }
    }
    
    return std::clamp(total_relevance, 0.0f, 1.0f);
}

void TopDownBias::set_config(const Config& config) {
    config_ = config;
}

TopDownBias::Config TopDownBias::get_config() const {
    return config_;
}

// ========================================================================
// PRIVATE HELPERS
// ========================================================================

float TopDownBias::goal_similarity(NodeID object_id, const std::string& goal,
                                   memory::SemanticBridge* semantic) {
    // Find goal-related concepts
    auto goal_concepts = semantic->find_by_label(goal);
    
    if (goal_concepts.empty()) {
        return 0.0f;
    }
    
    // Check if object related to any goal concept
    float max_similarity = 0.0f;
    
    for (NodeID goal_concept : goal_concepts) {
        float weight = semantic->get_edge_weight(object_id, goal_concept);
        max_similarity = std::max(max_similarity, weight);
    }
    
    return max_similarity;
}

float TopDownBias::concept_similarity(NodeID object_id, const std::vector<Thought>& thoughts,
                                     memory::SemanticBridge* semantic) {
    if (thoughts.empty()) return 0.0f;
    
    float total_similarity = 0.0f;
    int count = 0;
    
    // Check similarity to concepts in high-salience thoughts
    for (const auto& thought : thoughts) {
        if (thought.salience < 0.3f) continue;  // Skip low-salience
        
        for (NodeID concept_ref : thought.concept_refs) {
            float weight = semantic->get_edge_weight(object_id, concept_ref);
            
            if (weight > config_.min_similarity) {
                total_similarity += weight * thought.salience;
                count++;
            }
        }
    }
    
    return count > 0 ? total_similarity / count : 0.0f;
}

} // namespace melvin::v2::attention

