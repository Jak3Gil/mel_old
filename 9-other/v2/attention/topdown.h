#pragma once

#include "../core/types_v2.h"
#include "../core/global_workspace.h"
#include "../memory/semantic_bridge.h"
#include <vector>
#include <memory>

namespace melvin::v2::attention {

// ============================================================================
// TOP-DOWN BIAS - Goal-Driven Attention
// ============================================================================

/**
 * TopDownBias computes relevance scores based on current goals and active concepts.
 * 
 * Mechanism:
 * - Read current goal from Global Workspace
 * - Read active thoughts (percepts, hypotheses)
 * - Compute semantic similarity between objects and goals
 * - Boost objects that match active concepts
 * 
 * Formula:
 *   R = goal_match + Σ(concept_match) + context_bonus
 */
class TopDownBias {
public:
    struct Config {
        float goal_weight;           // Weight for goal matching
        float concept_weight;        // Weight for concept matching
        float context_weight;        // Weight for contextual relations
        float min_similarity;        // Threshold for matching
        
        Config()
            : goal_weight(0.6f), concept_weight(0.3f),
              context_weight(0.1f), min_similarity(0.3f) {}
    };
    
    TopDownBias(const Config& config = Config());
    
    /**
     * Compute top-down relevance scores
     * 
     * @param objects - Perceived objects
     * @param gw_snapshot - Current Global Workspace state
     * @param semantic - Semantic memory for concept matching
     * @returns Relevance scores (0-1) for each object
     */
    std::vector<float> compute(
        const std::vector<PerceivedObject>& objects,
        const GlobalWorkspace::Snapshot& gw_snapshot,
        memory::SemanticBridge* semantic
    );
    
    /**
     * Compute relevance for single object
     */
    float compute_object_relevance(
        NodeID object_id,
        const GlobalWorkspace::Snapshot& gw_snapshot,
        memory::SemanticBridge* semantic
    );
    
    void set_config(const Config& config);
    Config get_config() const;
    
private:
    Config config_;
    
    // Helper: compute similarity between object and goal
    float goal_similarity(NodeID object_id, const std::string& goal,
                         memory::SemanticBridge* semantic);
    
    // Helper: compute similarity to active concepts
    float concept_similarity(NodeID object_id, const std::vector<Thought>& thoughts,
                            memory::SemanticBridge* semantic);
};

} // namespace melvin::v2::attention

