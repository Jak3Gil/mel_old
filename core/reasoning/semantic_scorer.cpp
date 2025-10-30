/**
 * @file semantic_scorer.cpp
 * @brief Implementation of dynamic semantic scorer
 */

#include "semantic_scorer.h"
#include <algorithm>
#include <cmath>

namespace melvin {
namespace reasoning {

SemanticScorer::SemanticScorer() : genome_(nullptr) {}

float SemanticScorer::score_node(
    int node_id,
    float activation,
    const std::vector<float>& node_embedding,
    const std::vector<float>& query_embedding,
    const std::vector<int>& path_from_query
) {
    if (!genome_) {
        // Fallback: equal weights if no genome
        return activation * 0.33f;
    }
    
    auto& params = genome_->reasoning_params();
    
    // Compute semantic fit
    float semantic_fit = compute_semantic_fit(node_embedding, query_embedding);
    
    // Compute path coherence
    float path_coherence = 1.0f;  // Default for direct activation
    if (path_from_query.size() > 1) {
        // Decay based on path length
        path_coherence = 1.0f / std::sqrt(static_cast<float>(path_from_query.size()));
    }
    
    // Blend using genome weights
    float score = params.activation_weight * activation +
                  params.semantic_bias_weight * semantic_fit +
                  params.coherence_weight * path_coherence;
    
    return score;
}

std::vector<ScoredNode> SemanticScorer::score_all(
    const std::vector<int>& active_nodes,
    const std::unordered_map<int, float>& activations,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    const std::vector<float>& query_embedding,
    const std::unordered_map<int, std::vector<int>>& paths_from_query
) {
    std::vector<ScoredNode> scored;
    scored.reserve(active_nodes.size());
    
    for (int node_id : active_nodes) {
        ScoredNode snode;
        snode.node_id = node_id;
        
        // Get activation
        auto act_it = activations.find(node_id);
        snode.activation = (act_it != activations.end()) ? act_it->second : 0.0f;
        
        // Get embedding
        auto emb_it = embeddings.find(node_id);
        if (emb_it == embeddings.end() || emb_it->second.empty()) {
            continue;  // Skip nodes without embeddings
        }
        
        // Compute semantic fit
        snode.semantic_fit = compute_semantic_fit(emb_it->second, query_embedding);
        
        // Get path
        auto path_it = paths_from_query.find(node_id);
        if (path_it != paths_from_query.end()) {
            snode.best_path = path_it->second;
            snode.path_coherence = compute_path_coherence(snode.best_path, embeddings);
        } else {
            snode.path_coherence = 0.0f;
        }
        
        // Compute final score
        if (genome_) {
            auto& params = genome_->reasoning_params();
            snode.final_score = params.activation_weight * snode.activation +
                               params.semantic_bias_weight * snode.semantic_fit +
                               params.coherence_weight * snode.path_coherence;
        } else {
            // Fallback: equal weights
            snode.final_score = (snode.activation + snode.semantic_fit + snode.path_coherence) / 3.0f;
        }
        
        scored.push_back(snode);
    }
    
    // Sort by final score (descending)
    std::sort(scored.begin(), scored.end(),
        [](const ScoredNode& a, const ScoredNode& b) {
            return a.final_score > b.final_score;
        });
    
    return scored;
}

void SemanticScorer::learn_from_feedback(
    float confidence,
    float coherence,
    bool success
) {
    if (genome_) {
        genome_->tune_from_feedback(confidence, coherence, success);
    }
}

std::vector<ScoredNode> SemanticScorer::get_top_n(
    const std::vector<ScoredNode>& scored,
    size_t n
) const {
    size_t count = std::min(n, scored.size());
    return std::vector<ScoredNode>(scored.begin(), scored.begin() + count);
}

float SemanticScorer::compute_semantic_fit(
    const std::vector<float>& node_embedding,
    const std::vector<float>& query_embedding
) const {
    return cosine_similarity(node_embedding, query_embedding);
}

float SemanticScorer::compute_path_coherence(
    const std::vector<int>& path,
    const std::unordered_map<int, std::vector<float>>& embeddings
) const {
    if (path.size() < 2) return 1.0f;
    
    // Measure similarity between consecutive nodes in path
    float total_similarity = 0.0f;
    int valid_pairs = 0;
    
    for (size_t i = 0; i < path.size() - 1; i++) {
        auto emb1_it = embeddings.find(path[i]);
        auto emb2_it = embeddings.find(path[i + 1]);
        
        if (emb1_it != embeddings.end() && emb2_it != embeddings.end()) {
            float sim = cosine_similarity(emb1_it->second, emb2_it->second);
            total_similarity += sim;
            valid_pairs++;
        }
    }
    
    if (valid_pairs == 0) return 0.0f;
    
    float avg_similarity = total_similarity / valid_pairs;
    
    // Penalize long paths
    float length_penalty = 1.0f / (1.0f + std::log(static_cast<float>(path.size())));
    
    return avg_similarity * length_penalty;
}

float SemanticScorer::cosine_similarity(
    const std::vector<float>& a,
    const std::vector<float>& b
) const {
    if (a.size() != b.size() || a.empty()) return 0.0f;
    
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    return (denom > 1e-6f) ? (dot / denom) : 0.0f;
}

} // namespace reasoning
} // namespace melvin

