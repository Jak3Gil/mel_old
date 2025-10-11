#pragma once

#include "embedding_bridge.h"
#include "../../melvin_types.h"
#include <vector>
#include <string>

namespace melvin {
namespace embeddings {

// ==================== EMBEDDING LEARNING FEEDBACK ====================

class EmbeddingLearner {
private:
    EmbeddingBridge* bridge_;
    
    // Learning metrics
    struct LearningMetrics {
        float coherence_score;
        float entropy_improvement;
        float graph_alignment;
        int successful_predictions;
        int total_predictions;
        
        LearningMetrics() : coherence_score(0.0f), entropy_improvement(0.0f),
                           graph_alignment(0.0f), successful_predictions(0),
                           total_predictions(0) {}
    };
    
    LearningMetrics metrics_;
    
public:
    EmbeddingLearner(EmbeddingBridge* bridge) : bridge_(bridge) {}
    
    // Learn from successful generation
    void learn_from_success(
        const std::vector<uint64_t>& active_nodes,
        const std::vector<float>& activations,
        const std::vector<uint64_t>& generated_path,
        float coherence_score) {
        
        if (!bridge_ || generated_path.empty()) return;
        
        metrics_.total_predictions++;
        metrics_.successful_predictions++;
        metrics_.coherence_score = 0.9f * metrics_.coherence_score + 0.1f * coherence_score;
        
        // Positive reinforcement for each generated node/token
        for (uint64_t node_id : generated_path) {
            // Convert node to token
            extern std::unordered_map<uint64_t, Node> G_nodes;
            if (G_nodes.count(node_id) == 0) continue;
            
            std::string token = node_to_token(G_nodes[node_id].text);
            
            // Reward: align embeddings
            bridge_->learn_from_prediction(
                active_nodes,
                activations,
                token,
                coherence_score  // Use coherence as reward
            );
        }
    }
    
    // Learn from failure/misalignment
    void learn_from_failure(
        const std::vector<uint64_t>& active_nodes,
        const std::vector<float>& activations,
        const std::vector<uint64_t>& generated_path) {
        
        if (!bridge_ || generated_path.empty()) return;
        
        metrics_.total_predictions++;
        
        // Negative feedback
        for (uint64_t node_id : generated_path) {
            extern std::unordered_map<uint64_t, Node> G_nodes;
            if (G_nodes.count(node_id) == 0) continue;
            
            std::string token = node_to_token(G_nodes[node_id].text);
            
            bridge_->penalize_misalignment(active_nodes, activations, token);
        }
    }
    
    // Compute alignment between generated path and active nodes
    float compute_graph_alignment(
        const std::vector<uint64_t>& active_nodes,
        const std::vector<uint64_t>& generated_path) {
        
        if (active_nodes.empty() || generated_path.empty()) return 0.0f;
        
        // Count how many generated nodes are in active set
        int overlap = 0;
        for (uint64_t gen_node : generated_path) {
            if (std::find(active_nodes.begin(), active_nodes.end(), gen_node) 
                != active_nodes.end()) {
                overlap++;
            }
        }
        
        return (float)overlap / generated_path.size();
    }
    
    // Statistics
    const LearningMetrics& get_metrics() const {
        return metrics_;
    }
    
    void print_statistics() const {
        std::cout << "\n📚 Embedding Learning Statistics:\n";
        std::cout << "  Total predictions: " << metrics_.total_predictions << "\n";
        std::cout << "  Successful: " << metrics_.successful_predictions << "\n";
        if (metrics_.total_predictions > 0) {
            float success_rate = (float)metrics_.successful_predictions / metrics_.total_predictions;
            std::cout << "  Success rate: " << (success_rate * 100) << "%\n";
        }
        std::cout << "  Avg coherence: " << metrics_.coherence_score << "\n";
        std::cout << "  Graph alignment: " << metrics_.graph_alignment << "\n";
    }
};

// ==================== INTEGRATION WITH GENERATION LOOP ====================

// Helper to automatically learn from generation results
inline void auto_learn_from_generation(
    EmbeddingBridge& bridge,
    const std::vector<uint64_t>& context_nodes,
    const std::vector<uint64_t>& generated_path,
    float coherence_threshold = 0.6f) {
    
    // Compute activations (uniform for simplicity)
    std::vector<float> activations(context_nodes.size(), 1.0f / context_nodes.size());
    
    // Compute simple coherence (consecutive node similarity)
    float coherence = 0.0f;
    extern std::unordered_map<uint64_t, Node> G_nodes;
    
    for (size_t i = 0; i < generated_path.size() - 1; ++i) {
        if (G_nodes.count(generated_path[i]) > 0 && 
            G_nodes.count(generated_path[i+1]) > 0) {
            
            const auto& emb1 = bridge.node_manager().get_embedding_const(generated_path[i]);
            const auto& emb2 = bridge.node_manager().get_embedding_const(generated_path[i+1]);
            
            coherence += bridge.cosine_similarity(emb1, emb2);
        }
    }
    
    if (generated_path.size() > 1) {
        coherence /= (generated_path.size() - 1);
    }
    
    // Learn based on coherence
    EmbeddingLearner learner(&bridge);
    
    if (coherence >= coherence_threshold) {
        learner.learn_from_success(context_nodes, activations, generated_path, coherence);
    } else {
        learner.learn_from_failure(context_nodes, activations, generated_path);
    }
}

} // namespace embeddings
} // namespace melvin

