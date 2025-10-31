/**
 * @file emergent_intelligence.h
 * @brief Simplified unified intelligence based on emergent principles
 * 
 * REPLACES:
 * - unified_intelligence.cpp
 * - cognitive_engine.cpp
 * - unified_reasoning_engine.cpp
 * - reflection_controller_dynamic.cpp
 * - spreading_activation.cpp (parts)
 * 
 * PRINCIPLE: Intelligence emerges from need - cost dynamics.
 * No hardcoded strategies, modes, or structures.
 * Everything self-organizes.
 */

#ifndef MELVIN_EMERGENT_INTELLIGENCE_H
#define MELVIN_EMERGENT_INTELLIGENCE_H

#include "emergent_graph.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace melvin {
namespace emergent {

/**
 * @brief Simple reasoning result (no complex metadata)
 */
struct SimpleResult {
    std::string answer;
    float confidence;
    std::vector<std::string> active_concepts;
    
    SimpleResult() : confidence(0.0f) {}
};

/**
 * @brief Unified emergent intelligence system
 * 
 * SINGLE SIMPLE PIPELINE:
 * 1. Input → Activate nodes
 * 2. Spread activation through emergent graph
 * 3. Connections form/strengthen where need > cost
 * 4. Extract answer from active nodes
 * 5. Learn from feedback (adjust need/cost signals)
 * 
 * No modes, no strategies, no hardcoded logic.
 * Everything emerges.
 */
class EmergentIntelligence {
public:
    EmergentIntelligence();
    ~EmergentIntelligence() = default;
    
    /**
     * @brief Initialize with existing graph (if any)
     */
    void initialize(
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        const std::unordered_map<std::string, int>& word_to_id,
        const std::unordered_map<int, std::string>& id_to_word
    );
    
    /**
     * @brief Process a query - simple unified pipeline
     * 
     * 1. Tokenize query
     * 2. Activate corresponding nodes
     * 3. Let activation spread (connections emerge)
     * 4. Extract answer from active nodes
     */
    SimpleResult reason(const std::string& query);
    
    /**
     * @brief Learn from feedback
     * 
     * Adjusts need/cost signals:
     * - Correct answer → increase need for connections used
     * - Wrong answer → decrease need or increase cost
     */
    void learn(bool correct, const std::vector<int>& nodes_used);
    
    /**
     * @brief Get the emergent graph
     */
    EmergentGraph& graph() { return graph_; }
    const EmergentGraph& graph() const { return graph_; }
    
private:
    // The emergent graph (only structure we need)
    EmergentGraph graph_;
    
    // Word mappings (for tokenization)
    std::unordered_map<std::string, int> word_to_id_;
    std::unordered_map<int, std::string> id_to_word_;
    
    // Simple tokenization
    std::vector<std::string> tokenize(const std::string& query);
    
    // Simple embedding (can be replaced with real model)
    std::vector<float> compute_embedding(const std::vector<std::string>& tokens);
    
    // Answer generation from active nodes
    std::string generate_answer(const std::vector<int>& active_nodes);
    
    // Track which nodes were used (for learning)
    std::vector<int> last_nodes_used_;
};

} // namespace emergent
} // namespace melvin

#endif // MELVIN_EMERGENT_INTELLIGENCE_H

