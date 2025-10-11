#include "leap_diagnostic.h"
#include "melvin_types.h"
#include "text_norm.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

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
extern std::vector<Edge> G_edges;
extern std::unordered_map<uint64_t, std::vector<size_t>> G_adj;

namespace melvin {
namespace diagnostic {

// ==================== HELPER FUNCTIONS ====================

std::vector<uint64_t> find_nodes_by_concept(const std::string& concept_text) {
    std::vector<uint64_t> matching_nodes;
    
    std::string lower_concept = concept_text;
    std::transform(lower_concept.begin(), lower_concept.end(), 
                   lower_concept.begin(), ::tolower);
    
    for (const auto& [node_id, node] : G_nodes) {
        std::string lower_text = node.text;
        std::transform(lower_text.begin(), lower_text.end(),
                      lower_text.begin(), ::tolower);
        
        // Exact match or contains
        if (lower_text == lower_concept || lower_text.find(lower_concept) != std::string::npos) {
            matching_nodes.push_back(node_id);
        }
    }
    
    // If no matches, just return empty (will create new nodes if needed)
    return matching_nodes;
}

std::vector<std::pair<std::string, float>> compute_top_similar_nodes(
    const std::vector<float>& activation_vector,
    int n) {
    
    std::vector<std::pair<std::string, float>> results;
    
    if (activation_vector.empty()) return results;
    
    // Compute similarity for all nodes
    std::vector<std::tuple<uint64_t, std::string, float>> similarities;
    
    for (const auto& [node_id, node] : G_nodes) {
        if (node.emb.empty()) continue;
        
        // Compute cosine similarity
        float dot = 0.0f;
        float norm_act = 0.0f;
        float norm_node = 0.0f;
        
        size_t min_size = std::min(activation_vector.size(), node.emb.size());
        for (size_t i = 0; i < min_size; ++i) {
            dot += activation_vector[i] * node.emb[i];
            norm_act += activation_vector[i] * activation_vector[i];
            norm_node += node.emb[i] * node.emb[i];
        }
        
        if (norm_act > 1e-9f && norm_node > 1e-9f) {
            float similarity = dot / (std::sqrt(norm_act) * std::sqrt(norm_node));
            similarities.push_back({node_id, node.text, similarity});
        }
    }
    
    // Sort by similarity (descending)
    std::sort(similarities.begin(), similarities.end(),
             [](const auto& a, const auto& b) {
                 return std::get<2>(a) > std::get<2>(b);
             });
    
    // Extract top N
    for (int i = 0; i < std::min(n, (int)similarities.size()); ++i) {
        results.push_back({std::get<1>(similarities[i]), std::get<2>(similarities[i])});
    }
    
    return results;
}

std::vector<std::pair<std::string, float>> extract_top_biased_tokens(
    const std::vector<Candidate>& candidates,
    int n) {
    
    std::vector<std::pair<std::string, float>> results;
    
    // Sort by score (descending)
    std::vector<Candidate> sorted_candidates = candidates;
    std::sort(sorted_candidates.begin(), sorted_candidates.end(),
             [](const Candidate& a, const Candidate& b) {
                 return a.score > b.score;
             });
    
    // Extract top N
    for (int i = 0; i < std::min(n, (int)sorted_candidates.size()); ++i) {
        std::string token = "";
        if (G_nodes.count(sorted_candidates[i].node_id) > 0) {
            token = G_nodes[sorted_candidates[i].node_id].text;
        }
        results.push_back({token, sorted_candidates[i].score});
    }
    
    return results;
}

float compute_mean_bias_strength(const std::vector<Candidate>& candidates) {
    if (candidates.empty()) return 0.0f;
    
    float sum = 0.0f;
    for (const auto& c : candidates) {
        sum += std::abs(c.score);
    }
    
    return sum / candidates.size();
}

bool check_leap_success(
    float entropy_reduction,
    float context_similarity,
    float entropy_reduction_threshold,
    float similarity_threshold) {
    
    return (entropy_reduction >= entropy_reduction_threshold) &&
           (context_similarity >= similarity_threshold);
}

// ==================== DIAGNOSTIC TESTS ====================

LeapMetrics run_single_diagnostic_test(
    const TestPromptPair& prompt,
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config) {
    
    LeapMetrics metrics;
    metrics.prompt_pair = prompt.concept_a + "→" + prompt.concept_b;
    metrics.lambda_graph_bias = config.lambda_graph_bias;
    
    std::cout << "\n🧪 Testing: " << metrics.prompt_pair << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    // Find nodes for concepts
    auto nodes_a = find_nodes_by_concept(prompt.concept_a);
    auto nodes_b = find_nodes_by_concept(prompt.concept_b);
    
    if (nodes_a.empty()) {
        std::cout << "⚠️  Warning: No nodes found for '" << prompt.concept_a << "'\n";
        metrics.comments = "Missing concept A nodes";
        return metrics;
    }
    
    // Build context from concept A
    std::vector<uint64_t> context;
    context.insert(context.end(), nodes_a.begin(), nodes_a.end());
    
    // Generate candidates using score_neighbors
    if (context.empty()) {
        std::cout << "⚠️  Warning: Empty context\n";
        metrics.comments = "Empty context";
        return metrics;
    }
    
    uint64_t current_node = context.back();
    std::vector<Candidate> candidates = score_neighbors(current_node, context, config);
    
    if (candidates.empty()) {
        std::cout << "⚠️  Warning: No candidates generated\n";
        metrics.comments = "No candidates";
        return metrics;
    }
    
    // Measure entropy BEFORE leap
    metrics.entropy_before = compute_entropy(candidates);
    std::cout << "📊 Entropy (before): " << std::fixed << std::setprecision(3) 
              << metrics.entropy_before << "\n";
    
    // Check if leap should trigger
    metrics.leap_triggered = leap_controller.should_trigger_leap(candidates, context);
    std::cout << "🧠 Leap triggered: " << (metrics.leap_triggered ? "YES" : "NO") << "\n";
    
    // Apply leap if triggered
    if (metrics.leap_triggered) {
        // Create leap node
        LeapNode leap = leap_controller.create_leap_node(context, candidates);
        
        if (!leap.members.empty()) {
            metrics.active_cluster_count = leap.source_clusters.size();
            
            // Compute leap bias
            LeapBiasField bias_field = leap_controller.compute_leap_bias(leap);
            
            // Apply bias to candidates
            leap_controller.apply_leap_bias_to_candidates(candidates, bias_field);
            
            std::cout << "📌 Active clusters: " << metrics.active_cluster_count << "\n";
            std::cout << "🎯 Leap members: " << leap.members.size() << "\n";
        }
    }
    
    // Apply embedding bridge bias
    if (config.enable_embedding_bridge) {
        std::vector<float> activations(context.size(), 1.0f / context.size());
        
        auto activation_vec = embedding_bridge.compute_activation_vector(
            context, activations);
        
        // Get candidate tokens
        std::vector<std::string> candidate_tokens;
        for (const auto& cand : candidates) {
            if (G_nodes.count(cand.node_id) > 0) {
                candidate_tokens.push_back(
                    embeddings::node_to_token(G_nodes[cand.node_id].text));
            }
        }
        
        // Compute token bias
        auto bias_field = embedding_bridge.compute_token_bias(
            activation_vec, candidate_tokens);
        
        // Apply bias to candidates
        for (size_t i = 0; i < candidates.size() && i < candidate_tokens.size(); ++i) {
            float bias = bias_field.get_bias(candidate_tokens[i]);
            candidates[i].score += bias;
        }
        
        // Compute context similarity
        if (!candidate_tokens.empty()) {
            float max_similarity = 0.0f;
            for (const auto& token : candidate_tokens) {
                float bias = bias_field.get_bias(token);
                max_similarity = std::max(max_similarity, bias / config.lambda_graph_bias);
            }
            metrics.context_similarity = max_similarity;
        }
        
        // Find top similar nodes
        metrics.top_similar_nodes = compute_top_similar_nodes(
            activation_vec.embedding, 5);
        
        std::cout << "🌉 Context similarity: " << std::fixed << std::setprecision(3)
                  << metrics.context_similarity << "\n";
    }
    
    // Normalize candidates
    softmax_inplace(candidates);
    
    // Measure entropy AFTER leap
    metrics.entropy_after = compute_entropy(candidates);
    metrics.entropy_reduction = metrics.entropy_before - metrics.entropy_after;
    
    std::cout << "📊 Entropy (after): " << std::fixed << std::setprecision(3)
              << metrics.entropy_after << "\n";
    std::cout << "📉 Entropy reduction: " << std::fixed << std::setprecision(3)
              << metrics.entropy_reduction << "\n";
    
    // Extract top biased tokens
    metrics.top_biased_tokens = extract_top_biased_tokens(candidates, 5);
    metrics.mean_bias_strength = compute_mean_bias_strength(candidates);
    
    std::cout << "💪 Mean bias strength: " << std::fixed << std::setprecision(3)
              << metrics.mean_bias_strength << "\n";
    
    // Extract top predicted tokens
    for (int i = 0; i < std::min(5, (int)candidates.size()); ++i) {
        if (G_nodes.count(candidates[i].node_id) > 0) {
            metrics.top_predicted_tokens.push_back(G_nodes[candidates[i].node_id].text);
        }
    }
    
    std::cout << "🎯 Top predictions: ";
    for (size_t i = 0; i < std::min(size_t(3), metrics.top_predicted_tokens.size()); ++i) {
        std::cout << metrics.top_predicted_tokens[i];
        if (i < std::min(size_t(3), metrics.top_predicted_tokens.size()) - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "\n";
    
    // Check success
    metrics.leap_success = check_leap_success(
        metrics.entropy_reduction,
        metrics.context_similarity,
        0.15f,  // Slightly lower threshold for testing
        0.4f);  // Slightly lower threshold for testing
    
    std::cout << "✅ Leap success: " << (metrics.leap_success ? "YES" : "NO") << "\n";
    
    // Add comments based on results
    if (metrics.leap_success) {
        metrics.comments = "Good cluster alignment";
    } else if (metrics.entropy_reduction < 0.1f) {
        metrics.comments = "Low entropy reduction - check bias strength";
    } else if (metrics.context_similarity < 0.3f) {
        metrics.comments = "Low similarity - retrain embeddings";
    } else {
        metrics.comments = "Moderate performance";
    }
    
    return metrics;
}

std::vector<LeapMetrics> run_full_diagnostic_suite(
    LeapController& leap_controller,
    embeddings::EmbeddingBridge& embedding_bridge,
    PredictiveConfig& config) {
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧭 GRAPH-GUIDED PREDICTIVE SYSTEM DIAGNOSTICS       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    
    auto test_prompts = get_standard_test_prompts();
    std::vector<LeapMetrics> all_metrics;
    
    for (const auto& prompt : test_prompts) {
        auto metrics = run_single_diagnostic_test(
            prompt, leap_controller, embedding_bridge, config);
        all_metrics.push_back(metrics);
    }
    
    return all_metrics;
}

DiagnosticSummary compute_diagnostic_summary(
    const std::vector<LeapMetrics>& metrics) {
    
    DiagnosticSummary summary;
    summary.total_tests = metrics.size();
    
    if (metrics.empty()) return summary;
    
    float total_entropy_reduction = 0.0f;
    float total_context_similarity = 0.0f;
    int successful_count = 0;
    
    for (const auto& m : metrics) {
        total_entropy_reduction += m.entropy_reduction;
        total_context_similarity += m.context_similarity;
        if (m.leap_success) successful_count++;
    }
    
    summary.mean_entropy_reduction = total_entropy_reduction / metrics.size();
    summary.mean_context_similarity = total_context_similarity / metrics.size();
    summary.successful_leaps = successful_count;
    summary.leap_success_rate = (float)successful_count / metrics.size();
    
    // Check health based on target thresholds
    summary.entropy_reduction_healthy = (summary.mean_entropy_reduction >= 0.2f);
    summary.context_similarity_healthy = (summary.mean_context_similarity >= 0.5f);
    summary.leap_success_rate_healthy = (summary.leap_success_rate >= 0.6f);
    
    return summary;
}

// ==================== LOGGING & REPORTING ====================

void save_diagnostics_csv(
    const std::vector<LeapMetrics>& metrics,
    const std::string& filename) {
    
    std::ofstream csv(filename);
    if (!csv.is_open()) {
        std::cerr << "❌ Failed to open " << filename << " for writing\n";
        return;
    }
    
    // Write header
    csv << "Prompt,EntropyBefore,EntropyAfter,EntropyReduction,MeanBias,"
        << "Lambda,ClusterCount,ContextSimilarity,LeapTriggered,LeapSuccess,"
        << "TopToken1,TopToken2,TopToken3,Comments\n";
    
    // Write data
    for (const auto& m : metrics) {
        csv << m.prompt_pair << ","
            << m.entropy_before << ","
            << m.entropy_after << ","
            << m.entropy_reduction << ","
            << m.mean_bias_strength << ","
            << m.lambda_graph_bias << ","
            << m.active_cluster_count << ","
            << m.context_similarity << ","
            << (m.leap_triggered ? "1" : "0") << ","
            << (m.leap_success ? "1" : "0") << ",";
        
        // Top tokens
        for (int i = 0; i < 3; ++i) {
            if (i < (int)m.top_predicted_tokens.size()) {
                csv << m.top_predicted_tokens[i];
            }
            if (i < 2) csv << ",";
        }
        
        csv << "," << m.comments << "\n";
    }
    
    csv.close();
    std::cout << "\n💾 Diagnostics saved to " << filename << "\n";
}

void generate_diagnostic_report(
    const std::vector<LeapMetrics>& metrics,
    const DiagnosticSummary& summary,
    const std::string& filename) {
    
    std::ofstream report(filename);
    if (!report.is_open()) {
        std::cerr << "❌ Failed to open " << filename << " for writing\n";
        return;
    }
    
    // Write markdown header
    report << "# 🧭 Graph-Guided Predictive System Diagnostic Report\n\n";
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    report << "**Generated:** " << std::ctime(&time_t) << "\n";
    
    // Summary statistics
    report << "## 📊 Summary Statistics\n\n";
    report << "| Metric | Value | Target | Status |\n";
    report << "|--------|-------|--------|--------|\n";
    report << "| Mean Entropy Reduction | " << std::fixed << std::setprecision(3)
           << summary.mean_entropy_reduction << " | ≥0.20 | "
           << (summary.entropy_reduction_healthy ? "✅" : "❌") << " |\n";
    report << "| Mean Context Similarity | " << std::fixed << std::setprecision(3)
           << summary.mean_context_similarity << " | ≥0.50 | "
           << (summary.context_similarity_healthy ? "✅" : "❌") << " |\n";
    report << "| Leap Success Rate | " << std::fixed << std::setprecision(1)
           << (summary.leap_success_rate * 100) << "% | ≥60% | "
           << (summary.leap_success_rate_healthy ? "✅" : "❌") << " |\n";
    report << "| Total Tests | " << summary.total_tests << " | - | - |\n";
    report << "| Successful Leaps | " << summary.successful_leaps << " | - | - |\n";
    
    // Overall health assessment
    report << "\n## 🏥 System Health\n\n";
    
    bool all_healthy = summary.entropy_reduction_healthy && 
                      summary.context_similarity_healthy &&
                      summary.leap_success_rate_healthy;
    
    if (all_healthy) {
        report << "✅ **Status: HEALTHY** - All metrics within target ranges.\n\n";
    } else {
        report << "⚠️ **Status: NEEDS TUNING** - Some metrics below target.\n\n";
        
        report << "**Recommendations:**\n\n";
        if (!summary.entropy_reduction_healthy) {
            report << "- ⚙️ Increase `lambda_graph_bias` to strengthen graph influence\n";
            report << "- ⚙️ Adjust `leap_entropy_threshold` to trigger leaps more aggressively\n";
        }
        if (!summary.context_similarity_healthy) {
            report << "- 🎓 Retrain embeddings with higher learning rate\n";
            report << "- 🔍 Check embedding alignment between nodes and tokens\n";
        }
        if (!summary.leap_success_rate_healthy) {
            report << "- 🎯 Review cluster cohesion threshold\n";
            report << "- 📈 Increase embedding update frequency\n";
        }
        report << "\n";
    }
    
    // Detailed results table
    report << "## 📋 Detailed Test Results\n\n";
    report << "| Prompt | Entropy↓ | ContextSim | Success | TopTokens | Comments |\n";
    report << "|--------|----------|------------|---------|-----------|----------|\n";
    
    for (const auto& m : metrics) {
        report << "| " << m.prompt_pair << " | "
               << std::fixed << std::setprecision(2) << m.entropy_reduction << " | "
               << std::fixed << std::setprecision(2) << m.context_similarity << " | "
               << (m.leap_success ? "✅" : "❌") << " | ";
        
        // Top tokens
        for (size_t i = 0; i < std::min(size_t(3), m.top_predicted_tokens.size()); ++i) {
            report << m.top_predicted_tokens[i];
            if (i < std::min(size_t(3), m.top_predicted_tokens.size()) - 1) {
                report << ", ";
            }
        }
        
        report << " | " << m.comments << " |\n";
    }
    
    // Configuration snapshot
    report << "\n## ⚙️ Current Configuration\n\n";
    report << "```\n";
    if (!metrics.empty()) {
        report << "lambda_graph_bias = " << metrics[0].lambda_graph_bias << "\n";
    }
    report << "```\n";
    
    report.close();
    std::cout << "📄 Report saved to " << filename << "\n";
}

void print_diagnostic_metrics(const LeapMetrics& metrics) {
    std::cout << "\n┌─────────────────────────────────────────┐\n";
    std::cout << "│ Diagnostic Metrics: " << std::setw(20) << std::left 
              << metrics.prompt_pair << "│\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│ Entropy Before:      " << std::fixed << std::setprecision(3) 
              << std::setw(14) << std::right << metrics.entropy_before << " │\n";
    std::cout << "│ Entropy After:       " << std::fixed << std::setprecision(3)
              << std::setw(14) << std::right << metrics.entropy_after << " │\n";
    std::cout << "│ Entropy Reduction:   " << std::fixed << std::setprecision(3)
              << std::setw(14) << std::right << metrics.entropy_reduction << " │\n";
    std::cout << "│ Mean Bias:           " << std::fixed << std::setprecision(3)
              << std::setw(14) << std::right << metrics.mean_bias_strength << " │\n";
    std::cout << "│ Lambda:              " << std::fixed << std::setprecision(3)
              << std::setw(14) << std::right << metrics.lambda_graph_bias << " │\n";
    std::cout << "│ Cluster Count:       " << std::setw(14) << std::right 
              << metrics.active_cluster_count << " │\n";
    std::cout << "│ Context Similarity:  " << std::fixed << std::setprecision(3)
              << std::setw(14) << std::right << metrics.context_similarity << " │\n";
    std::cout << "│ Leap Success:        " << std::setw(14) << std::right
              << (metrics.leap_success ? "YES" : "NO") << " │\n";
    std::cout << "└─────────────────────────────────────────┘\n";
}

void print_diagnostic_summary(const DiagnosticSummary& summary) {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║            DIAGNOSTIC SUMMARY                         ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════╣\n";
    std::cout << "║ Total Tests:            " << std::setw(26) << std::right 
              << summary.total_tests << " ║\n";
    std::cout << "║ Successful Leaps:       " << std::setw(26) << std::right
              << summary.successful_leaps << " ║\n";
    std::cout << "║ Success Rate:           " << std::fixed << std::setprecision(1)
              << std::setw(24) << std::right 
              << (summary.leap_success_rate * 100) << " % ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════╣\n";
    std::cout << "║ Mean Entropy Reduction: " << std::fixed << std::setprecision(3)
              << std::setw(19) << std::right << summary.mean_entropy_reduction 
              << (summary.entropy_reduction_healthy ? " ✅ ║\n" : " ❌ ║\n");
    std::cout << "║ Mean Context Similarity:" << std::fixed << std::setprecision(3)
              << std::setw(19) << std::right << summary.mean_context_similarity
              << (summary.context_similarity_healthy ? " ✅ ║\n" : " ❌ ║\n");
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    
    bool all_healthy = summary.entropy_reduction_healthy && 
                      summary.context_similarity_healthy &&
                      summary.leap_success_rate_healthy;
    
    if (all_healthy) {
        std::cout << "\n✅ System Status: HEALTHY - All metrics within target ranges\n";
    } else {
        std::cout << "\n⚠️  System Status: NEEDS TUNING - Some metrics below target\n";
    }
}

void log_prediction_cycle(
    float entropy_before,
    float entropy_after,
    float avg_graph_bias_strength,
    float lambda_graph_bias,
    int active_cluster_count,
    const std::vector<std::pair<std::string, float>>& top_biased_tokens,
    const std::vector<std::pair<std::string, float>>& top_similar_nodes) {
    
    std::cout << "\n[LEAP CYCLE]\n";
    std::cout << "  entropy_before=" << std::fixed << std::setprecision(3) 
              << entropy_before << "\n";
    std::cout << "  entropy_after=" << std::fixed << std::setprecision(3)
              << entropy_after << "\n";
    std::cout << "  avg_graph_bias_strength=" << std::fixed << std::setprecision(3)
              << avg_graph_bias_strength << "\n";
    std::cout << "  lambda_graph_bias=" << std::fixed << std::setprecision(3)
              << lambda_graph_bias << "\n";
    std::cout << "  active_cluster_count=" << active_cluster_count << "\n";
    
    std::cout << "  top5_biased_tokens=[";
    for (size_t i = 0; i < std::min(size_t(5), top_biased_tokens.size()); ++i) {
        std::cout << "(" << top_biased_tokens[i].first << ", " 
                  << std::fixed << std::setprecision(3) << top_biased_tokens[i].second << ")";
        if (i < std::min(size_t(5), top_biased_tokens.size()) - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    
    std::cout << "  top5_similar_nodes=[";
    for (size_t i = 0; i < std::min(size_t(5), top_similar_nodes.size()); ++i) {
        std::cout << "(" << top_similar_nodes[i].first << ", "
                  << std::fixed << std::setprecision(3) << top_similar_nodes[i].second << ")";
        if (i < std::min(size_t(5), top_similar_nodes.size()) - 1) std::cout << ", ";
    }
    std::cout << "]\n\n";
}

} // namespace diagnostic
} // namespace melvin

