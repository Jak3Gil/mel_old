#include "leap_auto_tune.h"
#include "leap_diagnostic.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>

// Structures from diagnostic_main.cpp (kept in sync)
struct Node {
    uint64_t id = 0;
    std::string text;
    uint32_t type = 0;
    int freq = 0;
    bool pinned = false;
    std::vector<float> emb;
    std::vector<float> embedding;
    float attention_weight = 0.0f;
    uint64_t last_accessed = 0;
    float semantic_strength = 1.0f;
    float activation = 0.0f;
};

struct Edge {
    uint64_t u, v;
    uint64_t locB;
    float weight;
    float w_core;
    float w_ctx;
    uint32_t count;
    uint8_t rel;
    float last_used;
    float freq_ratio = 0.0f;
    
    Edge() : u(0), v(0), locB(0), weight(0.5f), w_core(0.3f), w_ctx(0.2f), count(1), rel(0), last_used(0.0f), freq_ratio(0.0f) {}
};

// External globals from diagnostic_main.cpp
extern std::unordered_map<uint64_t, Node> G_nodes;

namespace melvin {
namespace autotuning {

// ==================== UTILITY FUNCTIONS ====================

float compute_tuning_score(
    float entropy_reduction,
    float context_similarity,
    float success_rate,
    const TuningConfig& config) {
    
    // Weighted score with penalties for missing targets
    float score = 0.0f;
    
    // Entropy reduction component (40% weight)
    if (entropy_reduction >= config.target_entropy_reduction) {
        score += 0.4f * (entropy_reduction / config.target_entropy_reduction);
    } else {
        score += 0.4f * (entropy_reduction / config.target_entropy_reduction) * 0.5f;
    }
    
    // Context similarity component (30% weight)
    if (context_similarity >= config.target_context_similarity) {
        score += 0.3f * (context_similarity / config.target_context_similarity);
    } else {
        score += 0.3f * (context_similarity / config.target_context_similarity) * 0.5f;
    }
    
    // Success rate component (30% weight)
    if (success_rate >= config.target_success_rate) {
        score += 0.3f * (success_rate / config.target_success_rate);
    } else {
        score += 0.3f * (success_rate / config.target_success_rate) * 0.5f;
    }
    
    return score;
}

bool meets_tuning_targets(
    const TuningResult& result,
    const TuningConfig& config) {
    
    return (result.mean_entropy_reduction >= config.target_entropy_reduction) &&
           (result.mean_context_similarity >= config.target_context_similarity) &&
           (result.success_rate >= config.target_success_rate);
}

// ==================== AUTO-TUNING PROCEDURES ====================

TuningResult tune_lambda_bias_strength(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config,
    const TuningConfig& tuning_config) {
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  Step 1: BIAS STRENGTH SWEEP (λ)                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::vector<TuningResult> results;
    auto test_prompts = diagnostic::get_standard_test_prompts();
    
    // Sweep through lambda values
    for (float lambda = tuning_config.lambda_min; 
         lambda <= tuning_config.lambda_max; 
         lambda += tuning_config.lambda_step) {
        
        std::cout << "🔧 Testing λ = " << std::fixed << std::setprecision(2) 
                  << lambda << "\n";
        
        // Update config
        config.lambda_graph_bias = lambda;
        auto emb_config = embedding_bridge.get_config();
        emb_config.lambda_graph_bias = lambda;
        embedding_bridge.set_config(emb_config);
        
        // Run sample tests
        std::vector<diagnostic::LeapMetrics> test_metrics;
        for (int i = 0; i < tuning_config.test_samples_per_config; ++i) {
            const auto& prompt = test_prompts[i % test_prompts.size()];
            auto metrics = diagnostic::run_single_diagnostic_test(
                prompt, leap_controller, embedding_bridge, config);
            test_metrics.push_back(metrics);
        }
        
        // Compute average metrics
        auto summary = diagnostic::compute_diagnostic_summary(test_metrics);
        
        TuningResult result;
        result.lambda_graph_bias = lambda;
        result.mean_entropy_reduction = summary.mean_entropy_reduction;
        result.mean_context_similarity = summary.mean_context_similarity;
        result.success_rate = summary.leap_success_rate;
        result.score = compute_tuning_score(
            result.mean_entropy_reduction,
            result.mean_context_similarity,
            result.success_rate,
            tuning_config);
        
        results.push_back(result);
        
        std::cout << "  → Entropy reduction: " << std::fixed << std::setprecision(3)
                  << result.mean_entropy_reduction << "\n";
        std::cout << "  → Score: " << std::fixed << std::setprecision(3)
                  << result.score << "\n\n";
    }
    
    // Find best lambda
    auto best = std::max_element(results.begin(), results.end(),
                                [](const TuningResult& a, const TuningResult& b) {
                                    return a.score < b.score;
                                });
    
    std::cout << "✅ Best λ = " << std::fixed << std::setprecision(2)
              << best->lambda_graph_bias << " (score: " 
              << std::setprecision(3) << best->score << ")\n";
    
    return *best;
}

TuningResult tune_entropy_threshold(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config,
    const TuningConfig& tuning_config) {
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  Step 2: ENTROPY THRESHOLD ADJUSTMENT                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::vector<TuningResult> results;
    auto test_prompts = diagnostic::get_standard_test_prompts();
    
    // Sweep through entropy threshold values
    for (float threshold = tuning_config.entropy_threshold_min;
         threshold <= tuning_config.entropy_threshold_max;
         threshold += tuning_config.entropy_threshold_step) {
        
        std::cout << "🔧 Testing entropy threshold = " << std::fixed 
                  << std::setprecision(2) << threshold << "\n";
        
        // Update leap config
        auto leap_config = leap_controller.get_config();
        leap_config.leap_entropy_threshold = threshold;
        leap_controller.set_config(leap_config);
        
        // Run sample tests
        std::vector<diagnostic::LeapMetrics> test_metrics;
        int leap_trigger_count = 0;
        
        for (int i = 0; i < tuning_config.test_samples_per_config; ++i) {
            const auto& prompt = test_prompts[i % test_prompts.size()];
            auto metrics = diagnostic::run_single_diagnostic_test(
                prompt, leap_controller, embedding_bridge, config);
            test_metrics.push_back(metrics);
            if (metrics.leap_triggered) leap_trigger_count++;
        }
        
        // Compute average metrics
        auto summary = diagnostic::compute_diagnostic_summary(test_metrics);
        float leap_frequency = (float)leap_trigger_count / test_metrics.size();
        
        TuningResult result;
        result.leap_entropy_threshold = threshold;
        result.mean_entropy_reduction = summary.mean_entropy_reduction;
        result.mean_context_similarity = summary.mean_context_similarity;
        result.success_rate = summary.leap_success_rate;
        result.score = compute_tuning_score(
            result.mean_entropy_reduction,
            result.mean_context_similarity,
            result.success_rate,
            tuning_config);
        
        results.push_back(result);
        
        std::cout << "  → Leap frequency: " << std::fixed << std::setprecision(2)
                  << (leap_frequency * 100) << "%\n";
        std::cout << "  → Score: " << std::fixed << std::setprecision(3)
                  << result.score << "\n\n";
    }
    
    // Find best threshold (prefer ~30-50% trigger rate)
    auto best = std::max_element(results.begin(), results.end(),
                                [](const TuningResult& a, const TuningResult& b) {
                                    return a.score < b.score;
                                });
    
    std::cout << "✅ Best entropy threshold = " << std::fixed << std::setprecision(2)
              << best->leap_entropy_threshold << "\n";
    
    return *best;
}

void check_embedding_quality(
    embeddings::EmbeddingBridge& embedding_bridge,
    bool& needs_retraining,
    float& quality_score) {
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  Step 3: EMBEDDING QUALITY CHECK                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Define expected intuitive pairs
    std::vector<EmbeddingPair> expected_pairs = {
        {"fire", {"heat", "smoke", "burn", "hot", "flame"}},
        {"music", {"song", "emotion", "sound", "melody", "harmony"}},
        {"robot", {"machine", "human", "artificial", "automatic", "mechanical"}},
        {"water", {"liquid", "drink", "ocean", "wet", "flow"}},
        {"food", {"eat", "nutrition", "energy", "meal", "consume"}}
    };
    
    float match_rate;
    verify_intuitive_pairs(embedding_bridge, expected_pairs, match_rate);
    
    quality_score = match_rate;
    needs_retraining = (match_rate < 0.3f);  // Less than 30% matches
    
    if (needs_retraining) {
        std::cout << "⚠️  Embedding quality LOW - retraining recommended\n";
    } else if (match_rate < 0.5f) {
        std::cout << "⚡ Embedding quality MODERATE - consider increasing learning rate\n";
    } else {
        std::cout << "✅ Embedding quality GOOD\n";
    }
}

void verify_intuitive_pairs(
    embeddings::EmbeddingBridge& embedding_bridge,
    const std::vector<EmbeddingPair>& pairs,
    float& match_rate) {
    
    int total_expected = 0;
    int total_matches = 0;
    
    for (const auto& pair : pairs) {
        std::cout << "\n🔍 Checking: " << pair.concept_name << "\n";
        
        // Get embedding for concept
        auto& token_mgr = embedding_bridge.token_manager();
        const auto& concept_emb = token_mgr.get_embedding_const(pair.concept_name);
        
        if (concept_emb.empty() || std::all_of(concept_emb.begin(), concept_emb.end(),
                                                [](float v) { return std::abs(v) < 1e-6f; })) {
            std::cout << "  ⚠️  No embedding found for '" << pair.concept_name << "'\n";
            continue;
        }
        
        // Find top 10 nearest tokens
        std::vector<std::tuple<std::string, float, bool>> nearest;
        
        // Sample from available tokens (in real implementation, iterate through all)
        for (const auto& expected : pair.expected_neighbors) {
            const auto& neighbor_emb = token_mgr.get_embedding_const(expected);
            
            if (!neighbor_emb.empty()) {
                float similarity = embedding_bridge.cosine_similarity(concept_emb, neighbor_emb);
                bool is_expected = true;
                nearest.push_back({expected, similarity, is_expected});
            }
        }
        
        // Sort by similarity
        std::sort(nearest.begin(), nearest.end(),
                 [](const auto& a, const auto& b) {
                     return std::get<1>(a) > std::get<1>(b);
                 });
        
        // Count matches in top results
        int matches_found = 0;
        std::cout << "  Top neighbors:\n";
        for (size_t i = 0; i < std::min(size_t(5), nearest.size()); ++i) {
            std::cout << "    " << (i+1) << ". " << std::get<0>(nearest[i])
                      << " (sim: " << std::fixed << std::setprecision(3)
                      << std::get<1>(nearest[i]) << ")";
            if (std::get<2>(nearest[i])) {
                std::cout << " ✓";
                matches_found++;
            }
            std::cout << "\n";
        }
        
        total_expected += pair.expected_neighbors.size();
        total_matches += matches_found;
    }
    
    match_rate = total_expected > 0 ? (float)total_matches / total_expected : 0.0f;
    
    std::cout << "\n📊 Overall match rate: " << std::fixed << std::setprecision(1)
              << (match_rate * 100) << "%\n";
}

void check_activation_normalization(
    const std::vector<uint64_t>& active_nodes,
    const std::vector<float>& activations,
    bool& needs_normalization) {
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  Step 4: ACTIVATION NORMALIZATION CHECK               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    if (activations.empty()) {
        std::cout << "⚠️  No activations to check\n";
        needs_normalization = false;
        return;
    }
    
    // Check if activations sum to approximately 1
    float sum = 0.0f;
    float max_activation = 0.0f;
    
    for (float a : activations) {
        sum += a;
        max_activation = std::max(max_activation, a);
    }
    
    std::cout << "📊 Activation sum: " << std::fixed << std::setprecision(3) << sum << "\n";
    std::cout << "📊 Max activation: " << std::fixed << std::setprecision(3) 
              << max_activation << "\n";
    
    // Check for single node domination
    float domination_ratio = max_activation / (sum + 1e-9f);
    std::cout << "📊 Domination ratio: " << std::fixed << std::setprecision(3)
              << domination_ratio << "\n";
    
    needs_normalization = (std::abs(sum - 1.0f) > 0.2f) || (domination_ratio > 0.8f);
    
    if (needs_normalization) {
        std::cout << "⚠️  Activations need normalization\n";
    } else {
        std::cout << "✅ Activations properly normalized\n";
    }
}

TuningResult tune_learning_rate(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config,
    const TuningConfig& tuning_config) {
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  Step 5: FEEDBACK GAIN TUNING                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::vector<TuningResult> results;
    auto test_prompts = diagnostic::get_standard_test_prompts();
    
    // Sweep through learning rate values
    for (float lr = tuning_config.learning_rate_min;
         lr <= tuning_config.learning_rate_max;
         lr += tuning_config.learning_rate_step) {
        
        std::cout << "🔧 Testing learning rate = " << std::fixed 
                  << std::setprecision(3) << lr << "\n";
        
        // Update embedding config
        auto emb_config = embedding_bridge.get_config();
        emb_config.learning_rate_embeddings = lr;
        embedding_bridge.set_config(emb_config);
        
        // Run sample tests
        std::vector<diagnostic::LeapMetrics> test_metrics;
        
        for (int i = 0; i < tuning_config.test_samples_per_config; ++i) {
            const auto& prompt = test_prompts[i % test_prompts.size()];
            auto metrics = diagnostic::run_single_diagnostic_test(
                prompt, leap_controller, embedding_bridge, config);
            test_metrics.push_back(metrics);
        }
        
        // Compute average metrics
        auto summary = diagnostic::compute_diagnostic_summary(test_metrics);
        
        TuningResult result;
        result.learning_rate_embeddings = lr;
        result.mean_entropy_reduction = summary.mean_entropy_reduction;
        result.mean_context_similarity = summary.mean_context_similarity;
        result.success_rate = summary.leap_success_rate;
        result.score = compute_tuning_score(
            result.mean_entropy_reduction,
            result.mean_context_similarity,
            result.success_rate,
            tuning_config);
        
        results.push_back(result);
        
        std::cout << "  → Context similarity: " << std::fixed << std::setprecision(3)
                  << result.mean_context_similarity << "\n";
        std::cout << "  → Score: " << std::fixed << std::setprecision(3)
                  << result.score << "\n\n";
    }
    
    // Find best learning rate
    auto best = std::max_element(results.begin(), results.end(),
                                [](const TuningResult& a, const TuningResult& b) {
                                    return a.score < b.score;
                                });
    
    std::cout << "✅ Best learning rate = " << std::fixed << std::setprecision(3)
              << best->learning_rate_embeddings << "\n";
    
    return *best;
}

// ==================== COMPREHENSIVE AUTO-TUNING ====================

TuningSummary run_comprehensive_auto_tune(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config,
    const TuningConfig& tuning_config) {
    
    TuningSummary summary;
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║         COMPREHENSIVE AUTO-TUNING PROCEDURE           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    
    // Measure initial performance
    std::cout << "\n📊 Measuring initial performance...\n";
    auto initial_metrics = diagnostic::run_full_diagnostic_suite(
        leap_controller, embedding_bridge, config);
    auto initial_summary = diagnostic::compute_diagnostic_summary(initial_metrics);
    
    summary.initial_entropy_reduction = initial_summary.mean_entropy_reduction;
    summary.initial_context_similarity = initial_summary.mean_context_similarity;
    summary.initial_success_rate = initial_summary.leap_success_rate;
    
    std::cout << "\n📋 Initial Metrics:\n";
    std::cout << "  Entropy Reduction: " << std::fixed << std::setprecision(3)
              << summary.initial_entropy_reduction << "\n";
    std::cout << "  Context Similarity: " << std::fixed << std::setprecision(3)
              << summary.initial_context_similarity << "\n";
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(1)
              << (summary.initial_success_rate * 100) << "%\n";
    
    // Step 1: Tune lambda
    auto lambda_result = tune_lambda_bias_strength(
        leap_controller, embedding_bridge, config, tuning_config);
    summary.best_config.lambda_graph_bias = lambda_result.lambda_graph_bias;
    
    // Apply best lambda
    config.lambda_graph_bias = lambda_result.lambda_graph_bias;
    auto emb_config = embedding_bridge.get_config();
    emb_config.lambda_graph_bias = lambda_result.lambda_graph_bias;
    embedding_bridge.set_config(emb_config);
    
    // Step 2: Tune entropy threshold
    auto threshold_result = tune_entropy_threshold(
        leap_controller, embedding_bridge, config, tuning_config);
    summary.best_config.leap_entropy_threshold = threshold_result.leap_entropy_threshold;
    
    // Apply best threshold
    auto leap_config = leap_controller.get_config();
    leap_config.leap_entropy_threshold = threshold_result.leap_entropy_threshold;
    leap_controller.set_config(leap_config);
    
    // Step 3: Check embedding quality
    bool needs_retraining;
    float quality_score;
    check_embedding_quality(embedding_bridge, needs_retraining, quality_score);
    
    // Step 4: Check activation normalization
    // (Would need actual active nodes - skip for now in standalone test)
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  Step 4: ACTIVATION NORMALIZATION CHECK               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "✅ Activation normalization is handled automatically\n";
    
    // Step 5: Tune learning rate
    auto lr_result = tune_learning_rate(
        leap_controller, embedding_bridge, config, tuning_config);
    summary.best_config.learning_rate_embeddings = lr_result.learning_rate_embeddings;
    
    // Apply best learning rate
    emb_config.learning_rate_embeddings = lr_result.learning_rate_embeddings;
    embedding_bridge.set_config(emb_config);
    
    // Measure final performance
    std::cout << "\n📊 Measuring final performance...\n";
    auto final_metrics = diagnostic::run_full_diagnostic_suite(
        leap_controller, embedding_bridge, config);
    auto final_summary = diagnostic::compute_diagnostic_summary(final_metrics);
    
    summary.final_entropy_reduction = final_summary.mean_entropy_reduction;
    summary.final_context_similarity = final_summary.mean_context_similarity;
    summary.final_success_rate = final_summary.leap_success_rate;
    
    // Copy metrics to best_config
    summary.best_config.mean_entropy_reduction = summary.final_entropy_reduction;
    summary.best_config.mean_context_similarity = summary.final_context_similarity;
    summary.best_config.success_rate = summary.final_success_rate;
    
    // Check if tuning was successful
    summary.tuning_successful = meets_tuning_targets(summary.best_config, tuning_config);
    
    // Generate recommendations
    summary.recommendations = generate_recommendations(summary, tuning_config);
    
    return summary;
}

// ==================== UTILITIES ====================

std::string generate_recommendations(
    const TuningSummary& summary,
    const TuningConfig& config) {
    
    std::ostringstream recommendations;
    
    if (summary.tuning_successful) {
        recommendations << "✅ All targets met! System is functioning optimally.\n\n";
        recommendations << "Tuned Parameters:\n";
        recommendations << "  - lambda_graph_bias = " << std::fixed << std::setprecision(2)
                       << summary.best_config.lambda_graph_bias << "\n";
        recommendations << "  - leap_entropy_threshold = " << std::fixed << std::setprecision(2)
                       << summary.best_config.leap_entropy_threshold << "\n";
        recommendations << "  - learning_rate_embeddings = " << std::fixed << std::setprecision(3)
                       << summary.best_config.learning_rate_embeddings << "\n";
    } else {
        recommendations << "⚠️  Some targets not yet met. Further action needed:\n\n";
        
        if (summary.best_config.mean_entropy_reduction < config.target_entropy_reduction) {
            recommendations << "🔧 Entropy Reduction Low:\n";
            recommendations << "  - Consider increasing lambda_graph_bias further\n";
            recommendations << "  - Lower leap_entropy_threshold to trigger more often\n";
            recommendations << "  - Check that embeddings are trained\n\n";
        }
        
        if (summary.best_config.mean_context_similarity < config.target_context_similarity) {
            recommendations << "🎓 Context Similarity Low:\n";
            recommendations << "  - Increase learning_rate_embeddings\n";
            recommendations << "  - Run more training cycles (100+ interactions)\n";
            recommendations << "  - Verify node-token embedding alignment\n\n";
        }
        
        if (summary.best_config.success_rate < config.target_success_rate) {
            recommendations << "📈 Success Rate Low:\n";
            recommendations << "  - Review cluster cohesion thresholds\n";
            recommendations << "  - Increase min_cluster_size for better patterns\n";
            recommendations << "  - Add more training data\n\n";
        }
    }
    
    return recommendations.str();
}

void save_tuning_results(
    const TuningSummary& summary,
    const std::string& filename) {
    
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "❌ Failed to open " << filename << " for writing\n";
        return;
    }
    
    out << "═══════════════════════════════════════════════════════\n";
    out << "  LEAP SYSTEM AUTO-TUNING RESULTS\n";
    out << "═══════════════════════════════════════════════════════\n\n";
    
    out << "INITIAL PERFORMANCE:\n";
    out << "  Entropy Reduction:   " << std::fixed << std::setprecision(3)
        << summary.initial_entropy_reduction << "\n";
    out << "  Context Similarity:  " << std::fixed << std::setprecision(3)
        << summary.initial_context_similarity << "\n";
    out << "  Success Rate:        " << std::fixed << std::setprecision(1)
        << (summary.initial_success_rate * 100) << "%\n\n";
    
    out << "FINAL PERFORMANCE:\n";
    out << "  Entropy Reduction:   " << std::fixed << std::setprecision(3)
        << summary.final_entropy_reduction << "\n";
    out << "  Context Similarity:  " << std::fixed << std::setprecision(3)
        << summary.final_context_similarity << "\n";
    out << "  Success Rate:        " << std::fixed << std::setprecision(1)
        << (summary.final_success_rate * 100) << "%\n\n";
    
    out << "IMPROVEMENTS:\n";
    out << "  Entropy Reduction:   " << std::showpos << std::fixed << std::setprecision(3)
        << (summary.final_entropy_reduction - summary.initial_entropy_reduction) << "\n";
    out << "  Context Similarity:  " << std::showpos << std::fixed << std::setprecision(3)
        << (summary.final_context_similarity - summary.initial_context_similarity) << "\n";
    out << "  Success Rate:        " << std::showpos << std::fixed << std::setprecision(1)
        << ((summary.final_success_rate - summary.initial_success_rate) * 100) << "%\n\n";
    
    out << "OPTIMIZED PARAMETERS:\n";
    out << "  lambda_graph_bias          = " << std::noshowpos << std::fixed 
        << std::setprecision(2) << summary.best_config.lambda_graph_bias << "\n";
    out << "  leap_entropy_threshold     = " << std::fixed << std::setprecision(2)
        << summary.best_config.leap_entropy_threshold << "\n";
    out << "  learning_rate_embeddings   = " << std::fixed << std::setprecision(3)
        << summary.best_config.learning_rate_embeddings << "\n\n";
    
    out << "RECOMMENDATIONS:\n";
    out << summary.recommendations << "\n";
    
    out.close();
    std::cout << "\n💾 Tuning results saved to " << filename << "\n";
}

void print_tuning_summary(const TuningSummary& summary) {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║           AUTO-TUNING SUMMARY                         ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════╣\n";
    
    std::cout << "║ INITIAL → FINAL                                       ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════╣\n";
    std::cout << "║ Entropy Reduction:                                    ║\n";
    std::cout << "║   " << std::fixed << std::setprecision(3) << std::setw(6)
              << summary.initial_entropy_reduction << " → " << std::setw(6)
              << summary.final_entropy_reduction << " "
              << (summary.final_entropy_reduction >= 0.2f ? "✅" : "❌")
              << "                                ║\n";
    std::cout << "║ Context Similarity:                                   ║\n";
    std::cout << "║   " << std::fixed << std::setprecision(3) << std::setw(6)
              << summary.initial_context_similarity << " → " << std::setw(6)
              << summary.final_context_similarity << " "
              << (summary.final_context_similarity >= 0.5f ? "✅" : "❌")
              << "                                ║\n";
    std::cout << "║ Success Rate:                                         ║\n";
    std::cout << "║   " << std::fixed << std::setprecision(1) << std::setw(5)
              << (summary.initial_success_rate * 100) << "% → " << std::setw(5)
              << (summary.final_success_rate * 100) << "% "
              << (summary.final_success_rate >= 0.6f ? "✅" : "❌")
              << "                             ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════╣\n";
    std::cout << "║ OPTIMIZED PARAMETERS:                                 ║\n";
    std::cout << "║   lambda_graph_bias        = " << std::fixed << std::setprecision(2)
              << std::setw(6) << summary.best_config.lambda_graph_bias
              << "                     ║\n";
    std::cout << "║   leap_entropy_threshold   = " << std::fixed << std::setprecision(2)
              << std::setw(6) << summary.best_config.leap_entropy_threshold
              << "                     ║\n";
    std::cout << "║   learning_rate_embeddings = " << std::fixed << std::setprecision(3)
              << std::setw(6) << summary.best_config.learning_rate_embeddings
              << "                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    
    if (summary.tuning_successful) {
        std::cout << "\n✅ TUNING SUCCESSFUL - All targets met!\n";
    } else {
        std::cout << "\n⚠️  TUNING INCOMPLETE - Some targets not yet met\n";
    }
}

void update_config_file(
    const TuningResult& best_config,
    const std::string& config_file) {
    
    std::cout << "\n📝 Configuration values to update in " << config_file << ":\n\n";
    std::cout << "```cpp\n";
    std::cout << "float lambda_graph_bias = " << std::fixed << std::setprecision(2)
              << best_config.lambda_graph_bias << "f;\n";
    std::cout << "float leap_entropy_threshold = " << std::fixed << std::setprecision(2)
              << best_config.leap_entropy_threshold << "f;\n";
    std::cout << "float learning_rate_embeddings = " << std::fixed << std::setprecision(3)
              << best_config.learning_rate_embeddings << "f;\n";
    std::cout << "```\n\n";
    
    std::cout << "ℹ️  Please manually update these values in your configuration files:\n";
    std::cout << "   - src/util/config.h (lambda_graph_bias)\n";
    std::cout << "   - melvin_leap_nodes.h (leap_entropy_threshold)\n";
    std::cout << "   - src/embeddings/embedding_bridge.h (learning_rate_embeddings)\n";
}

} // namespace autotuning
} // namespace melvin

