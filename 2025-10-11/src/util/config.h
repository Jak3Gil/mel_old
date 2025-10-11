/*
 * MELVIN CONFIGURATION - Tunable Thresholds
 * 
 * Provides:
 * - Environment variable and CLI override support
 * - Runtime tunable thresholds for LEAP system
 * - Default values with fallbacks
 */

#pragma once
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace melvin_config {

// ==================== CONFIGURATION PARAMETERS ====================

struct Config {
    // Crowd support thresholds
    float th_support = 3.0f;           // Minimum # of examples for LEAP
    float th_diversity = 2.0f;         // Minimum distinct subjects
    float th_margin = 1.0f;            // Best must beat second by this margin
    
    // Promotion/decay thresholds
    float promote_threshold = 0.7f;    // Weight to promote LEAP to EXACT
    int min_successes = 3;             // Minimum successes before promotion
    float decay_time_hours = 168.0f;   // 1 week default
    
    // Conflict handling
    float penalty = 1.0f;              // Penalty for conflicting LEAPs
    float override_threshold = 8.0f;   // LEAP support needed to override EXACT
    float abstain_margin = 1.0f;       // Margin for abstention
    
    // Path scoring
    float kappa = 0.5f;                // Bonus for EXACT edges
    float mu = 0.7f;                   // Penalty for LEAP edges
    float sigma = 0.2f;                // Coherence bonus
    
    // Leap creation
    float leap_threshold = 0.4f;       // Minimum score for fallback LEAP
    float min_weight = 0.2f;           // Below this, LEAP gets pruned
    
    // Bigram cache
    int cache_topk = 128;              // Top K entries per connector
    int cache_ttl_s = 300;             // Cache TTL in seconds
    
    // Safety guards
    bool allow_multi_attr = false;     // Allow multiple attributes per (X,C)
    bool require_connector = true;     // Require connector in query
    
    // Prediction parameters
    float lambda_graph_bias = 0.5f;    // Graph bias strength (0=Markov only, 1=graph heavy)
    float cluster_min_activation = 0.1f; // Min activation for clustering
    int max_response_tokens = 20;      // Max output length
    float learning_rate = 0.01f;       // Incremental update rate
    float markov_smoothing = 0.001f;   // Laplace smoothing for unseen bigrams
    bool enable_prediction = true;     // Enable predictive generation
    bool log_predictions = false;      // Log raw and biased probabilities
    bool reasoning_only_mode = false;  // Fallback to old system (no prediction)
};

// Global configuration instance
static Config global_config;

// ==================== ENVIRONMENT VARIABLE READING ====================

inline float read_float(const char* env_name, float default_value) {
    const char* env_val = std::getenv(env_name);
    if (env_val) {
        try {
            float val = std::stof(std::string(env_val));
            std::cout << "[CONFIG] " << env_name << " = " << val << " (from env)\n";
            return val;
        } catch (...) {
            std::cerr << "[CONFIG] Warning: Invalid value for " << env_name << ", using default\n";
        }
    }
    return default_value;
}

inline int read_int(const char* env_name, int default_value) {
    const char* env_val = std::getenv(env_name);
    if (env_val) {
        try {
            int val = std::stoi(std::string(env_val));
            std::cout << "[CONFIG] " << env_name << " = " << val << " (from env)\n";
            return val;
        } catch (...) {
            std::cerr << "[CONFIG] Warning: Invalid value for " << env_name << ", using default\n";
        }
    }
    return default_value;
}

// Load configuration from environment variables
inline void load_config() {
    std::cout << "🔧 Loading Configuration...\n";
    
    // Crowd support
    global_config.th_support = read_float("TH_SUPPORT", 3.0f);
    global_config.th_diversity = read_float("TH_DIVERSITY", 2.0f);
    global_config.th_margin = read_float("TH_MARGIN", 1.0f);
    
    // Promotion/decay
    global_config.promote_threshold = read_float("PROMOTE_THRESHOLD", 0.7f);
    global_config.min_successes = read_int("MIN_SUCCESSES", 3);
    global_config.decay_time_hours = read_float("DECAY_TIME_H", 168.0f);
    
    // Conflict handling
    global_config.penalty = read_float("PENALTY", 1.0f);
    global_config.override_threshold = read_float("OVERRIDE_THRESHOLD", 8.0f);
    global_config.abstain_margin = read_float("ABSTAIN_MARGIN", 1.0f);
    
    // Path scoring
    global_config.kappa = read_float("KAPPA", 0.5f);
    global_config.mu = read_float("MU", 0.7f);
    global_config.sigma = read_float("SIGMA", 0.2f);
    
    // LEAP creation
    global_config.leap_threshold = read_float("LEAP_THRESHOLD", 0.4f);
    global_config.min_weight = read_float("MIN_WEIGHT", 0.2f);
    
    // Bigram cache
    global_config.cache_topk = read_int("CACHE_TOPK", 128);
    global_config.cache_ttl_s = read_int("CACHE_TTL_S", 300);
    
    // Safety guards
    global_config.allow_multi_attr = read_int("ALLOW_MULTI_ATTR", 0) != 0;
    global_config.require_connector = read_int("REQUIRE_CONNECTOR", 1) != 0;
    
    // Prediction parameters
    global_config.lambda_graph_bias = read_float("LAMBDA_GRAPH_BIAS", 0.5f);
    global_config.cluster_min_activation = read_float("CLUSTER_MIN_ACTIVATION", 0.1f);
    global_config.max_response_tokens = read_int("MAX_RESPONSE_TOKENS", 20);
    global_config.learning_rate = read_float("LEARNING_RATE", 0.01f);
    global_config.markov_smoothing = read_float("MARKOV_SMOOTHING", 0.001f);
    global_config.enable_prediction = read_int("ENABLE_PREDICTION", 1) != 0;
    global_config.log_predictions = read_int("LOG_PREDICTIONS", 0) != 0;
    global_config.reasoning_only_mode = read_int("REASONING_ONLY", 0) != 0;
    
    std::cout << "✅ Configuration loaded\n\n";
}

// Get current configuration
inline const Config& get_config() {
    return global_config;
}

// Set configuration (for programmatic override)
inline void set_config(const Config& config) {
    global_config = config;
}

// Print current configuration
inline void print_config() {
    std::cout << "\n📋 Current Configuration:\n";
    std::cout << "   Crowd Support:\n";
    std::cout << "      TH_SUPPORT       = " << global_config.th_support << "\n";
    std::cout << "      TH_DIVERSITY     = " << global_config.th_diversity << "\n";
    std::cout << "      TH_MARGIN        = " << global_config.th_margin << "\n";
    std::cout << "   \n";
    std::cout << "   Promotion/Decay:\n";
    std::cout << "      PROMOTE_THRESHOLD = " << global_config.promote_threshold << "\n";
    std::cout << "      MIN_SUCCESSES     = " << global_config.min_successes << "\n";
    std::cout << "      DECAY_TIME_H      = " << global_config.decay_time_hours << "\n";
    std::cout << "   \n";
    std::cout << "   Conflict Handling:\n";
    std::cout << "      PENALTY           = " << global_config.penalty << "\n";
    std::cout << "      OVERRIDE_THRESHOLD = " << global_config.override_threshold << "\n";
    std::cout << "      ABSTAIN_MARGIN    = " << global_config.abstain_margin << "\n";
    std::cout << "   \n";
    std::cout << "   Path Scoring:\n";
    std::cout << "      KAPPA             = " << global_config.kappa << "\n";
    std::cout << "      MU                = " << global_config.mu << "\n";
    std::cout << "      SIGMA             = " << global_config.sigma << "\n";
    std::cout << "   \n";
    std::cout << "   LEAP Creation:\n";
    std::cout << "      LEAP_THRESHOLD    = " << global_config.leap_threshold << "\n";
    std::cout << "      MIN_WEIGHT        = " << global_config.min_weight << "\n";
    std::cout << "   \n";
    std::cout << "   Bigram Cache:\n";
    std::cout << "      CACHE_TOPK        = " << global_config.cache_topk << "\n";
    std::cout << "      CACHE_TTL_S       = " << global_config.cache_ttl_s << "\n";
    std::cout << "   \n";
    std::cout << "   Safety Guards:\n";
    std::cout << "      ALLOW_MULTI_ATTR  = " << (global_config.allow_multi_attr ? "1" : "0") << "\n";
    std::cout << "      REQUIRE_CONNECTOR = " << (global_config.require_connector ? "1" : "0") << "\n";
    std::cout << "   \n";
    std::cout << "   Prediction:\n";
    std::cout << "      LAMBDA_GRAPH_BIAS = " << global_config.lambda_graph_bias << "\n";
    std::cout << "      CLUSTER_MIN_ACT   = " << global_config.cluster_min_activation << "\n";
    std::cout << "      MAX_RESP_TOKENS   = " << global_config.max_response_tokens << "\n";
    std::cout << "      ENABLE_PREDICTION = " << (global_config.enable_prediction ? "1" : "0") << "\n";
    std::cout << "      REASONING_ONLY    = " << (global_config.reasoning_only_mode ? "1" : "0") << "\n";
    std::cout << "      LOG_PREDICTIONS   = " << (global_config.log_predictions ? "1" : "0") << "\n";
    std::cout << "\n";
}

// ==================== USAGE EXAMPLE ====================
/*
   Usage in code:
   
   #include "src/util/config.h"
   
   int main() {
       melvin_config::load_config();
       
       auto cfg = melvin_config::get_config();
       if (support >= cfg.th_support) {
           // Create LEAP
       }
   }
   
   Usage from command line:
   
   $ TH_SUPPORT=4 TH_MARGIN=1.5 ./test_enhanced_leap
   $ PROMOTE_THRESHOLD=0.8 ./demo_guardrails
*/

} // namespace melvin_config

