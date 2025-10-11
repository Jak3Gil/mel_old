#pragma once

#include "../../melvin_types.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <random>
#include <cmath>

namespace melvin {
namespace embeddings {

// ==================== NODE EMBEDDING MANAGER ====================

class NodeEmbeddingManager {
private:
    int embedding_dim_;
    std::unordered_map<uint64_t, std::vector<float>> node_embeddings_;
    std::mt19937 rng_;
    
    // Normalization tracking
    uint64_t updates_since_normalization_;
    uint64_t normalization_interval_;
    
public:
    NodeEmbeddingManager(int dim = 64, uint64_t seed = 42)
        : embedding_dim_(dim), rng_(seed),
          updates_since_normalization_(0),
          normalization_interval_(1000) {
    }
    
    // Get or create embedding for a node
    std::vector<float>& get_embedding(uint64_t node_id) {
        auto it = node_embeddings_.find(node_id);
        if (it != node_embeddings_.end()) {
            return it->second;
        }
        
        // Create new embedding with small random initialization
        std::vector<float> embedding(embedding_dim_);
        std::normal_distribution<float> dist(0.0f, 0.1f);
        
        for (int i = 0; i < embedding_dim_; ++i) {
            embedding[i] = dist(rng_);
        }
        
        // Normalize
        normalize_embedding(embedding);
        
        node_embeddings_[node_id] = embedding;
        return node_embeddings_[node_id];
    }
    
    const std::vector<float>& get_embedding_const(uint64_t node_id) const {
        static std::vector<float> zero_embedding;
        if (zero_embedding.empty()) {
            zero_embedding.resize(embedding_dim_, 0.0f);
        }
        
        auto it = node_embeddings_.find(node_id);
        if (it != node_embeddings_.end()) {
            return it->second;
        }
        return zero_embedding;
    }
    
    // Check if embedding exists
    bool has_embedding(uint64_t node_id) const {
        return node_embeddings_.find(node_id) != node_embeddings_.end();
    }
    
    // Initialize embedding from neighbor average
    void initialize_from_neighbors(uint64_t node_id,
                                   const std::vector<uint64_t>& neighbor_ids) {
        if (has_embedding(node_id)) return;
        
        std::vector<float> avg_embedding(embedding_dim_, 0.0f);
        int valid_neighbors = 0;
        
        for (uint64_t neighbor : neighbor_ids) {
            if (has_embedding(neighbor)) {
                const auto& neighbor_emb = get_embedding_const(neighbor);
                for (int i = 0; i < embedding_dim_; ++i) {
                    avg_embedding[i] += neighbor_emb[i];
                }
                valid_neighbors++;
            }
        }
        
        if (valid_neighbors > 0) {
            for (int i = 0; i < embedding_dim_; ++i) {
                avg_embedding[i] /= valid_neighbors;
            }
            normalize_embedding(avg_embedding);
            node_embeddings_[node_id] = avg_embedding;
        } else {
            // Fallback to random init
            get_embedding(node_id);
        }
    }
    
    // Update embedding toward target (learning)
    void update_toward(uint64_t node_id,
                      const std::vector<float>& target_embedding,
                      float learning_rate,
                      float reward = 1.0f) {
        auto& emb = get_embedding(node_id);
        
        if (target_embedding.size() != (size_t)embedding_dim_) return;
        
        // Move toward target proportional to reward
        for (int i = 0; i < embedding_dim_; ++i) {
            float delta = target_embedding[i] - emb[i];
            emb[i] += learning_rate * reward * delta;
        }
        
        normalize_embedding(emb);
        
        updates_since_normalization_++;
        if (updates_since_normalization_ >= normalization_interval_) {
            normalize_all();
            updates_since_normalization_ = 0;
        }
    }
    
    // Normalize a single embedding
    void normalize_embedding(std::vector<float>& embedding) const {
        float norm = 0.0f;
        for (float val : embedding) {
            norm += val * val;
        }
        norm = std::sqrt(norm);
        
        if (norm > 1e-6f) {
            for (float& val : embedding) {
                val /= norm;
            }
        }
    }
    
    // Normalize all embeddings
    void normalize_all() {
        for (auto& [node_id, embedding] : node_embeddings_) {
            normalize_embedding(embedding);
        }
    }
    
    // Statistics
    size_t size() const { return node_embeddings_.size(); }
    int dimension() const { return embedding_dim_; }
    
    // Persistence
    void save(const std::string& filename = "data/node_embeddings.bin") {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return;
        
        uint32_t count = node_embeddings_.size();
        uint32_t dim = embedding_dim_;
        
        file.write((char*)&count, sizeof(count));
        file.write((char*)&dim, sizeof(dim));
        
        for (const auto& [node_id, embedding] : node_embeddings_) {
            file.write((char*)&node_id, sizeof(node_id));
            file.write((char*)embedding.data(), embedding.size() * sizeof(float));
        }
    }
    
    void load(const std::string& filename = "data/node_embeddings.bin") {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return;
        
        uint32_t count, dim;
        file.read((char*)&count, sizeof(count));
        file.read((char*)&dim, sizeof(dim));
        
        if (dim != (uint32_t)embedding_dim_) {
            std::cerr << "Warning: Embedding dimension mismatch\n";
            return;
        }
        
        for (uint32_t i = 0; i < count; ++i) {
            uint64_t node_id;
            file.read((char*)&node_id, sizeof(node_id));
            
            std::vector<float> embedding(dim);
            file.read((char*)embedding.data(), dim * sizeof(float));
            
            node_embeddings_[node_id] = embedding;
        }
    }
};

} // namespace embeddings
} // namespace melvin

