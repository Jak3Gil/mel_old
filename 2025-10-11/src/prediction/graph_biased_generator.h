/*
 * MELVIN GRAPH-BIASED GENERATOR
 * 
 * Combines Markov prediction with cluster-based graph bias:
 * - Markov handles fluency and token-to-token flow
 * - Graph clusters provide semantic grounding
 * - Energy function balances both influences
 * - Logs raw and biased probabilities for analysis
 */

#pragma once
#include "markov_predictor.h"
#include "semantic_clusters.h"
#include "../../include/melvin_storage.h"
#include "../../src/util/config.h"
#include <string>
#include <vector>
#include <map>
#include <random>
#include <sstream>
#include <iomanip>

namespace melvin_prediction {

using namespace melvin_storage;
using namespace melvin_config;

// ==================== PREDICTION LOG ====================

struct PredictionLog {
    int token;
    float markov_prob;
    float graph_bias;
    float biased_prob;
    float energy;
    bool selected;
};

// ==================== GRAPH-BIASED GENERATOR ====================

class GraphBiasedGenerator {
private:
    MarkovPredictor predictor_;
    ClusterManager clusters_;
    
    // Logging
    std::vector<PredictionLog> last_prediction_log_;
    
    // Random number generator
    mutable std::mt19937 rng_;
    
public:
    GraphBiasedGenerator() : rng_(std::random_device{}()) {}
    
    // ==================== CORE GENERATION ====================
    
    // Generate complete response using hybrid prediction
    std::string generate_response(
        const std::vector<int>& reasoning_path,
        const std::vector<float>& context_field,
        int max_tokens = 20
    ) {
        auto cfg = get_config();
        
        // Form clusters from context field
        auto new_clusters = clusters_.form_clusters(context_field, cfg.cluster_min_activation);
        
        // Merge with cached clusters for multi-turn context
        auto all_clusters = clusters_.merge_with_cache(new_clusters);
        clusters_.cache_clusters(all_clusters);
        
        // Print cluster info if enabled
        if (cfg.log_predictions) {
            clusters_.print_cluster_info(all_clusters);
        }
        
        // Compute graph bias from clusters
        auto cluster_bias = clusters_.compute_cluster_bias(all_clusters);
        
        // Start generation with reasoning path tokens
        std::vector<int> context = reasoning_path;
        std::vector<int> generated;
        
        // Generate tokens one by one
        for (int step = 0; step < max_tokens; ++step) {
            // Get Markov predictions
            auto markov_probs = predictor_.predict_next(context, 20);
            
            if (markov_probs.empty()) break;
            
            // Apply graph bias
            auto biased_probs = apply_graph_bias(markov_probs, cluster_bias, cfg.lambda_graph_bias);
            
            if (biased_probs.empty()) break;
            
            // Sample token (or take argmax)
            int selected_token = sample_token(biased_probs);
            
            // Check for stopping conditions
            if (is_stop_token(selected_token)) break;
            
            // Add to generated sequence
            generated.push_back(selected_token);
            context.push_back(selected_token);
            
            // Limit context window
            if (context.size() > 10) {
                context.erase(context.begin());
            }
        }
        
        // Convert tokens to text
        return tokens_to_text(generated);
    }
    
    // ==================== BIAS APPLICATION ====================
    
    // Apply cluster-based graph bias to Markov probabilities
    std::vector<std::pair<int, float>> apply_graph_bias(
        const std::vector<std::pair<int, float>>& markov_probs,
        const std::map<int, float>& cluster_bias,
        float lambda
    ) {
        last_prediction_log_.clear();
        
        std::vector<std::pair<int, float>> biased_probs;
        
        for (const auto& [token, markov_prob] : markov_probs) {
            // Get graph bias for this token (0 if not in clusters)
            float graph_bias = 0.0f;
            auto it = cluster_bias.find(token);
            if (it != cluster_bias.end()) {
                graph_bias = it->second;
            }
            
            // Compute energy (lower is better)
            float energy = compute_energy(token, markov_prob, graph_bias, lambda);
            
            // Convert energy to probability (softmax-like)
            float biased_prob = markov_prob * (1.0f + lambda * graph_bias);
            
            biased_probs.push_back({token, biased_prob});
            
            // Log for analysis
            PredictionLog log;
            log.token = token;
            log.markov_prob = markov_prob;
            log.graph_bias = graph_bias;
            log.biased_prob = biased_prob;
            log.energy = energy;
            log.selected = false;
            last_prediction_log_.push_back(log);
        }
        
        // Renormalize
        float total = 0.0f;
        for (const auto& p : biased_probs) total += p.second;
        
        if (total > 0.0f) {
            for (auto& p : biased_probs) p.second /= total;
        }
        
        // Sort by probability
        std::sort(biased_probs.begin(), biased_probs.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return biased_probs;
    }
    
    // ==================== ENERGY FUNCTION ====================
    
    // Energy balances predictor probability and graph bias
    // Lower energy = better candidate
    float compute_energy(int token, float markov_prob, float graph_bias, float lambda) const {
        // Energy = -log(P_markov) + lambda * (1 - bias_graph)
        // This favors tokens with high Markov prob AND high graph bias
        
        float markov_term = -std::log(std::max(markov_prob, 1e-6f));
        float graph_term = lambda * (1.0f - graph_bias);
        
        return markov_term + graph_term;
    }
    
    // ==================== SAMPLING ====================
    
    // Sample token from probability distribution
    int sample_token(const std::vector<std::pair<int, float>>& probs) {
        if (probs.empty()) return -1;
        
        // For now, use greedy (argmax) - can add sampling later
        for (auto& log : last_prediction_log_) {
            if (log.token == probs[0].first) {
                log.selected = true;
                break;
            }
        }
        
        return probs[0].first;
    }
    
    // ==================== UTILITIES ====================
    
    bool is_stop_token(int token) const {
        // Stop if we hit certain punctuation or special tokens
        if (token < 0 || token >= (int)nodes.size()) return true;
        
        const std::string& text = nodes[token].data;
        return text == "." || text == "?" || text == "!" || 
               text == "<end>" || text == "</s>";
    }
    
    std::string tokens_to_text(const std::vector<int>& tokens) const {
        std::stringstream ss;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] >= 0 && tokens[i] < (int)nodes.size()) {
                if (i > 0) ss << " ";
                ss << nodes[tokens[i]].data;
            }
        }
        return ss.str();
    }
    
    // ==================== LOGGING ====================
    
    void print_prediction_log(int top_n = 10) const {
        if (last_prediction_log_.empty()) return;
        
        std::cout << "\n📊 Prediction Log (Top " << std::min(top_n, (int)last_prediction_log_.size()) << "):\n";
        std::cout << "   Token              Markov   GraphBias  Biased   Energy  Selected\n";
        std::cout << "   " << std::string(70, '-') << "\n";
        
        for (int i = 0; i < top_n && i < (int)last_prediction_log_.size(); ++i) {
            const auto& log = last_prediction_log_[i];
            
            std::string token_text = "?";
            if (log.token >= 0 && log.token < (int)nodes.size()) {
                token_text = nodes[log.token].data;
            }
            
            std::cout << "   " << std::left << std::setw(18) << token_text
                     << std::fixed << std::setprecision(3)
                     << std::setw(8) << log.markov_prob
                     << std::setw(11) << log.graph_bias
                     << std::setw(8) << log.biased_prob
                     << std::setw(8) << log.energy
                     << (log.selected ? " ✓" : "") << "\n";
        }
        std::cout << "\n";
    }
    
    // ==================== LEARNING INTERFACE ====================
    
    MarkovPredictor& get_predictor() { return predictor_; }
    ClusterManager& get_clusters() { return clusters_; }
    
    const std::vector<PredictionLog>& get_last_log() const { return last_prediction_log_; }
};

} // namespace melvin_prediction

