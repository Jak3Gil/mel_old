#pragma once

#include "melvin_types.h"
#include "storage.h"
#include "scoring.hpp"
#include "learner.hpp"
#include "beam.hpp"
#include "nlg.hpp"
#include "policy.hpp"
#include "miner.hpp"
#include <memory>
#include <string>
#include <vector>

namespace melvin {

// Forward declarations
class Store;
struct Query;

// Main configuration for the graph reasoning system
struct GraphReasoningConfig {
    // Scoring configuration
    ScoringWeights scoring_weights;
    
    // Learning configuration
    LearningParams learning_params;
    
    // Beam search configuration
    BeamParams default_beam_params;
    
    // NLG configuration
    NLGConfig nlg_config;
    
    // Mining configuration
    MiningConfig mining_config;
    
    // System-wide parameters
    double confidence_threshold_low = 0.45;
    double confidence_threshold_high = 0.7;
    bool enable_learning = true;
    bool enable_mining = true;
    bool enable_generalization = true;
    size_t max_response_length = 200;
    
    GraphReasoningConfig() = default;
};

// Main graph reasoning orchestrator
class GraphReasoningSystem {
private:
    // Core components
    std::unique_ptr<Store> store_;
    GraphScorer scorer_;
    GraphLearner learner_;
    BeamSearchEngine beam_engine_;
    GraphNLG nlg_;
    PolicyManager policy_manager_;
    GraphMiner miner_;
    QueryRouter router_;
    
    // Configuration
    GraphReasoningConfig config_;
    
    // State
    bool initialized_;
    uint64_t last_maintenance_time_;
    
public:
    GraphReasoningSystem(const std::string& store_dir, const GraphReasoningConfig& config = GraphReasoningConfig());
    ~GraphReasoningSystem() = default;
    
    // Main reasoning interface
    std::string reason(const std::string& query);
    
    // Reasoning with pre-computed start nodes (for unified system)
    std::string reason_with_start_nodes(const Query& query);
    
    // Learning interface
    void learn(const std::string& text);
    void learn_from_sequence(const std::vector<NodeID>& sequence);
    
    // Maintenance interface
    void run_maintenance_pass();
    void decay_pass();
    
    // Configuration
    void set_config(const GraphReasoningConfig& config);
    const GraphReasoningConfig& get_config() const { return config_; }
    
    // Statistics
    size_t get_node_count() const;
    size_t get_edge_count() const;
    size_t get_path_count() const;
    double get_health_score() const;
    
    // Query processing
    QueryClassification classify_query(const std::string& query_text) const;
    std::vector<BeamPath> search_paths(const std::string& query_text);
    
private:
    // Internal processing
    Query preprocess_query(const std::string& raw_query);
    std::vector<NodeID> find_start_nodes(const Query& query);
    bool find_node_by_token(const std::string& token, NodeID& node_id);
    std::vector<BeamPath> perform_beam_search(const Query& query, const std::vector<NodeID>& start_nodes);
    std::string generate_response(const std::vector<BeamPath>& paths, const Query& query);
    void update_learning(const std::vector<BeamPath>& used_paths, const Query& query);
    
    // Maintenance
    void initialize_system();
    void run_periodic_maintenance();
    bool should_run_maintenance() const;
    
    // Configuration helpers
    void apply_config_to_components();
    void validate_config() const;
};

// Configuration management
class ConfigManager {
public:
    // Load configuration from JSON file
    static GraphReasoningConfig load_config(const std::string& config_file);
    
    // Save configuration to JSON file
    static void save_config(const GraphReasoningConfig& config, const std::string& config_file);
    
    // Get default configuration
    static GraphReasoningConfig get_default_config();
    
    // Validate configuration
    static bool validate_config(const GraphReasoningConfig& config);
    
private:
    static GraphReasoningConfig parse_config_json(const std::string& json_str);
    static std::string config_to_json(const GraphReasoningConfig& config);
};

// Integration with existing Melvin API
class MelvinGraphReasoning {
private:
    std::unique_ptr<GraphReasoningSystem> reasoning_system_;
    
public:
    MelvinGraphReasoning(const std::string& store_dir, const GraphReasoningConfig& config = GraphReasoningConfig());
    ~MelvinGraphReasoning() = default;
    
    // C API compatibility
    const char* reason(const char* query);
    std::string reason_with_start_nodes(const Query& query);
    void learn(const char* text);
    void decay_pass();
    
    // Statistics
    size_t node_count() const;
    size_t edge_count() const;
    size_t path_count() const;
    float health_score() const;
    
    // Configuration
    void set_config(const GraphReasoningConfig& config);
    const GraphReasoningConfig& get_config() const;
    
    // Internal access
    GraphReasoningSystem* get_reasoning_system() { return reasoning_system_.get(); }
};

} // namespace melvin
