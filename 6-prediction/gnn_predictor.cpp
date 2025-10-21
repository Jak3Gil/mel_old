/*
 * Graph Neural Network Predictor Implementation
 * 
 * Hybrid symbolic-neural reasoning:
 * - Learnable node embeddings
 * - Message passing along edges
 * - Activation prediction
 * - Learning from errors
 */

#include "gnn_predictor.h"
#include "../UCAConfig.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace melvin {
namespace gnn {

GNNPredictor::Config::Config() {}

GNNPredictor::GNNPredictor(const Config& config)
    : config_(config) {
    
    // Initialize projection weights randomly
    projection_weights_.resize(config_.embed_dim);
    for (auto& w : projection_weights_) {
        w = random_uniform(-0.1f, 0.1f);
    }
}

GNNPredictor::~GNNPredictor() = default;

void GNNPredictor::initialize_embeddings(std::vector<Node>& nodes) {
    for (auto& node : nodes) {
        // Initialize embedding if not already set
        if (node.embedding.empty()) {
            node.embedding.resize(config_.embed_dim);
            for (auto& val : node.embedding) {
                val = random_uniform(-0.1f, 0.1f);
            }
        } else if ((int)node.embedding.size() != config_.embed_dim) {
            // Resize if dimension changed
            node.embedding.resize(config_.embed_dim, 0.0f);
        }
    }
}

void GNNPredictor::message_pass(
    std::vector<Node>& nodes,
    const std::vector<Edge>& edges,
    Storage* storage
) {
    // Initialize embeddings if needed
    initialize_embeddings(nodes);
    
    // Create node ID to index mapping
    std::unordered_map<NodeID, size_t> id_to_idx;
    for (size_t i = 0; i < nodes.size(); ++i) {
        id_to_idx[nodes[i].id] = i;
    }
    
    // Perform multiple message passing iterations
    for (int pass = 0; pass < config_.message_passes; ++pass) {
        // Allocate new embeddings
        std::vector<std::vector<float>> new_embeddings(
            nodes.size(),
            std::vector<float>(config_.embed_dim, 0.0f)
        );
        
        if (config_.full_connectivity) {
            // DENSE MODE: Every node to every node (transformer-like)
            for (size_t i = 0; i < nodes.size(); ++i) {
                for (size_t j = 0; j < nodes.size(); ++j) {
                    if (i == j) continue;
                    
                    float message_weight = nodes[i].activation * 0.01f;
                    
                    for (int d = 0; d < config_.embed_dim; ++d) {
                        new_embeddings[j][d] += message_weight * nodes[i].embedding[d];
                    }
                }
            }
        } else {
            // SPARSE MODE: Message passing along existing edges only
            for (const auto& edge : edges) {
                auto from_it = id_to_idx.find(edge.from_id);
                auto to_it = id_to_idx.find(edge.to_id);
                
                if (from_it == id_to_idx.end() || to_it == id_to_idx.end()) {
                    continue;
                }
                
                size_t from_idx = from_it->second;
                size_t to_idx = to_it->second;
                
                // Message weight combines edge weight and adaptive weight
                float message_weight = edge.get_effective_weight();
                
                // Aggregate messages
                for (int d = 0; d < config_.embed_dim; ++d) {
                    new_embeddings[to_idx][d] += message_weight * nodes[from_idx].embedding[d];
                }
            }
        }
        
        // Apply nonlinearity and normalize
        for (size_t i = 0; i < nodes.size(); ++i) {
            float norm = 0.0f;
            
            // Apply tanh activation
            for (int d = 0; d < config_.embed_dim; ++d) {
                new_embeddings[i][d] = std::tanh(new_embeddings[i][d]);
                norm += new_embeddings[i][d] * new_embeddings[i][d];
            }
            
            // Normalize
            norm = std::sqrt(norm) + 1e-6f;
            
            // Update with momentum
            for (int d = 0; d < config_.embed_dim; ++d) {
                float normalized = new_embeddings[i][d] / norm;
                nodes[i].embedding[d] = config_.prediction_decay * nodes[i].embedding[d] +
                                       (1.0f - config_.prediction_decay) * normalized;
            }
        }
    }
    
    (void)storage;  // Reserved for future use
}

std::vector<float> GNNPredictor::predict_activations(const std::vector<Node>& nodes) {
    std::vector<float> predictions(nodes.size(), 0.0f);
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].embedding.empty()) {
            predictions[i] = 0.0f;
            continue;
        }
        
        // Linear projection: embedding → activation
        float sum = 0.0f;
        for (int d = 0; d < config_.embed_dim && d < (int)nodes[i].embedding.size(); ++d) {
            sum += projection_weights_[d] * nodes[i].embedding[d];
        }
        
        // Sigmoid activation
        predictions[i] = sigmoid(sum);
    }
    
    return predictions;
}

void GNNPredictor::train(
    std::vector<Node>& nodes,
    const std::vector<float>& target_activations
) {
    if (target_activations.size() != nodes.size()) {
        return;
    }
    
    // Get current predictions
    auto predictions = predict_activations(nodes);
    
    // Compute loss and gradients
    float total_loss = 0.0f;
    std::vector<float> projection_grads(config_.embed_dim, 0.0f);
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        float error = predictions[i] - target_activations[i];
        total_loss += error * error;
        
        // Gradient of sigmoid: pred * (1 - pred) * error
        float grad_output = predictions[i] * (1.0f - predictions[i]) * error;
        
        // Backprop to projection weights
        for (int d = 0; d < config_.embed_dim && d < (int)nodes[i].embedding.size(); ++d) {
            projection_grads[d] += grad_output * nodes[i].embedding[d];
        }
        
        // Backprop to embeddings
        for (int d = 0; d < config_.embed_dim && d < (int)nodes[i].embedding.size(); ++d) {
            float embedding_grad = grad_output * projection_weights_[d];
            nodes[i].embedding[d] -= config_.learning_rate * embedding_grad;
        }
    }
    
    // Update projection weights
    for (int d = 0; d < config_.embed_dim; ++d) {
        projection_weights_[d] -= config_.learning_rate * projection_grads[d];
    }
    
    // Update stats
    stats_.total_loss = total_loss;
    stats_.avg_prediction_error = std::sqrt(total_loss / nodes.size());
    stats_.training_steps++;
    
    // Compute embedding norm
    float total_norm = 0.0f;
    for (const auto& node : nodes) {
        if (!node.embedding.empty()) {
            float norm = 0.0f;
            for (float val : node.embedding) {
                norm += val * val;
            }
            total_norm += std::sqrt(norm);
        }
    }
    stats_.avg_embedding_norm = total_norm / nodes.size();
    
    if (config_.verbose && stats_.training_steps % 10 == 0) {
        std::cout << "[GNN] Step " << stats_.training_steps 
                  << " | Loss: " << stats_.total_loss
                  << " | Avg Error: " << stats_.avg_prediction_error
                  << " | Embedding Norm: " << stats_.avg_embedding_norm << "\n";
    }
}

int GNNPredictor::create_leaps_from_errors(
    const std::vector<Node>& nodes,
    const std::vector<float>& predicted,
    const std::vector<float>& actual,
    Storage* storage
) {
    int leaps_created = 0;
    
    // Find pairs with high prediction error
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = 0; j < nodes.size(); ++j) {
            if (i == j) continue;
            
            // If both have high activation but connection missing
            float error_i = std::abs(predicted[i] - actual[i]);
            float error_j = std::abs(predicted[j] - actual[j]);
            
            if (error_i > config_.prediction_threshold &&
                error_j > config_.prediction_threshold &&
                actual[i] > 0.5f && actual[j] > 0.5f) {
                
                // Check if connection already exists
                auto existing = storage->get_edges(nodes[i].id, nodes[j].id);
                
                if (existing.empty()) {
                    // Create LEAP connection (hypothesis from prediction failure)
                    storage->create_edge(nodes[i].id, nodes[j].id, RelationType::LEAP, 0.7f);
                    leaps_created++;
                    
                    if (config_.verbose) {
                        std::cout << "[GNN] Created LEAP: " 
                                  << storage->get_node_content(nodes[i].id) << " → "
                                  << storage->get_node_content(nodes[j].id)
                                  << " (prediction error: " << error_i << ")\n";
                    }
                }
            }
        }
    }
    
    stats_.leaps_created += leaps_created;
    return leaps_created;
}

} // namespace gnn
} // namespace melvin

