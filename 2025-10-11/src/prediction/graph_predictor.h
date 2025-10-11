/*
 * MELVIN GRAPH PREDICTOR
 * 
 * Prediction directly from storage.cpp graph structure:
 * - Uses edge counts as transition probabilities (no separate file)
 * - Edge weights already encode how often transitions occur
 * - All data lives in storage.cpp (nodes, edges, adjacency)
 */

#pragma once
#include "../../include/melvin_storage.h"
#include "../../src/util/config.h"
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

namespace melvin_prediction {

using namespace melvin_storage;
using namespace melvin_config;

// ==================== GRAPH-NATIVE PREDICTOR ====================

class GraphPredictor {
public:
    // Predict next token using graph edges as transition probabilities
    std::vector<std::pair<int, float>> predict_next(const std::vector<int>& context, int top_k = 10) {
        std::vector<std::pair<int, float>> predictions;
        
        if (context.empty()) return predictions;
        
        int current_node = context.back();
        if (current_node < 0 || current_node >= (int)nodes.size()) return predictions;
        
        // Get outgoing edges from current node
        auto it = adjacency.find(current_node);
        if (it == adjacency.end()) return predictions;
        
        // Build probability distribution from edge counts and weights
        std::map<int, float> token_probs;
        float total_weight = 0.0f;
        
        for (int edge_id : it->second) {
            if (edge_id < 0 || edge_id >= (int)edges.size()) continue;
            
            const auto& edge = edges[edge_id];
            int next_token = edge.b;
            
            // Weight = edge.w * edge.count (how strong and how frequent)
            float score = edge.w * std::log(1.0f + edge.count);
            
            token_probs[next_token] += score;
            total_weight += score;
        }
        
        // Normalize to probabilities
        if (total_weight > 0.0f) {
            for (auto& [token, prob] : token_probs) {
                prob /= total_weight;
            }
        }
        
        // Convert to vector and sort
        for (const auto& [token, prob] : token_probs) {
            predictions.push_back({token, prob});
        }
        
        std::sort(predictions.begin(), predictions.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (predictions.size() > (size_t)top_k) {
            predictions.resize(top_k);
        }
        
        return predictions;
    }
    
    // Generate complete response
    std::string generate_response(const std::vector<int>& reasoning_path, 
                                 const std::vector<float>& context_field,
                                 int max_tokens = 20) {
        auto cfg = get_config();
        
        // Form clusters from context
        auto cluster_bias = compute_cluster_bias(context_field);
        
        std::vector<int> generated;
        std::vector<int> context = reasoning_path;
        
        for (int step = 0; step < max_tokens; ++step) {
            // Get predictions from graph
            auto predictions = predict_next(context, 20);
            if (predictions.empty()) break;
            
            // Apply cluster bias
            auto biased = apply_cluster_bias(predictions, cluster_bias, cfg.lambda_graph_bias);
            if (biased.empty()) break;
            
            int token = biased[0].first;
            if (is_stop_token(token)) break;
            
            generated.push_back(token);
            context.push_back(token);
            
            if (context.size() > 10) context.erase(context.begin());
        }
        
        return tokens_to_text(generated);
    }
    
private:
    // Compute cluster bias from activated nodes
    std::map<int, float> compute_cluster_bias(const std::vector<float>& context_field) {
        std::map<int, float> bias;
        auto cfg = get_config();
        
        // Bias toward activated nodes and their neighbors
        for (size_t i = 0; i < context_field.size(); ++i) {
            if (context_field[i] >= cfg.cluster_min_activation) {
                bias[i] = context_field[i];
                
                // Spread to neighbors
                auto it = adjacency.find(i);
                if (it != adjacency.end()) {
                    for (int edge_id : it->second) {
                        if (edge_id >= 0 && edge_id < (int)edges.size()) {
                            int neighbor = edges[edge_id].b;
                            bias[neighbor] += context_field[i] * edges[edge_id].w * 0.5f;
                        }
                    }
                }
            }
        }
        
        // Normalize
        float max_bias = 0.0f;
        for (const auto& [token, b] : bias) max_bias = std::max(max_bias, b);
        
        if (max_bias > 0.0f) {
            for (auto& [token, b] : bias) b /= max_bias;
        }
        
        return bias;
    }
    
    // Apply cluster bias to predictions
    std::vector<std::pair<int, float>> apply_cluster_bias(
        const std::vector<std::pair<int, float>>& predictions,
        const std::map<int, float>& cluster_bias,
        float lambda
    ) {
        std::vector<std::pair<int, float>> biased;
        
        for (const auto& [token, prob] : predictions) {
            float bias = 0.0f;
            auto it = cluster_bias.find(token);
            if (it != cluster_bias.end()) bias = it->second;
            
            float biased_prob = prob * (1.0f + lambda * bias);
            biased.push_back({token, biased_prob});
        }
        
        // Renormalize
        float total = 0.0f;
        for (const auto& p : biased) total += p.second;
        
        if (total > 0.0f) {
            for (auto& p : biased) p.second /= total;
        }
        
        std::sort(biased.begin(), biased.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return biased;
    }
    
    bool is_stop_token(int token) const {
        if (token < 0 || token >= (int)nodes.size()) return true;
        const std::string& text = nodes[token].data;
        return text == "." || text == "?" || text == "!";
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
};

} // namespace melvin_prediction

