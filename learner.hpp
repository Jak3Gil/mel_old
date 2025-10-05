#pragma once

#include "melvin_types.h"
#include "storage.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <algorithm>

// Hash function for NodeID pairs is now defined in melvin_types.h

namespace melvin {

// Forward declarations
class Store;
struct EdgeRec;

// Context for learning updates
struct LearningContext {
    Store* store;
    std::vector<NodeID> query_nodes;     // Nodes involved in the query
    std::vector<NodeID> focus_window;    // Recent context window
    uint64_t timestamp;
    bool is_explicit_observation;        // True for direct observations, false for inferred
    
    LearningContext() : store(nullptr), timestamp(0), is_explicit_observation(false) {}
};

// Learning parameters
struct LearningParams {
    // Hebbian reinforcement parameters
    double alpha_core = 1.0;             // Core weight increment for explicit observations
    double alpha_ctx = 0.5;              // Context weight increment
    double alpha_infer = 0.2;            // Weight increment for inferred connections
    
    // Decay parameters
    double lambda_core = 0.001;          // Core weight decay rate
    double lambda_ctx = 0.01;            // Context weight decay rate
    
    // Pruning parameters
    double epsilon_prune = 0.2;          // Minimum weight threshold before pruning
    double epsilon_anchor = 0.1;         // Minimum weight for anchor protection
    
    // Context similarity parameters
    double context_window_size = 10;     // Number of recent nodes to consider for context
    double similarity_threshold = 0.3;   // Minimum similarity for context updates
    
    LearningParams() = default;
};

// Edge learning and update system
class EdgeLearner {
private:
    LearningParams params_;
    std::unordered_map<EdgeID, uint64_t> last_update_times_;
    std::vector<NodeID> recent_context_window_;
    
public:
    EdgeLearner(const LearningParams& params = LearningParams()) : params_(params) {}
    
    // Update edge weights based on usage
    void update_edge_on_use(const EdgeID& edge_id, const LearningContext& context);
    
    // Batch update multiple edges
    void update_edges_on_path(const std::vector<EdgeID>& edge_ids, 
                             const LearningContext& context);
    
    // Decay all edges in the graph
    void decay_all_edges(Store* store);
    
    // Materialize inferred connections
    bool materialize_inferred_edge(const NodeID& src, const NodeID& dst, 
                                  Rel relation, double confidence, Store* store);
    
    // Context similarity computation
    double compute_context_similarity(const std::vector<NodeID>& query_context,
                                    const std::vector<NodeID>& edge_context) const;
    
    // Update context window
    void update_context_window(const std::vector<NodeID>& new_nodes);
    
    // Configuration
    void set_params(const LearningParams& params) { params_ = params; }
    const LearningParams& get_params() const { return params_; }
    
private:
    // Helper methods
    void reinforce_edge_weight(EdgeRec& edge, const LearningContext& context);
    void decay_edge_weight(EdgeRec& edge, double time_delta);
    bool should_prune_edge(const EdgeRec& edge) const;
    EdgeID create_edge_id(const NodeID& src, const NodeID& dst, Rel relation) const;
};

// Pattern learning for generalization
class PatternLearner {
private:
    LearningParams params_;
    std::unordered_map<std::string, uint32_t> ngram_counts_;
    std::unordered_map<std::string, std::unordered_set<std::string>> ngram_contexts_;
    
    // Pattern frequency thresholds
    double theta_pat = 8.0;              // Minimum frequency for pattern creation
    double theta_div = 0.4;              // Minimum diversity for pattern creation
    
public:
    PatternLearner(const LearningParams& params = LearningParams()) : params_(params) {}
    
    // Learn patterns from a sequence of nodes
    void learn_patterns(const std::vector<NodeID>& node_sequence, Store* store);
    
    // Create generalization nodes for frequent patterns
    std::vector<NodeID> create_generalization_nodes(Store* store);
    
    // Check if a pattern should be generalized
    bool should_generalize_pattern(const std::string& pattern) const;
    
    // Get pattern diversity score
    double get_pattern_diversity(const std::string& pattern) const;
    
    // Configuration
    void set_params(const LearningParams& params) {
        params_ = params;
    }
    
    void set_pattern_thresholds(double theta_pat, double theta_div) {
        this->theta_pat = theta_pat;
        this->theta_div = theta_div;
    }
    
private:
    // Helper methods
    std::vector<std::string> extract_ngrams(const std::vector<NodeID>& nodes, Store* store, size_t n) const;
    std::string node_sequence_to_string(const std::vector<NodeID>& nodes, Store* store) const;
    NodeID create_generalization_node(const std::string& pattern, Store* store) const;
};

// PMI (Pointwise Mutual Information) table for LEAP edge creation
class PMITable {
private:
    std::unordered_map<NodeID, uint32_t> node_counts_;
    std::unordered_map<std::pair<NodeID, NodeID>, uint32_t> pair_counts_;
    std::unordered_map<NodeID, std::vector<NodeID>> cooccurrence_contexts_;
    
    uint32_t total_observations_ = 0;
    double theta_pmi_ = 1.0;             // PMI threshold for LEAP edge creation
    
public:
    PMITable(double theta_pmi = 1.0) : theta_pmi_(theta_pmi) {}
    
    // Update PMI table with new observations
    void update(const std::vector<NodeID>& node_sequence);
    
    // Compute PMI between two nodes
    double compute_pmi(const NodeID& node_a, const NodeID& node_b) const;
    
    // Get candidate LEAP edges
    std::vector<std::pair<NodeID, NodeID>> get_leap_candidates() const;
    
    // Check if nodes should have a LEAP connection
    bool should_create_leap_edge(const NodeID& node_a, const NodeID& node_b) const;
    
    // Configuration
    void set_pmi_threshold(double theta_pmi) { theta_pmi_ = theta_pmi; }
    double get_pmi_threshold() const { return theta_pmi_; }
    
    // Statistics
    size_t get_vocab_size() const { return node_counts_.size(); }
    uint32_t get_total_observations() const { return total_observations_; }
    
private:
    // Helper methods
    double compute_probability(const NodeID& node) const;
    double compute_joint_probability(const NodeID& node_a, const NodeID& node_b) const;
    bool are_temporally_adjacent(const NodeID& node_a, const NodeID& node_b) const;
};

// Causal relationship learning
class CausalLearner {
private:
    std::unordered_map<std::pair<NodeID, NodeID>, double> causal_strengths_;
    std::unordered_map<std::pair<NodeID, NodeID>, uint32_t> lead_lag_counts_;
    
    double min_causal_threshold_ = 0.6;  // Minimum causal strength threshold
    
public:
    CausalLearner(double min_threshold = 0.6) : min_causal_threshold_(min_threshold) {}
    
    // Learn causal relationships from temporal sequences
    void learn_causal_patterns(const std::vector<NodeID>& temporal_sequence);
    
    // Get causal strength between two nodes
    double get_causal_strength(const NodeID& cause, const NodeID& effect) const;
    
    // Check if a causal relationship exists
    bool is_causal(const NodeID& cause, const NodeID& effect) const;
    
    // Get all causal relationships
    std::vector<std::pair<NodeID, NodeID>> get_causal_relationships() const;
    
    // Update causal annotations on edges
    void update_edge_causal_annotations(Store* store);
    
    // Configuration
    void set_causal_threshold(double threshold) { min_causal_threshold_ = threshold; }
    
private:
    // Helper methods
    void update_lead_lag_count(const NodeID& earlier, const NodeID& later);
    double compute_causal_strength_from_counts(uint32_t lead_count, uint32_t lag_count) const;
};

// Main learning orchestrator
class GraphLearner {
private:
    EdgeLearner edge_learner_;
    PatternLearner pattern_learner_;
    PMITable pmi_table_;
    CausalLearner causal_learner_;
    
    LearningParams params_;
    
public:
    GraphLearner(const LearningParams& params = LearningParams()) 
        : edge_learner_(params), pattern_learner_(params), params_(params) {}
    
    // Main learning interface
    void learn_from_query(const std::vector<NodeID>& query_nodes, 
                         const std::vector<EdgeID>& used_edges,
                         Store* store, bool is_explicit = true);
    
    void learn_from_sequence(const std::vector<NodeID>& node_sequence, Store* store);
    
    // Periodic maintenance
    void run_maintenance_pass(Store* store);
    
    // Configuration
    void set_params(const LearningParams& params) {
        params_ = params;
        edge_learner_.set_params(params);
        pattern_learner_.set_params(params);
    }
    
    const LearningParams& get_params() const { return params_; }
    
    // Component access
    EdgeLearner& get_edge_learner() { return edge_learner_; }
    PatternLearner& get_pattern_learner() { return pattern_learner_; }
    PMITable& get_pmi_table() { return pmi_table_; }
    CausalLearner& get_causal_learner() { return causal_learner_; }
};

// Utility functions
namespace LearningUtils {
    // Compute time-based decay factor
    double compute_decay_factor(double time_delta, double lambda);
    
    // Compute edge ID from nodes and relation
    EdgeID compute_edge_id(const NodeID& src, const NodeID& dst, Rel relation);
    
    // Check if edge is anchored
    bool is_edge_anchored(const EdgeRec& edge);
    
    // Normalize weight to [0, 1] range
    float normalize_weight(float weight);
    
    // Compute node similarity (for context matching)
    double compute_node_similarity(const NodeID& a, const NodeID& b, Store* store);
}

} // namespace melvin
