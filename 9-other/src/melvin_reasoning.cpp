#include "melvin_reasoning.h"
#include <algorithm>

namespace melvin {

ReasoningEngine::ReasoningEngine(AtomicGraph& graph)
    : graph_(graph)
{}

std::vector<uint64_t> ReasoningEngine::active_concepts() const {
    return active_concepts_;
}

Thought ReasoningEngine::infer_from_focus(uint64_t focus_node) {
    Thought t;
    t.subj = focus_node;
    t.confidence = 0.5f;
    
    // Find strongest connected concept
    uint64_t related = find_strongest_neighbor(focus_node);
    
    if (related != 0) {
        t.obj = related;
        t.pred_concept = graph_.get_or_create_concept("relates_to");
        t.confidence = 0.8f;
        
        // Update active concepts
        active_concepts_.clear();
        active_concepts_.push_back(focus_node);
        active_concepts_.push_back(related);
    } else {
        // No strong connections, mark as novel
        t.obj = graph_.get_or_create_concept("unknown");
        t.pred_concept = graph_.get_or_create_concept("observes");
        t.confidence = 0.3f;
        
        active_concepts_.clear();
        active_concepts_.push_back(focus_node);
    }
    
    return t;
}

std::string ReasoningEngine::verbalize(const Thought& t) {
    // Simple verbalization
    return "I observe something interesting";
}

void ReasoningEngine::set_active_concepts(const std::vector<uint64_t>& concepts) {
    active_concepts_ = concepts;
}

uint64_t ReasoningEngine::find_strongest_neighbor(uint64_t node_id) {
    auto neighbors = graph_.all_neighbors(node_id, 10);
    
    if (neighbors.empty()) {
        return 0;
    }
    
    // Find neighbor with highest edge weight
    uint64_t best = 0;
    float best_weight = 0;
    
    for (uint64_t n : neighbors) {
        // Check various relation types
        for (Rel r : {Rel::CO_OCCURS_WITH, Rel::SIMILAR_FEATURES, Rel::OBSERVED_AS}) {
            float w = graph_.get_edge_weight(node_id, n, r);
            if (w > best_weight) {
                best_weight = w;
                best = n;
            }
        }
    }
    
    return best;
}

} // namespace melvin


