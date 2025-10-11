#pragma once

#include "melvin_types.h"
#include "melvin_leap_nodes.h"
#include "predictive_sampler.h"
#include "src/embeddings/embedding_bridge.h"
#include <string>
#include <vector>
#include <fstream>

namespace melvin {
namespace diagnostic {

// ==================== DIAGNOSTIC METRICS ====================

struct LeapMetrics {
    std::string prompt_pair;          // e.g., "fire→water"
    float entropy_before;             // Entropy before leap
    float entropy_after;              // Entropy after leap
    float entropy_reduction;          // entropy_before - entropy_after
    float mean_bias_strength;         // Average bias applied
    float lambda_graph_bias;          // Current lambda value
    int active_cluster_count;         // Number of clusters active
    float context_similarity;         // Cosine similarity between activation and top tokens
    bool leap_triggered;              // Whether leap was actually triggered
    bool leap_success;                // Whether leap met success criteria
    
    // Top biased tokens
    std::vector<std::pair<std::string, float>> top_biased_tokens;
    
    // Top similar nodes to activation vector
    std::vector<std::pair<std::string, float>> top_similar_nodes;
    
    // Top predicted tokens
    std::vector<std::string> top_predicted_tokens;
    
    // Comments/notes
    std::string comments;
    
    LeapMetrics() : entropy_before(0), entropy_after(0), entropy_reduction(0),
                    mean_bias_strength(0), lambda_graph_bias(0), 
                    active_cluster_count(0), context_similarity(0),
                    leap_triggered(false), leap_success(false) {}
};

// Aggregate statistics across multiple tests
struct DiagnosticSummary {
    float mean_entropy_reduction;
    float mean_context_similarity;
    float avg_cluster_distance;
    float leap_success_rate;
    int total_tests;
    int successful_leaps;
    
    // Health status based on target thresholds
    bool entropy_reduction_healthy;    // >= 0.2
    bool context_similarity_healthy;   // >= 0.5
    bool leap_success_rate_healthy;    // >= 0.6 (60%)
    
    DiagnosticSummary() : mean_entropy_reduction(0), mean_context_similarity(0),
                          avg_cluster_distance(0), leap_success_rate(0),
                          total_tests(0), successful_leaps(0),
                          entropy_reduction_healthy(false),
                          context_similarity_healthy(false),
                          leap_success_rate_healthy(false) {}
};

// ==================== TEST PROMPT PAIRS ====================

struct TestPromptPair {
    std::string concept_a;
    std::string concept_b;
    
    TestPromptPair(const std::string& a, const std::string& b) 
        : concept_a(a), concept_b(b) {}
};

// Standard test prompts for conceptual leap testing
inline std::vector<TestPromptPair> get_standard_test_prompts() {
    return {
        {"fire", "water"},
        {"music", "emotion"},
        {"robot", "person"},
        {"sun", "night"},
        {"anger", "calm"},
        {"bird", "flight"},
        {"tree", "air"},
        {"food", "energy"},
        {"thought", "memory"},
        {"rain", "growth"}
    };
}

// ==================== DIAGNOSTIC FUNCTIONS ====================

// Run a single diagnostic test on a prompt pair
LeapMetrics run_single_diagnostic_test(
    const TestPromptPair& prompt,
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config);

// Run full diagnostic suite on all test prompts
std::vector<LeapMetrics> run_full_diagnostic_suite(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config);

// Compute summary statistics from diagnostic results
DiagnosticSummary compute_diagnostic_summary(
    const std::vector<LeapMetrics>& metrics);

// ==================== LOGGING & REPORTING ====================

// Log diagnostic data to CSV file
void save_diagnostics_csv(
    const std::vector<LeapMetrics>& metrics,
    const std::string& filename = "leap_diagnostics.csv");

// Generate markdown report
void generate_diagnostic_report(
    const std::vector<LeapMetrics>& metrics,
    const DiagnosticSummary& summary,
    const std::string& filename = "leap_tuning_report.md");

// Print detailed metrics to console
void print_diagnostic_metrics(const LeapMetrics& metrics);

// Print summary table to console
void print_diagnostic_summary(const DiagnosticSummary& summary);

// ==================== ENHANCED LOGGING ====================

// Log detailed prediction cycle information
void log_prediction_cycle(
    float entropy_before,
    float entropy_after,
    float avg_graph_bias_strength,
    float lambda_graph_bias,
    int active_cluster_count,
    const std::vector<std::pair<std::string, float>>& top_biased_tokens,
    const std::vector<std::pair<std::string, float>>& top_similar_nodes);

// ==================== HELPER FUNCTIONS ====================

// Find nodes matching concept text (fuzzy search)
std::vector<uint64_t> find_nodes_by_concept(const std::string& concept_text);

// Compute top N similar nodes to an activation vector
std::vector<std::pair<std::string, float>> compute_top_similar_nodes(
    const std::vector<float>& activation_vector,
    int n = 5);

// Compute top N biased tokens from candidates
std::vector<std::pair<std::string, float>> extract_top_biased_tokens(
    const std::vector<Candidate>& candidates,
    int n = 5);

// Compute mean bias strength from candidates
float compute_mean_bias_strength(const std::vector<Candidate>& candidates);

// Check if leap was successful based on metrics
bool check_leap_success(
    float entropy_reduction,
    float context_similarity,
    float entropy_reduction_threshold = 0.2f,
    float similarity_threshold = 0.5f);

} // namespace diagnostic
} // namespace melvin

