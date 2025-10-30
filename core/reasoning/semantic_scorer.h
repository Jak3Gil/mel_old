/**
 * @file semantic_scorer.h
 * @brief Dynamic node scoring with adaptive weights
 * 
 * Scores nodes using:
 * score = α*activation + β*semantic_fit + γ*coherence
 * 
 * Weights adapt based on feedback
 */

#ifndef MELVIN_SEMANTIC_SCORER_H
#define MELVIN_SEMANTIC_SCORER_H

#include <vector>
#include <unordered_map>
#include <string>
#include "core/evolution/dynamic_genome.h"

namespace melvin {
namespace reasoning {

/**
 * @brief Scored node result
 */
struct ScoredNode {
    int node_id;
    float activation;
    float semantic_fit;
    float path_coherence;
    float final_score;
    std::vector<int> best_path;  // Path from query to this node
    
    ScoredNode() : 
        node_id(-1), 
        activation(0.0f),
        semantic_fit(0.0f),
        path_coherence(0.0f),
        final_score(0.0f) 
    {}
};

/**
 * @brief Dynamic semantic scorer
 * 
 * Uses genome-controlled weights to blend:
 * - Raw activation (β)
 * - Semantic similarity to query (α)
 * - Path coherence (γ)
 */
class SemanticScorer {
public:
    SemanticScorer();
    ~SemanticScorer() = default;
    
    /**
     * @brief Set genome for adaptive parameters
     */
    void set_genome(melvin::evolution::DynamicGenome* genome) {
        genome_ = genome;
    }
    
    /**
     * @brief Score a single node
     */
    float score_node(
        int node_id,
        float activation,
        const std::vector<float>& node_embedding,
        const std::vector<float>& query_embedding,
        const std::vector<int>& path_from_query
    );
    
    /**
     * @brief Score all activated nodes
     */
    std::vector<ScoredNode> score_all(
        const std::vector<int>& active_nodes,
        const std::unordered_map<int, float>& activations,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        const std::vector<float>& query_embedding,
        const std::unordered_map<int, std::vector<int>>& paths_from_query
    );
    
    /**
     * @brief Update weights based on feedback
     * 
     * If answer was correct, reinforce current balance
     * If incorrect, adjust strategy
     */
    void learn_from_feedback(
        float confidence,
        float coherence,
        bool success
    );
    
    /**
     * @brief Get top N scored nodes
     */
    std::vector<ScoredNode> get_top_n(
        const std::vector<ScoredNode>& scored,
        size_t n
    ) const;
    
private:
    melvin::evolution::DynamicGenome* genome_;
    
    // Scoring helpers
    float compute_semantic_fit(
        const std::vector<float>& node_embedding,
        const std::vector<float>& query_embedding
    ) const;
    
    float compute_path_coherence(
        const std::vector<int>& path,
        const std::unordered_map<int, std::vector<float>>& embeddings
    ) const;
    
    float cosine_similarity(
        const std::vector<float>& a,
        const std::vector<float>& b
    ) const;
};

} // namespace reasoning
} // namespace melvin

#endif // MELVIN_SEMANTIC_SCORER_H

