#pragma once
#include "melvin_types.h"
#include "text_norm.h"
#include <vector>
#include <unordered_map>

// Forward declarations
class LeapController;

namespace melvin {
namespace embeddings {
class EmbeddingBridge;
}
}

namespace melvin {

// Configuration for the predictive sampler
struct PredictiveConfig {
    float alpha = 0.7f;        // freq vs similarity balance (0.6-0.8)
    float beta = 8.0f;         // softmax sharpness (inverse temperature)
    float gamma = 1.0f;        // relation prior scale
    float rel_bias[8] = {      // per-Rel biases
        0.0f,  // EXACT
        0.1f,  // TEMPORAL  
        0.0f,  // LEAP
        0.0f,  // GENERALIZATION
        0.35f, // ISA
        0.25f, // CONSUMES
        0.0f,  // HAS
        0.0f   // CAN
    };
    int ctx_K = 4;            // context window size (nodes)
    int beam_width = 4;        // beam search width
    int max_hops = 24;         // maximum path length
    float top_p = 0.9f;        // top-p sampling threshold
    bool use_beam = true;      // use beam search vs top-p sampling
    float epsilon = 1e-6f;     // flooring to avoid dead ends
    float ngram_bonus = 0.15f; // bonus for n-gram continuation
    
    // Leap system integration
    bool enable_leap_system = true;  // Enable graph-guided predictive leaps
    
    // Embedding bridge integration
    bool enable_embedding_bridge = true;  // Enable semantic embedding bias
    float lambda_graph_bias = 0.5f;       // Strength of embedding-based bias
};

// Candidate node with score and relation
struct Candidate {
    uint64_t node_id;          // Node ID (B)
    float score;               // unnormalized score
    Rel rel;                   // relation type
    float freq_ratio;          // frequency ratio from edge
    
    Candidate() : node_id(0), score(0.0f), rel(Rel::TEMPORAL), freq_ratio(0.0f) {}
    Candidate(uint64_t n, float s, Rel r, float f) 
        : node_id(n), score(s), rel(r), freq_ratio(f) {}
};

// Context embedding computation
std::vector<float> compute_context_embedding(const std::vector<uint64_t>& recent_nodes);

// Similarity computation between embeddings
float similarity(const std::vector<float>& a, const std::vector<float>& b);

// Score all neighbors of a node given context
std::vector<Candidate> score_neighbors(
    uint64_t current_node,
    const std::vector<uint64_t>& context,
    const PredictiveConfig& cfg);

// Apply n-gram continuation bonus using Thought nodes
void apply_ngram_bonus(
    const std::vector<uint64_t>& context,
    std::vector<Candidate>& candidates,
    const PredictiveConfig& cfg);

// Convert scores to probabilities (softmax)
void softmax_inplace(std::vector<Candidate>& candidates);

// Generate path using beam search
std::vector<uint64_t> beam_search(
    const std::vector<uint64_t>& seed_context,
    const PredictiveConfig& cfg,
    LeapController* leap_controller = nullptr,
    embeddings::EmbeddingBridge* embedding_bridge = nullptr);

// Generate path using top-p sampling
std::vector<uint64_t> top_p_sample(
    const std::vector<uint64_t>& seed_context,
    const PredictiveConfig& cfg,
    LeapController* leap_controller = nullptr,
    embeddings::EmbeddingBridge* embedding_bridge = nullptr);

// Main generation function
std::vector<uint64_t> generate_path(
    const std::vector<uint64_t>& seed_context,
    PredictiveConfig& cfg,  // Note: cfg is non-const for adaptive tuning
    LeapController* leap_controller = nullptr,  // Optional leap system integration
    embeddings::EmbeddingBridge* embedding_bridge = nullptr);  // Optional embedding bridge

// Online learning: update frequencies after generation
void update_frequencies_from_path(const std::vector<uint64_t>& path);

// Debug: print probability distribution at each hop
void debug_generation_step(
    uint64_t current_node,
    const std::vector<Candidate>& candidates,
    const std::string& context_text = "");

// Thinking log: dump sampler's top-k neighbors and probabilities
void log_sampler_step(
    uint64_t from_node,
    const std::vector<Candidate>& candidates,
    int step);

// Compute entropy of candidate distribution for adaptive tuning
float compute_entropy(const std::vector<Candidate>& candidates);

// Adaptive tuning: adjust config based on entropy and success
void adaptive_tune_config(PredictiveConfig& cfg, 
                         const std::vector<Candidate>& candidates,
                         bool success);

// Self-reinforcement: strengthen co-activated edges
void reinforce_active_edges(const std::vector<uint64_t>& active_edges,
                           float learning_rate = 0.002f);

// Store successful paths as Thought nodes
void save_thought_node(const std::vector<uint64_t>& path);

// Convert relation enum to string for logging
std::string rel_to_string(Rel rel);

// Get node text for logging
std::string node_text(uint64_t node_id);

// Visualization: dump subgraph around context to .dot file
void dump_subgraph_to_dot(const std::vector<uint64_t>& context, 
                         const std::string& filename = "melvin_graph.dot");

} // namespace melvin
