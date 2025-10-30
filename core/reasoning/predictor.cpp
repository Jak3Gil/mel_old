#include "predictor.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace melvin {
namespace reasoning {

Predictor::Predictor(int embedding_dim)
    : embedding_dim_(embedding_dim)
{
}

std::vector<PredictionResult> Predictor::predict_next(
    const std::vector<int>& context_nodes,
    const ActivationField& activation_field,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    int top_k,
    Mode mode
) {
    if (context_nodes.empty()) {
        return {};
    }
    
    // Try exact recall first (hybrid or exact mode)
    if (mode == Mode::HYBRID || mode == Mode::EXACT) {
        auto exact_results = predict_exact_sequence(context_nodes, top_k);
        if (!exact_results.empty()) {
            return exact_results;
        }
    }
    
    // If exact mode and no matches, return empty
    if (mode == Mode::EXACT) {
        return {};
    }
    
    // Fall back to semantic prediction
    return predict_semantic(context_nodes, activation_field, graph, embeddings, top_k);
}

std::vector<PredictionResult> Predictor::predict_exact_sequence(
    const std::vector<int>& context_nodes,
    int top_k
) {
    // Try decreasing context lengths: 3, 2, 1
    for (int context_len : {3, 2, 1}) {
        if (context_nodes.size() < static_cast<size_t>(context_len)) {
            continue;
        }
        
        // Get last N nodes
        auto start_it = context_nodes.end() - context_len;
        std::vector<int> context(start_it, context_nodes.end());
        
        std::map<int, int> matches;
        
        // Check trigrams
        if (context_len == 3 && context.size() == 3) {
            auto key = std::make_tuple(context[0], context[1], context[2]);
            auto it = trigram_counts_.find(key);
            if (it != trigram_counts_.end() && !it->second.empty()) {
                matches = it->second;
            }
        }
        // Check bigrams
        else if (context_len == 2 && context.size() == 2) {
            auto key = std::make_pair(context[0], context[1]);
            auto it = bigram_counts_.find(key);
            if (it != bigram_counts_.end() && !it->second.empty()) {
                matches = it->second;
            }
        }
        // Check unigrams
        else if (context_len == 1 && context.size() == 1) {
            auto it = transition_counts_.find(context[0]);
            if (it != transition_counts_.end() && !it->second.empty()) {
                matches = it->second;
            }
        }
        
        if (!matches.empty()) {
            // Convert to results
            std::vector<PredictionResult> results;
            int total = 0;
            for (const auto& pair : matches) {
                total += pair.second;
            }
            
            // Sort by count
            std::vector<std::pair<int, int>> sorted_matches(matches.begin(), matches.end());
            std::sort(sorted_matches.begin(), sorted_matches.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (size_t i = 0; i < std::min(sorted_matches.size(), static_cast<size_t>(top_k)); ++i) {
                PredictionResult result;
                result.node_id = sorted_matches[i].first;
                result.confidence = static_cast<float>(sorted_matches[i].second) / total;
                result.score = result.confidence;
                result.source = (context_len == 3) ? "exact_trigram" :
                               (context_len == 2) ? "exact_bigram" : "exact_unigram";
                results.push_back(result);
            }
            
            return results;
        }
    }
    
    return {};
}

std::vector<PredictionResult> Predictor::predict_semantic(
    const std::vector<int>& context_nodes,
    const ActivationField& activation_field,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    int top_k
) {
    // Get all activated nodes (infinite context!)
    auto active_nodes = activation_field.get_active_nodes(0.05f);
    
    if (active_nodes.empty()) {
        // Fall back to local context
        for (size_t i = 0; i < std::min(context_nodes.size(), size_t(5)); ++i) {
            active_nodes[context_nodes[context_nodes.size() - 1 - i]] = 1.0f;
        }
    }
    
    // Collect candidate neighbors from ALL active nodes
    std::unordered_map<int, float> candidates;
    
    for (const auto& active_pair : active_nodes) {
        int context_node = active_pair.first;
        float activation_level = active_pair.second;
        
        // Find in graph
        auto graph_it = graph.find(context_node);
        if (graph_it == graph.end()) {
            continue;
        }
        
        // Recency weight
        float recency_weight = 0.5f;
        auto it = std::find(context_nodes.begin(), context_nodes.end(), context_node);
        if (it != context_nodes.end()) {
            size_t pos = std::distance(context_nodes.begin(), it);
            size_t from_end = context_nodes.size() - pos - 1;
            recency_weight = std::exp(-0.05f * from_end);
        }
        
        // Process neighbors
        for (const auto& edge : graph_it->second) {
            int neighbor_id = edge.first;
            float edge_weight = edge.second;
            
            // Skip if already in context
            if (std::find(context_nodes.begin(), context_nodes.end(), neighbor_id) != context_nodes.end()) {
                continue;
            }
            
            // Get neighbor activation
            float neighbor_activation = 0.1f;
            auto neighbor_it = active_nodes.find(neighbor_id);
            if (neighbor_it != active_nodes.end()) {
                neighbor_activation = neighbor_it->second;
            }
            
            // Combined score
            float score = edge_weight * activation_level * neighbor_activation * recency_weight;
            candidates[neighbor_id] += score;
        }
    }
    
    // Sort and return top-k
    std::vector<std::pair<int, float>> sorted_candidates(candidates.begin(), candidates.end());
    std::sort(sorted_candidates.begin(), sorted_candidates.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<PredictionResult> results;
    float max_score = sorted_candidates.empty() ? 1.0f : sorted_candidates[0].second;
    
    for (size_t i = 0; i < std::min(sorted_candidates.size(), static_cast<size_t>(top_k)); ++i) {
        PredictionResult result;
        result.node_id = sorted_candidates[i].first;
        result.score = sorted_candidates[i].second;
        result.confidence = max_score > 0 ? sorted_candidates[i].second / max_score : 0.0f;
        result.source = "semantic";
        results.push_back(result);
    }
    
    return results;
}

void Predictor::record_sequence(const std::vector<int>& context, int next_node) {
    if (context.empty()) {
        return;
    }
    
    // Record unigram
    if (context.size() >= 1) {
        transition_counts_[context.back()][next_node]++;
    }
    
    // Record bigram
    if (context.size() >= 2) {
        auto key = std::make_pair(context[context.size()-2], context[context.size()-1]);
        bigram_counts_[key][next_node]++;
    }
    
    // Record trigram
    if (context.size() >= 3) {
        auto key = std::make_tuple(
            context[context.size()-3],
            context[context.size()-2],
            context[context.size()-1]
        );
        trigram_counts_[key][next_node]++;
    }
}

void Predictor::update_context_performance(const std::vector<int>& context, bool correct) {
    if (context.empty()) {
        return;
    }
    
    int last_node = context.back();
    context_performance_[last_node].emplace_back(context.size(), correct);
    
    // Keep last 20 observations
    if (context_performance_[last_node].size() > 20) {
        context_performance_[last_node].erase(context_performance_[last_node].begin());
    }
    
    // Update optimal length if enough data
    if (context_performance_[last_node].size() >= 10) {
        std::map<int, std::vector<float>> length_accuracies;
        for (const auto& pair : context_performance_[last_node]) {
            length_accuracies[pair.first].push_back(pair.second ? 1.0f : 0.0f);
        }
        
        int best_length = 3;
        float best_accuracy = 0.0f;
        for (const auto& pair : length_accuracies) {
            float sum = std::accumulate(pair.second.begin(), pair.second.end(), 0.0f);
            float accuracy = sum / pair.second.size();
            if (accuracy > best_accuracy) {
                best_accuracy = accuracy;
                best_length = pair.first;
            }
        }
        
        optimal_context_lengths_[last_node] = best_length;
    }
}

int Predictor::get_optimal_context_length(const std::vector<int>& context_nodes) {
    if (context_nodes.empty()) {
        return 3;
    }
    
    int last_node = context_nodes.back();
    auto it = optimal_context_lengths_.find(last_node);
    return (it != optimal_context_lengths_.end()) ? it->second : 3;
}

std::vector<float> Predictor::get_context_embedding(
    const std::vector<int>& nodes,
    const std::unordered_map<int, std::vector<float>>& embeddings
) {
    std::vector<float> result(embedding_dim_, 0.0f);
    int count = 0;
    
    for (int node_id : nodes) {
        auto it = embeddings.find(node_id);
        if (it != embeddings.end()) {
            for (size_t i = 0; i < embedding_dim_ && i < it->second.size(); ++i) {
                result[i] += it->second[i];
            }
            count++;
        }
    }
    
    if (count > 0) {
        for (float& val : result) {
            val /= count;
        }
    }
    
    return result;
}

} // namespace reasoning
} // namespace melvin
