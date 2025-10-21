#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include <vector>
#include <string>

namespace melvin {

/**
 * Thought structure - subject-predicate-object triple
 */
struct Thought {
    uint64_t subj;         // Subject node
    uint64_t pred_concept; // Predicate (relation concept)
    uint64_t obj;          // Object node
    float confidence;      // Confidence score
    
    Thought() : subj(0), pred_concept(0), obj(0), confidence(0) {}
};

/**
 * ReasoningEngine - Prefrontal Cortex (PFC)
 * 
 * Biological analog:
 * - Dorsolateral PFC: Multi-hop reasoning
 * - Ventrolateral PFC: Semantic retrieval
 * - Frontopolar cortex: Integration and abstraction
 */
class ReasoningEngine {
public:
    explicit ReasoningEngine(AtomicGraph& graph);
    
    /**
     * Get currently active concepts (global workspace)
     * @return List of concept node IDs
     */
    std::vector<uint64_t> active_concepts() const;
    
    /**
     * Infer thought from focused node
     * Performs multi-hop graph reasoning
     * 
     * @param focus_node Currently focused node
     * @return Generated thought triple
     */
    Thought infer_from_focus(uint64_t focus_node);
    
    /**
     * Convert thought to natural language
     * @param t Thought to verbalize
     * @return Natural language string
     */
    std::string verbalize(const Thought& t);
    
    /**
     * Update active concepts based on reasoning
     * @param concepts New active concepts
     */
    void set_active_concepts(const std::vector<uint64_t>& concepts);
    
private:
    AtomicGraph& graph_;
    std::vector<uint64_t> active_concepts_;
    
    // Helper: find strongest connected concept
    uint64_t find_strongest_neighbor(uint64_t node_id);
};

} // namespace melvin


