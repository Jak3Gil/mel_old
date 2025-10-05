#include "scoring.hpp"
#include "storage.h"
#include <algorithm>
#include <numeric>
#include <functional>
#include <random>
#include <chrono>

namespace melvin {

// NodePriorScorer implementation
double NodePriorScorer::compute_prior(const NodeID& node, const Query& query, Store* store) const {
    double semantic_score = 0.0;
    double freq_score = compute_frequency_score(node, store);
    double anchor_bonus = is_anchored(node, store) ? 1.0 : 0.0;
    
    // Compute semantic similarity with query
    if (!query.token_embeddings.empty()) {
        std::vector<float> node_embedding = ScoringUtils::compute_node_embedding(node, store);
        semantic_score = ScoringUtils::cosine_similarity(query.token_embeddings, node_embedding);
    }
    
    double prior = weights_.beta_text * semantic_score +
                   weights_.beta_freq * freq_score +
                   weights_.beta_anchor * anchor_bonus;
    
    return ScoringUtils::sigmoid(prior);
}

double NodePriorScorer::compute_semantic_similarity(const std::vector<float>& query_embedding, 
                                                   const NodeID& node, Store* store) const {
    std::vector<float> node_embedding = ScoringUtils::compute_node_embedding(node, store);
    return ScoringUtils::cosine_similarity(query_embedding, node_embedding);
}

double NodePriorScorer::compute_frequency_score(const NodeID& node, Store* store) const {
    // Get node degree as proxy for frequency
    RelMask all_relations;
    all_relations.set_all();
    
    AdjView view;
    if (store->get_adj(node, all_relations, view)) {
        return ScoringUtils::safe_log(1.0 + view.count);
    }
    
    return 0.0;
}

bool NodePriorScorer::is_anchored(const NodeID& node, Store* store) const {
    NodeRecHeader header;
    std::vector<uint8_t> payload;
    
    if (store->get_node(node, header, payload)) {
        return (header.flags & static_cast<uint32_t>(ANCHOR)) != 0;
    }
    
    return false;
}

// StepScorer implementation
double StepScorer::score_step(const EdgeRec& edge, const ScoringContext& context) const {
    if (!context.query || !context.store) return 0.0;
    
    double rel_bias = compute_relation_bias(static_cast<Rel>(edge.rel), *context.query);
    double target_sim = compute_target_similarity(edge, *context.query, context.store);
    double ctx_weight = normalize_weight(edge.w_ctx);
    double core_weight = normalize_weight(edge.w_core);
    double step_penalty = 1.0; // Base penalty for taking a step
    
    double score = weights_.gamma_rel * rel_bias +
                   weights_.gamma_sem * target_sim +
                   weights_.gamma_ctx * ctx_weight +
                   weights_.gamma_core * core_weight -
                   weights_.gamma_len * step_penalty;
    
    return score;
}

double StepScorer::compute_relation_bias(Rel relation, const Query& query) const {
    // Simple query type detection based on keywords
    std::string lower_query = query.original_text;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    ScoringWeights::RelBias bias;
    
    // Determine query type
    if (lower_query.find("what") != std::string::npos || 
        lower_query.find("define") != std::string::npos ||
        lower_query.find("meaning") != std::string::npos) {
        bias = weights_.define_query;
    } else if (lower_query.find("why") != std::string::npos ||
               lower_query.find("how") != std::string::npos ||
               lower_query.find("cause") != std::string::npos) {
        bias = weights_.why_query;
    } else if (lower_query.find("compare") != std::string::npos ||
               lower_query.find("which") != std::string::npos ||
               lower_query.find("difference") != std::string::npos) {
        bias = weights_.compare_query;
    } else {
        // Default bias
        bias = weights_.define_query;
    }
    
    switch (relation) {
        case Rel::EXACT: return bias.exact;
        case Rel::TEMPORAL: return bias.temporal;
        case Rel::LEAP: return bias.leap;
        case Rel::GENERALIZATION: return bias.generalize;
        default: return 1.0;
    }
}

double StepScorer::compute_target_similarity(const EdgeRec& edge, const Query& query, Store* store) const {
    if (query.token_embeddings.empty()) return 0.5; // Neutral score
    
    NodeID target_node;
    std::copy(edge.dst, edge.dst + 32, target_node.begin());
    
    std::vector<float> target_embedding = ScoringUtils::compute_node_embedding(target_node, store);
    return ScoringUtils::cosine_similarity(query.token_embeddings, target_embedding);
}

double StepScorer::normalize_weight(float weight) const {
    // Sigmoid normalization to [0, 1]
    return ScoringUtils::sigmoid(weight);
}

// PathScorer implementation
double PathScorer::score_path(const std::vector<EdgeRec>& path_edges, 
                             const NodeID& start_node,
                             const ScoringContext& context) const {
    if (path_edges.empty() || !context.query || !context.store) return 0.0;
    
    // Start node prior
    NodePriorScorer node_scorer(weights_);
    double start_score = node_scorer.compute_prior(start_node, *context.query, context.store);
    
    // Sum of step scores
    StepScorer step_scorer(weights_);
    double step_sum = 0.0;
    for (const auto& edge : path_edges) {
        step_sum += step_scorer.score_step(edge, context);
    }
    
    // Support evidence
    double support = compute_support_evidence(path_edges, start_node, context.store);
    
    // Redundancy penalty
    double redundancy = compute_redundancy_penalty(path_edges);
    
    double path_score = weights_.delta_start * start_score +
                       step_sum +
                       weights_.delta_support * support -
                       weights_.delta_redund * redundancy;
    
    return path_score;
}

double PathScorer::compute_support_evidence(const std::vector<EdgeRec>& path_edges, 
                                           const NodeID& start_node, Store* store) const {
    // Count independent paths that support the same conclusion
    // This is a simplified version - in practice, you'd want to check for
    // alternative paths between start and end nodes
    
    if (path_edges.empty()) return 0.0;
    
    NodeID end_node;
    std::copy(path_edges.back().dst, path_edges.back().dst + 32, end_node.begin());
    
    // Count alternative edges to the end node
    RelMask all_relations;
    all_relations.set_all();
    
    AdjView view;
    if (store->get_in_edges(end_node, all_relations, view)) {
        return ScoringUtils::safe_log(1.0 + view.count);
    }
    
    return 0.0;
}

double PathScorer::compute_redundancy_penalty(const std::vector<EdgeRec>& path_edges) const {
    // Count repeated nodes
    std::unordered_set<NodeID, NodeIDHash> visited_nodes;
    double penalty = 0.0;
    
    for (const auto& edge : path_edges) {
        NodeID src, dst;
        std::copy(edge.src, edge.src + 32, src.begin());
        std::copy(edge.dst, edge.dst + 32, dst.begin());
        
        if (visited_nodes.find(src) != visited_nodes.end()) {
            penalty += 0.1; // Penalty for revisiting source
        } else {
            visited_nodes.insert(src);
        }
        
        if (visited_nodes.find(dst) != visited_nodes.end()) {
            penalty += 0.1; // Penalty for revisiting destination
        } else {
            visited_nodes.insert(dst);
        }
    }
    
    return penalty;
}

double PathScorer::compute_confidence(const std::vector<EdgeRec>& path_edges,
                                     const NodeID& start_node,
                                     const ScoringContext& context) const {
    double path_score = score_path(path_edges, start_node, context);
    double support = compute_support_evidence(path_edges, start_node, context.store);
    
    // Combine path score and support evidence for confidence
    double confidence = ScoringUtils::sigmoid(path_score + 0.5 * support);
    return std::min(1.0, std::max(0.0, confidence));
}

// ScoringUtils implementation
namespace ScoringUtils {
    double cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size() || a.empty()) return 0.0;
        
        double dot_product = 0.0;
        double norm_a = 0.0;
        double norm_b = 0.0;
        
        for (size_t i = 0; i < a.size(); ++i) {
            dot_product += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        if (norm_a == 0.0 || norm_b == 0.0) return 0.0;
        
        return dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    
    std::vector<float> compute_text_embedding(const std::string& text) {
        // Simple hash-based embedding (in practice, you'd use a proper embedding model)
        std::hash<std::string> hasher;
        size_t hash = hasher(text);
        
        std::vector<float> embedding(64); // 64-dimensional embedding
        
        // Distribute hash bits across embedding dimensions
        for (size_t i = 0; i < embedding.size(); ++i) {
            embedding[i] = ((hash >> i) & 1) ? 1.0f : -1.0f;
        }
        
        // Add some variation based on character n-grams
        for (size_t i = 0; i < std::min(text.length(), embedding.size()); ++i) {
            embedding[i] += (static_cast<float>(text[i]) / 128.0f - 1.0f) * 0.1f;
        }
        
        return embedding;
    }
    
    std::vector<float> compute_node_embedding(const NodeID& node, Store* store) {
        NodeRecHeader header;
        std::vector<uint8_t> payload;
        
        if (!store->get_node(node, header, payload)) {
            return std::vector<float>(64, 0.0f); // Zero embedding for missing nodes
        }
        
        // Convert payload to string and compute embedding
        std::string content(reinterpret_cast<const char*>(payload.data()), payload.size());
        return compute_text_embedding(content);
    }
    
    double sigmoid(double x) {
        return 1.0 / (1.0 + std::exp(-x));
    }
    
    std::vector<double> softmax(const std::vector<double>& logits) {
        if (logits.empty()) return {};
        
        std::vector<double> result(logits.size());
        double max_logit = *std::max_element(logits.begin(), logits.end());
        double sum_exp = 0.0;
        
        // Compute exponentials and sum
        for (size_t i = 0; i < logits.size(); ++i) {
            result[i] = std::exp(logits[i] - max_logit);
            sum_exp += result[i];
        }
        
        // Normalize
        if (sum_exp > 0.0) {
            for (auto& val : result) {
                val /= sum_exp;
            }
        }
        
        return result;
    }
    
    double safe_log(double x) {
        if (x <= 0.0) return 0.0;
        return std::log(x);
    }
}

} // namespace melvin
