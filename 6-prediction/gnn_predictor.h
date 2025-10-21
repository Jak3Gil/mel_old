#pragma once

#include "types.h"
#include "storage.h"
#include <vector>
#include <memory>

namespace melvin {
namespace gnn {

/**
 * 🧠 Graph Neural Network Predictor
 * 
 * Hybrid symbolic-neural reasoning:
 * - Each node has learnable embedding vector
 * - Message passing propagates information along edges
 * - Predicts next activation states
 * - Learns from prediction errors
 * - Creates LEAP connections from failed predictions
 * 
 * Result: Melvin predicts what will activate next!
 */
class GNNPredictor {
public:
    struct Config {
        int embed_dim = 32;              // Embedding vector size
        float learning_rate = 0.001f;    // Learning rate
        float prediction_decay = 0.9f;   // Momentum for embeddings
        float prediction_threshold = 0.3f; // Create LEAP if error > this
        bool full_connectivity = false;  // Dense attention mode
        int message_passes = 3;          // Message passing iterations
        bool verbose = false;            // Log training progress
        
        Config();
    };
    
    explicit GNNPredictor(const Config& config = Config());
    ~GNNPredictor();
    
    /**
     * Initialize embeddings for nodes (random or load from storage)
     */
    void initialize_embeddings(std::vector<Node>& nodes);
    
    /**
     * Message passing: propagate information along edges
     * Updates node embeddings based on neighbors
     */
    void message_pass(std::vector<Node>& nodes, const std::vector<Edge>& edges, Storage* storage);
    
    /**
     * Predict next activation state for each node
     * Returns: predicted activation values
     */
    std::vector<float> predict_activations(const std::vector<Node>& nodes);
    
    /**
     * Train: Update embeddings based on prediction error
     */
    void train(std::vector<Node>& nodes, const std::vector<float>& target_activations);
    
    /**
     * Create LEAP connections from high prediction errors
     * If predicted connection but doesn't exist → add LEAP edge
     */
    int create_leaps_from_errors(
        const std::vector<Node>& nodes,
        const std::vector<float>& predicted,
        const std::vector<float>& actual,
        Storage* storage
    );
    
    /**
     * Get statistics
     */
    struct Stats {
        float avg_prediction_error = 0.0f;
        float avg_embedding_norm = 0.0f;
        int leaps_created = 0;
        int training_steps = 0;
        float total_loss = 0.0f;
    };
    
    Stats get_stats() const { return stats_; }
    void reset_stats() { stats_ = Stats(); }
    
private:
    Config config_;
    Stats stats_;
    
    // Learned projection weights (embedding → activation prediction)
    std::vector<float> projection_weights_;
    
    // Helper: Sigmoid activation
    float sigmoid(float x) const {
        return 1.0f / (1.0f + std::exp(-x));
    }
    
    // Helper: Random uniform
    float random_uniform(float min, float max) const {
        return min + (max - min) * (rand() / (float)RAND_MAX);
    }
};

} // namespace gnn
} // namespace melvin

