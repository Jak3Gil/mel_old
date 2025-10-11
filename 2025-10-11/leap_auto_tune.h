#pragma once

#include "melvin_types.h"
#include "melvin_leap_nodes.h"
#include "predictive_sampler.h"
#include "src/embeddings/embedding_bridge.h"
#include "leap_diagnostic.h"
#include <vector>
#include <string>

namespace melvin {
namespace autotuning {

// ==================== TUNING CONFIGURATION ====================

struct TuningConfig {
    // Lambda sweep parameters
    float lambda_min = 0.2f;
    float lambda_max = 1.2f;
    float lambda_step = 0.2f;
    
    // Entropy threshold sweep parameters
    float entropy_threshold_min = 0.4f;
    float entropy_threshold_max = 0.8f;
    float entropy_threshold_step = 0.1f;
    
    // Learning rate sweep parameters
    float learning_rate_min = 0.01f;
    float learning_rate_max = 0.05f;
    float learning_rate_step = 0.01f;
    
    // Test parameters
    int test_samples_per_config = 5;  // Number of prompts to test per configuration
    
    // Target metrics
    float target_entropy_reduction = 0.2f;
    float target_context_similarity = 0.5f;
    float target_success_rate = 0.6f;
};

// ==================== TUNING RESULTS ====================

struct TuningResult {
    float lambda_graph_bias;
    float leap_entropy_threshold;
    float learning_rate_embeddings;
    
    // Measured metrics
    float mean_entropy_reduction;
    float mean_context_similarity;
    float success_rate;
    float score;  // Combined score for ranking
    
    TuningResult() : lambda_graph_bias(0), leap_entropy_threshold(0),
                     learning_rate_embeddings(0), mean_entropy_reduction(0),
                     mean_context_similarity(0), success_rate(0), score(0) {}
};

struct TuningSummary {
    TuningResult best_config;
    std::vector<TuningResult> all_results;
    
    float initial_entropy_reduction;
    float initial_context_similarity;
    float initial_success_rate;
    
    float final_entropy_reduction;
    float final_context_similarity;
    float final_success_rate;
    
    bool tuning_successful;
    std::string recommendations;
};

// ==================== AUTO-TUNING PROCEDURES ====================

// Step 1: Bias strength sweep
TuningResult tune_lambda_bias_strength(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config,
    const TuningConfig& tuning_config);

// Step 2: Entropy threshold adjustment
TuningResult tune_entropy_threshold(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config,
    const TuningConfig& tuning_config);

// Step 3: Embedding quality check
void check_embedding_quality(
    embeddings::EmbeddingBridge& embedding_bridge,
    bool& needs_retraining,
    float& quality_score);

// Step 4: Activation normalization check
void check_activation_normalization(
    const std::vector<uint64_t>& active_nodes,
    const std::vector<float>& activations,
    bool& needs_normalization);

// Step 5: Feedback gain tuning
TuningResult tune_learning_rate(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config,
    const TuningConfig& tuning_config);

// ==================== COMPREHENSIVE AUTO-TUNING ====================

// Run full auto-tuning procedure
TuningSummary run_comprehensive_auto_tune(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config,
    const TuningConfig& tuning_config = TuningConfig());

// ==================== EMBEDDING QUALITY CHECKS ====================

// Check nearest neighbors for frequent nodes
void check_nearest_token_neighbors(
    embeddings::EmbeddingBridge& embedding_bridge,
    const std::vector<std::string>& test_concepts);

// Verify expected intuitive pairs
struct EmbeddingPair {
    std::string concept_name;
    std::vector<std::string> expected_neighbors;
};

void verify_intuitive_pairs(
    embeddings::EmbeddingBridge& embedding_bridge,
    const std::vector<EmbeddingPair>& pairs,
    float& match_rate);

// ==================== UTILITIES ====================

// Compute combined score from metrics
float compute_tuning_score(
    float entropy_reduction,
    float context_similarity,
    float success_rate,
    const TuningConfig& config);

// Check if configuration meets targets
bool meets_tuning_targets(
    const TuningResult& result,
    const TuningConfig& config);

// Generate recommendations based on tuning results
std::string generate_recommendations(
    const TuningSummary& summary,
    const TuningConfig& config);

// Save tuning results to file
void save_tuning_results(
    const TuningSummary& summary,
    const std::string& filename = "leap_tuning_results.txt");

// Print tuning summary
void print_tuning_summary(const TuningSummary& summary);

// Update config file with tuned parameters
void update_config_file(
    const TuningResult& best_config,
    const std::string& config_file = "src/util/config.h");

} // namespace autotuning
} // namespace melvin

