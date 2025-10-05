#pragma once

#include "melvin_types.h"
#include "storage.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <chrono>


namespace melvin {

// Forward declarations
class Store;

// Pattern structure for mining
struct Pattern {
    std::vector<NodeID> sequence;
    std::vector<Rel> relations;
    uint32_t frequency;
    double confidence;
    std::vector<std::string> contexts;  // Different contexts where this pattern appears
    uint64_t first_seen;
    uint64_t last_seen;
    
    Pattern() : frequency(0), confidence(0.0), first_seen(0), last_seen(0) {}
};

// N-gram statistics
struct NGramStats {
    std::string pattern_string;
    uint32_t count;
    uint32_t unique_contexts;
    std::vector<NodeID> constituent_nodes;
    double diversity_score;
    uint64_t last_updated;
    
    NGramStats() : count(0), unique_contexts(0), diversity_score(0.0), last_updated(0) {}
};

// Mining configuration
struct MiningConfig {
    // Pattern frequency thresholds
    double theta_node = 0.15;           // Novelty threshold for node creation
    double theta_pmi = 1.0;             // PMI threshold for LEAP edge creation
    double theta_pat = 8.0;             // Minimum frequency for pattern creation
    double theta_div = 0.4;             // Minimum diversity for pattern creation
    
    // Mining parameters
    size_t max_pattern_length = 4;      // Maximum n-gram length to mine
    size_t min_pattern_length = 2;      // Minimum n-gram length to mine
    size_t window_size = 100;           // Rolling window size for pattern mining
    double decay_factor = 0.99;         // Decay factor for old patterns
    
    // Generalization parameters
    double min_generalization_confidence = 0.7;
    size_t max_generalizations_per_batch = 10;
    bool enable_hierarchical_generalization = true;
    
    // PMI parameters
    size_t min_cooccurrence_count = 3;
    double max_temporal_distance = 5.0;  // Maximum temporal distance for LEAP edges
    
    MiningConfig() = default;
};

// Pattern miner
class PatternMiner {
private:
    MiningConfig config_;
    std::unordered_map<std::string, NGramStats> ngram_stats_;
    std::vector<Pattern> discovered_patterns_;
    std::queue<std::vector<NodeID>> recent_sequences_;
    
    Store* store_;
    
public:
    PatternMiner(Store* store, const MiningConfig& config = MiningConfig()) 
        : store_(store), config_(config) {}
    
    // Mine patterns from a sequence of nodes
    void mine_sequence(const std::vector<NodeID>& sequence);
    
    // Mine patterns from multiple sequences
    void mine_sequences(const std::vector<std::vector<NodeID>>& sequences);
    
    // Get discovered patterns
    const std::vector<Pattern>& get_patterns() const { return discovered_patterns_; }
    
    // Get patterns above frequency threshold
    std::vector<Pattern> get_frequent_patterns(double min_frequency = -1.0) const;
    
    // Get patterns for generalization
    std::vector<Pattern> get_generalization_candidates() const;
    
    // Update pattern statistics
    void update_pattern_stats();
    
    // Configuration
    void set_config(const MiningConfig& config) { config_ = config; }
    const MiningConfig& get_config() const { return config_; }
    
private:
    // Pattern extraction
    std::vector<Pattern> extract_ngrams(const std::vector<NodeID>& sequence, size_t n) const;
    void update_ngram_stats(const Pattern& pattern, const std::string& context);
    
    // Pattern evaluation
    bool should_generalize_pattern(const Pattern& pattern) const;
    double compute_pattern_diversity(const Pattern& pattern) const;
    double compute_pattern_confidence(const Pattern& pattern) const;
    
    // Context management
    std::string generate_context_signature(const std::vector<NodeID>& sequence) const;
    void update_recent_sequences(const std::vector<NodeID>& sequence);
    void decay_old_patterns();
    
    // Helper methods
    std::string pattern_to_string(const Pattern& pattern) const;
    bool patterns_overlap(const Pattern& pattern1, const Pattern& pattern2) const;
    Rel extract_relation_between_nodes(const NodeID& node1, const NodeID& node2) const;
};

// Generalization node creator
class GeneralizationCreator {
private:
    MiningConfig config_;
    Store* store_;
    
public:
    GeneralizationCreator(Store* store, const MiningConfig& config = MiningConfig())
        : store_(store), config_(config) {}
    
    // Create generalization nodes from patterns
    std::vector<NodeID> create_generalization_nodes(const std::vector<Pattern>& patterns);
    
    // Create a single generalization node
    NodeID create_generalization_node(const Pattern& pattern);
    
    // Connect pattern constituents to generalization node
    void connect_pattern_to_generalization(const Pattern& pattern, const NodeID& gen_node);
    
    // Create hierarchical generalizations
    std::vector<NodeID> create_hierarchical_generalizations(const std::vector<NodeID>& existing_generalizations);
    
    // Configuration
    void set_config(const MiningConfig& config) { config_ = config; }
    const MiningConfig& get_config() const { return config_; }
    
private:
    // Node creation helpers
    std::string generate_generalization_label(const Pattern& pattern) const;
    NodeType determine_generalization_type(const Pattern& pattern) const;
    
    // Connection helpers
    void create_generalization_edges(const NodeID& gen_node, const std::vector<NodeID>& constituents);
    EdgeID create_generalization_edge(const NodeID& constituent, const NodeID& generalization);
    
    // Hierarchy helpers
    bool should_create_hierarchy(const std::vector<NodeID>& generalizations) const;
    std::vector<NodeID> find_hierarchy_candidates(const std::vector<NodeID>& generalizations) const;
};

// PMI (Pointwise Mutual Information) miner
class PMIMiner {
private:
    MiningConfig config_;
    std::unordered_map<NodeID, uint32_t> node_counts_;
    std::unordered_map<std::pair<NodeID, NodeID>, uint32_t> pair_counts_;
    std::unordered_map<NodeID, std::vector<std::pair<NodeID, double>>> cooccurrence_contexts_;
    
    uint32_t total_observations_;
    Store* store_;
    
public:
    PMIMiner(Store* store, const MiningConfig& config = MiningConfig())
        : store_(store), config_(config), total_observations_(0) {}
    
    // Update PMI statistics with new sequence
    void update_pmi_stats(const std::vector<NodeID>& sequence);
    
    // Get LEAP edge candidates
    std::vector<std::pair<NodeID, NodeID>> get_leap_candidates() const;
    
    // Create LEAP edges for high-PMI pairs
    std::vector<EdgeID> create_leap_edges(const std::vector<std::pair<NodeID, NodeID>>& candidates);
    
    // Compute PMI between two nodes
    double compute_pmi(const NodeID& node_a, const NodeID& node_b) const;
    
    // Check if nodes should have LEAP connection
    bool should_create_leap_edge(const NodeID& node_a, const NodeID& node_b) const;
    
    // Get PMI statistics
    size_t get_vocab_size() const { return node_counts_.size(); }
    uint32_t get_total_observations() const { return total_observations_; }
    
    // Configuration
    void set_config(const MiningConfig& config) { config_ = config; }
    const MiningConfig& get_config() const { return config_; }
    
private:
    // PMI computation helpers
    double compute_probability(const NodeID& node) const;
    double compute_joint_probability(const NodeID& node_a, const NodeID& node_b) const;
    bool are_temporally_distant(const NodeID& node_a, const NodeID& node_b) const;
    
    // Statistics management
    void update_node_counts(const std::vector<NodeID>& sequence);
    void update_pair_counts(const std::vector<NodeID>& sequence);
    void update_cooccurrence_contexts(const std::vector<NodeID>& sequence);
    void decay_old_statistics();
    
    // Edge creation helpers
    EdgeID create_leap_edge(const NodeID& src, const NodeID& dst, double pmi_score);
    double compute_edge_weight_from_pmi(double pmi) const;
};

// Causal relationship miner
class CausalMiner {
private:
    MiningConfig config_;
    std::unordered_map<std::pair<NodeID, NodeID>, double> causal_strengths_;
    std::unordered_map<std::pair<NodeID, NodeID>, uint32_t> lead_lag_counts_;
    std::unordered_map<std::pair<NodeID, NodeID>, std::vector<double>> temporal_distances_;
    
    Store* store_;
    
public:
    CausalMiner(Store* store, const MiningConfig& config = MiningConfig())
        : store_(store), config_(config) {}
    
    // Mine causal relationships from temporal sequences
    void mine_causal_relationships(const std::vector<NodeID>& sequence);
    
    // Get discovered causal relationships
    std::vector<std::pair<NodeID, NodeID>> get_causal_relationships(double min_strength = 0.6) const;
    
    // Get causal strength between two nodes
    double get_causal_strength(const NodeID& cause, const NodeID& effect) const;
    
    // Check if relationship is causal
    bool is_causal(const NodeID& cause, const NodeID& effect) const;
    
    // Update edge causal annotations
    void update_causal_annotations();
    
    // Configuration
    void set_config(const MiningConfig& config) { config_ = config; }
    const MiningConfig& get_config() const { return config_; }
    
private:
    // Causal analysis helpers
    void update_lead_lag_statistics(const std::vector<NodeID>& sequence);
    void update_temporal_distances(const std::vector<NodeID>& sequence);
    double compute_causal_strength_from_counts(uint32_t lead_count, uint32_t lag_count) const;
    
    // Statistical tests
    bool passes_causal_test(const std::pair<NodeID, NodeID>& pair) const;
    double compute_granger_causality_score(const std::pair<NodeID, NodeID>& pair) const;
    
    // Edge annotation helpers
    void annotate_edge_with_causal_info(const EdgeID& edge, bool is_causal, double strength);
};

// Main mining orchestrator
class GraphMiner {
private:
    PatternMiner pattern_miner_;
    GeneralizationCreator generalization_creator_;
    PMIMiner pmi_miner_;
    CausalMiner causal_miner_;
    
    MiningConfig config_;
    Store* store_;
    
    // Mining state
    uint64_t last_mining_time_;
    size_t sequences_processed_;
    
public:
    GraphMiner(Store* store, const MiningConfig& config = MiningConfig())
        : pattern_miner_(store, config),
          generalization_creator_(store, config),
          pmi_miner_(store, config),
          causal_miner_(store, config),
          store_(store),
          config_(config),
          last_mining_time_(0),
          sequences_processed_(0) {}
    
    // Main mining interface
    void mine_from_sequence(const std::vector<NodeID>& sequence);
    
    // Batch mining
    void mine_from_sequences(const std::vector<std::vector<NodeID>>& sequences);
    
    // Periodic mining pass
    void run_mining_pass();
    
    // Get mining results
    std::vector<Pattern> get_discovered_patterns() const;
    std::vector<NodeID> get_generalization_nodes() const;
    std::vector<EdgeID> get_leap_edges() const;
    std::vector<std::pair<NodeID, NodeID>> get_causal_relationships() const;
    
    // Configuration
    void set_config(const MiningConfig& config);
    const MiningConfig& get_config() const { return config_; }
    
    // Statistics
    size_t get_sequences_processed() const { return sequences_processed_; }
    uint64_t get_last_mining_time() const { return last_mining_time_; }
    
private:
    // Mining coordination
    void coordinate_mining_components(const std::vector<NodeID>& sequence);
    void update_mining_statistics();
    
    // Result consolidation
    void consolidate_mining_results();
    void validate_mining_results();
};

// Utility functions
namespace MiningUtils {
    // Pattern analysis
    bool is_pattern_significant(const Pattern& pattern, const MiningConfig& config);
    double compute_pattern_novelty(const Pattern& pattern, Store* store);
    bool patterns_are_similar(const Pattern& pattern1, const Pattern& pattern2);
    
    // Node analysis
    bool node_is_frequent(const NodeID& node, Store* store, double threshold = 0.1);
    std::vector<NodeID> find_frequent_nodes(Store* store, double threshold = 0.1);
    
    // Edge analysis
    bool edge_should_be_created(const NodeID& src, const NodeID& dst, Rel relation, double confidence);
    double compute_edge_creation_confidence(const NodeID& src, const NodeID& dst, Rel relation);
    
    // Temporal analysis
    bool is_temporal_sequence(const std::vector<NodeID>& sequence, Store* store);
    std::vector<double> compute_temporal_distances(const std::vector<NodeID>& sequence, Store* store);
    
    // Generalization analysis
    bool should_create_generalization(const Pattern& pattern, const MiningConfig& config);
    NodeType determine_best_generalization_type(const Pattern& pattern);
    
    // Statistical functions
    double compute_entropy(const std::vector<double>& probabilities);
    double compute_mutual_information(const std::vector<std::vector<double>>& joint_probs);
    double compute_chi_square_statistic(const std::vector<std::vector<uint32_t>>& observed);
    
    // Text processing
    std::string nodes_to_string(const std::vector<NodeID>& nodes, Store* store);
    std::vector<std::string> extract_keywords(const std::string& text);
    std::string generate_pattern_label(const Pattern& pattern, Store* store);
}

} // namespace melvin
