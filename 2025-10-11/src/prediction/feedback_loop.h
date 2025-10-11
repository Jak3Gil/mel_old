/*
 * MELVIN FEEDBACK LOOP
 * 
 * Incremental learning from prediction outcomes:
 * - Computes quality metrics for generated responses
 * - Updates predictor weights based on success/failure
 * - Strengthens graph edges that led to good predictions
 * - Provides feedback signals for both systems
 */

#pragma once
#include "markov_predictor.h"
#include "../../include/melvin_storage.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace melvin_prediction {

using namespace melvin_storage;

// ==================== FEEDBACK METRICS ====================

struct FeedbackMetrics {
    float coherence = 0.0f;         // Path coherence (no loops)
    float novelty = 0.0f;           // New patterns vs repetition
    float graph_alignment = 0.0f;   // Prediction matched graph bias
    float overall_quality = 0.0f;   // Combined score
    
    void print() const {
        std::cout << "📈 Feedback Metrics:\n";
        std::cout << "   Coherence:       " << std::fixed << std::setprecision(3) << coherence << "\n";
        std::cout << "   Novelty:         " << novelty << "\n";
        std::cout << "   Graph Alignment: " << graph_alignment << "\n";
        std::cout << "   Overall Quality: " << overall_quality << "\n";
    }
};

// ==================== QUALITY COMPUTATION ====================

// Compute response quality for feedback
inline FeedbackMetrics compute_response_quality(
    const std::vector<int>& output_tokens,
    const std::vector<int>& reasoning_path,
    const std::vector<float>& context_field
) {
    FeedbackMetrics metrics;
    
    // 1. Coherence: no token repetition, reasonable length
    std::set<int> unique_tokens(output_tokens.begin(), output_tokens.end());
    metrics.coherence = output_tokens.empty() ? 0.0f : 
                       (float)unique_tokens.size() / output_tokens.size();
    
    // Penalize if too short or too long
    if (output_tokens.size() < 3) metrics.coherence *= 0.5f;
    if (output_tokens.size() > 30) metrics.coherence *= 0.7f;
    
    // 2. Novelty: how many tokens weren't in reasoning path
    int novel_count = 0;
    std::set<int> path_set(reasoning_path.begin(), reasoning_path.end());
    for (int token : output_tokens) {
        if (path_set.find(token) == path_set.end()) {
            novel_count++;
        }
    }
    metrics.novelty = output_tokens.empty() ? 0.0f :
                     (float)novel_count / output_tokens.size();
    
    // 3. Graph alignment: how many output tokens were activated
    float alignment_sum = 0.0f;
    for (int token : output_tokens) {
        if (token >= 0 && token < (int)context_field.size()) {
            alignment_sum += context_field[token];
        }
    }
    metrics.graph_alignment = output_tokens.empty() ? 0.0f :
                             alignment_sum / output_tokens.size();
    
    // 4. Overall quality (weighted combination)
    metrics.overall_quality = 0.4f * metrics.coherence +
                             0.2f * metrics.novelty +
                             0.4f * metrics.graph_alignment;
    
    return metrics;
}

// ==================== PREDICTOR UPDATES ====================

// Update predictor incrementally based on feedback
inline void update_predictor_incremental(
    MarkovPredictor& predictor,
    const std::vector<int>& input_tokens,
    const std::vector<int>& output_tokens,
    const FeedbackMetrics& metrics
) {
    // Convert quality to reward signal
    float reward = metrics.overall_quality;
    
    // If quality is high, reinforce this pattern
    if (reward > 0.5f) {
        // Combine input and output for sequence learning
        std::vector<int> full_sequence = input_tokens;
        full_sequence.insert(full_sequence.end(), output_tokens.begin(), output_tokens.end());
        
        predictor.update_from_feedback(full_sequence, reward);
    }
    // If quality is low, penalize (negative reward)
    else if (reward < 0.3f) {
        predictor.update_from_feedback(output_tokens, -0.1f);
    }
}

// ==================== GRAPH UPDATES ====================

// Update graph based on successful predictions
inline void update_graph_from_prediction(
    const std::vector<int>& predicted_tokens,
    const std::vector<int>& reasoning_path,
    float confidence
) {
    if (confidence < 0.5f) return;  // Only update on confident predictions
    
    // Create weak connections between reasoning path and predicted tokens
    // This helps future queries benefit from successful predictions
    
    for (size_t i = 0; i + 1 < predicted_tokens.size(); ++i) {
        int from = predicted_tokens[i];
        int to = predicted_tokens[i+1];
        
        if (from < 0 || to < 0) continue;
        if (from >= (int)nodes.size() || to >= (int)nodes.size()) continue;
        
        // Check if edge already exists
        bool exists = false;
        auto it = adjacency.find(from);
        if (it != adjacency.end()) {
            for (int edge_id : it->second) {
                if (edges[edge_id].b == to) {
                    // Strengthen existing edge slightly
                    edges[edge_id].w = std::min(1.0f, edges[edge_id].w + 0.05f * confidence);
                    edges[edge_id].count++;
                    exists = true;
                    break;
                }
            }
        }
        
        // Create new weak edge if doesn't exist
        if (!exists && confidence > 0.7f) {
            Edge edge;
            edge.a = from;
            edge.b = to;
            edge.type = EdgeType::EXACT;
            edge.rel = 1;  // TEMPORAL
            edge.w = 0.1f * confidence;  // Start weak
            edge.w_core = 0.1f * confidence;
            edge.w_ctx = 0.0f;
            edge.count = 1;
            edge.ts_last = std::chrono::system_clock::now().time_since_epoch().count();
            
            int edge_id = (int)edges.size();
            edges.push_back(edge);
            adjacency[from].push_back(edge_id);
        }
    }
}

// ==================== FEEDBACK ORCHESTRATION ====================

// Complete feedback loop after generation
inline void apply_feedback(
    MarkovPredictor& predictor,
    const std::vector<int>& input_tokens,
    const std::vector<int>& output_tokens,
    const std::vector<int>& reasoning_path,
    const std::vector<float>& context_field,
    bool verbose = false
) {
    // Compute metrics
    auto metrics = compute_response_quality(output_tokens, reasoning_path, context_field);
    
    if (verbose) {
        metrics.print();
    }
    
    // Update predictor (fast learning)
    update_predictor_incremental(predictor, input_tokens, output_tokens, metrics);
    
    // Update graph (slow learning)
    update_graph_from_prediction(output_tokens, reasoning_path, metrics.overall_quality);
}

} // namespace melvin_prediction

