#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "spreading_activation.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <map>

namespace melvin {
namespace reasoning {

struct PredictionResult {
    int node_id;
    float confidence;
    float score;
    std::string source;  // "exact_trigram", "exact_bigram", "exact_unigram", "semantic"
};

class Predictor {
public:
    explicit Predictor(int embedding_dim = 128);
    
    // Prediction modes
    enum class Mode {
        HYBRID,    // Try exact first, then semantic
        EXACT,     // Only exact recall
        SEMANTIC   // Only semantic reasoning
    };
    
    // Main prediction interface
    std::vector<PredictionResult> predict_next(
        const std::vector<int>& context_nodes,
        const ActivationField& activation_field,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        int top_k = 5,
        Mode mode = Mode::HYBRID
    );
    
    // Sequence recording (for learning)
    void record_sequence(const std::vector<int>& context, int next_node);
    
    // Context performance tracking
    void update_context_performance(const std::vector<int>& context, bool correct);
    
private:
    // Exact recall methods
    std::vector<PredictionResult> predict_exact_sequence(
        const std::vector<int>& context_nodes,
        int top_k
    );
    
    // Semantic prediction methods
    std::vector<PredictionResult> predict_semantic(
        const std::vector<int>& context_nodes,
        const ActivationField& activation_field,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        int top_k
    );
    
    // Helper methods
    int get_optimal_context_length(const std::vector<int>& context_nodes);
    std::vector<float> get_context_embedding(
        const std::vector<int>& nodes,
        const std::unordered_map<int, std::vector<float>>& embeddings
    );
    
    int embedding_dim_;
    
    // Exact sequence memory (n-grams)
    std::map<int, std::map<int, int>> transition_counts_;  // node → {next: count}
    std::map<std::pair<int,int>, std::map<int, int>> bigram_counts_;
    std::map<std::tuple<int,int,int>, std::map<int, int>> trigram_counts_;
    
    // Adaptive context tracking
    std::unordered_map<int, int> optimal_context_lengths_;
    std::unordered_map<int, std::vector<std::pair<int, bool>>> context_performance_;
};

} // namespace reasoning
} // namespace melvin

#endif // PREDICTOR_H
