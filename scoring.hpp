#pragma once

#include "melvin_types.h"
#include "storage.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>

namespace melvin {

// Forward declarations
class Store;
struct Query;

// Query structure for scoring
struct Query {
    std::vector<std::string> tokens;
    std::vector<float> token_embeddings;
    std::string original_text;
    std::vector<NodeID> focus_nodes;  // Nodes that match query tokens
    uint64_t timestamp;
    
    Query() : timestamp(0) {}
};

// Context for scoring
struct ScoringContext {
    const Query* query;
    Store* store;
    std::vector<NodeID> recent_nodes;  // Recently accessed nodes for context
    float temperature = 1.0f;          // Exploration temperature
    uint64_t current_time;
    
    ScoringContext() : query(nullptr), store(nullptr), current_time(0) {}
};

// Tunable weights for scoring
struct ScoringWeights {
    // Node prior weights
    double beta_text = 1.0;        // Semantic similarity weight
    double beta_freq = 0.3;        // Frequency weight
    double beta_anchor = 2.0;      // Anchor node bonus
    
    // Step scoring weights
    double gamma_rel = 1.0;        // Relation type bias weight
    double gamma_sem = 1.5;        // Semantic similarity weight
    double gamma_ctx = 0.8;        // Context weight weight
    double gamma_core = 1.2;       // Core weight weight
    double gamma_len = 0.1;        // Length penalty weight
    
    // Path scoring weights
    double delta_start = 1.0;      // Starting node weight
    double delta_support = 0.5;    // Support evidence weight
    double delta_redund = 0.3;     // Redundancy penalty weight
    
    // Relation bias multipliers (per query type)
    struct RelBias {
        double exact = 1.0;
        double temporal = 1.0;
        double leap = 1.0;
        double generalize = 1.0;
    };
    
    RelBias define_query;          // For "what is" queries
    RelBias why_query;             // For "why/how" queries
    RelBias compare_query;         // For "compare/which" queries
    
    ScoringWeights() {
        // Default relation biases
        define_query.exact = 1.0;
        define_query.temporal = 0.6;
        define_query.leap = 0.2;
        define_query.generalize = 1.0;
        
        why_query.exact = 0.3;
        why_query.temporal = 1.0;
        why_query.leap = 0.6;
        why_query.generalize = 0.3;
        
        compare_query.exact = 0.8;
        compare_query.temporal = 0.4;
        compare_query.leap = 0.8;
        compare_query.generalize = 1.2;
    }
};

// Node prior scorer
class NodePriorScorer {
private:
    ScoringWeights weights_;
    
public:
    NodePriorScorer(const ScoringWeights& weights = ScoringWeights()) : weights_(weights) {}
    
    // Compute node prior probability given query
    double compute_prior(const NodeID& node, const Query& query, Store* store) const;
    
    // Compute semantic similarity between query and node
    double compute_semantic_similarity(const std::vector<float>& query_embedding, 
                                     const NodeID& node, Store* store) const;
    
    // Compute frequency-based score
    double compute_frequency_score(const NodeID& node, Store* store) const;
    
    // Check if node is anchored
    bool is_anchored(const NodeID& node, Store* store) const;
    
    void set_weights(const ScoringWeights& weights) { weights_ = weights; }
};

// Step scorer for individual edge traversal
class StepScorer {
private:
    ScoringWeights weights_;
    
public:
    StepScorer(const ScoringWeights& weights = ScoringWeights()) : weights_(weights) {}
    
    // Score a single edge step given query context
    double score_step(const EdgeRec& edge, const ScoringContext& context) const;
    
    // Compute relation bias based on query type
    double compute_relation_bias(Rel relation, const Query& query) const;
    
    // Compute semantic similarity between edge target and query focus
    double compute_target_similarity(const EdgeRec& edge, const Query& query, Store* store) const;
    
    // Normalize weight values
    double normalize_weight(float weight) const;
    
    void set_weights(const ScoringWeights& weights) { weights_ = weights; }
};

// Path scorer for complete reasoning chains
class PathScorer {
private:
    ScoringWeights weights_;
    
public:
    PathScorer(const ScoringWeights& weights = ScoringWeights()) : weights_(weights) {}
    
    // Score a complete path given query context
    double score_path(const std::vector<EdgeRec>& path_edges, 
                     const NodeID& start_node,
                     const ScoringContext& context) const;
    
    // Compute support evidence (triangulation/votes)
    double compute_support_evidence(const std::vector<EdgeRec>& path_edges, 
                                   const NodeID& start_node, Store* store) const;
    
    // Compute redundancy penalty
    double compute_redundancy_penalty(const std::vector<EdgeRec>& path_edges) const;
    
    // Compute path confidence
    double compute_confidence(const std::vector<EdgeRec>& path_edges,
                             const NodeID& start_node,
                             const ScoringContext& context) const;
    
    void set_weights(const ScoringWeights& weights) { weights_ = weights; }
};

// Combined scoring system
class GraphScorer {
private:
    NodePriorScorer node_prior_scorer_;
    StepScorer step_scorer_;
    PathScorer path_scorer_;
    ScoringWeights weights_;
    
public:
    GraphScorer(const ScoringWeights& weights = ScoringWeights()) 
        : node_prior_scorer_(weights), step_scorer_(weights), path_scorer_(weights), weights_(weights) {}
    
    // Main scoring interface
    double score_node_prior(const NodeID& node, const Query& query, Store* store) const {
        return node_prior_scorer_.compute_prior(node, query, store);
    }
    
    double score_edge_step(const EdgeRec& edge, const ScoringContext& context) const {
        return step_scorer_.score_step(edge, context);
    }
    
    double score_path(const std::vector<EdgeRec>& path_edges, 
                     const NodeID& start_node,
                     const ScoringContext& context) const {
        return path_scorer_.score_path(path_edges, start_node, context);
    }
    
    // Configuration
    void set_weights(const ScoringWeights& weights) {
        weights_ = weights;
        node_prior_scorer_.set_weights(weights);
        step_scorer_.set_weights(weights);
        path_scorer_.set_weights(weights);
    }
    
    const ScoringWeights& get_weights() const { return weights_; }
};

// Utility functions
namespace ScoringUtils {
    // Compute cosine similarity between two embeddings
    double cosine_similarity(const std::vector<float>& a, const std::vector<float>& b);
    
    // Compute embedding for text (simplified hash-based)
    std::vector<float> compute_text_embedding(const std::string& text);
    
    // Compute embedding for node content
    std::vector<float> compute_node_embedding(const NodeID& node, Store* store);
    
    // Sigmoid activation
    double sigmoid(double x);
    
    // Softmax normalization
    std::vector<double> softmax(const std::vector<double>& logits);
    
    // Safe log function
    double safe_log(double x);
}

} // namespace melvin
