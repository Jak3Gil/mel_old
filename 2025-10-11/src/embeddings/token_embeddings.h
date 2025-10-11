#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <random>
#include <fstream>
#include <iostream>
#include <cmath>

namespace melvin {
namespace embeddings {

// ==================== TOKEN EMBEDDING MANAGER ====================

class TokenEmbeddingManager {
private:
    int embedding_dim_;
    std::unordered_map<std::string, std::vector<float>> token_embeddings_;
    std::mt19937 rng_;
    
    // Co-occurrence for initialization
    std::unordered_map<std::string, std::unordered_map<std::string, int>> cooccurrence_;
    
public:
    TokenEmbeddingManager(int dim = 64, uint64_t seed = 42)
        : embedding_dim_(dim), rng_(seed) {
    }
    
    // Get or create embedding for a token
    std::vector<float>& get_embedding(const std::string& token) {
        auto it = token_embeddings_.find(token);
        if (it != token_embeddings_.end()) {
            return it->second;
        }
        
        // Create new embedding
        std::vector<float> embedding(embedding_dim_);
        std::normal_distribution<float> dist(0.0f, 0.1f);
        
        for (int i = 0; i < embedding_dim_; ++i) {
            embedding[i] = dist(rng_);
        }
        
        normalize_embedding(embedding);
        
        token_embeddings_[token] = embedding;
        return token_embeddings_[token];
    }
    
    const std::vector<float>& get_embedding_const(const std::string& token) const {
        static std::vector<float> zero_embedding;
        if (zero_embedding.empty()) {
            zero_embedding.resize(embedding_dim_, 0.0f);
        }
        
        auto it = token_embeddings_.find(token);
        if (it != token_embeddings_.end()) {
            return it->second;
        }
        return zero_embedding;
    }
    
    // Initialize embeddings from co-occurrence data
    void initialize_from_cooccurrence(const std::string& text_corpus) {
        // Build co-occurrence matrix
        std::vector<std::string> tokens = tokenize(text_corpus);
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            for (int offset = -3; offset <= 3; ++offset) {
                if (offset == 0) continue;
                int j = i + offset;
                if (j >= 0 && j < (int)tokens.size()) {
                    cooccurrence_[tokens[i]][tokens[j]]++;
                }
            }
        }
        
        // Simple initialization: random with slight bias from co-occurrence
        for (const auto& [token, contexts] : cooccurrence_) {
            get_embedding(token);  // Creates if doesn't exist
        }
    }
    
    // Update embedding toward target
    void update_toward(const std::string& token,
                      const std::vector<float>& target_embedding,
                      float learning_rate,
                      float reward = 1.0f) {
        auto& emb = get_embedding(token);
        
        if (target_embedding.size() != (size_t)embedding_dim_) return;
        
        for (int i = 0; i < embedding_dim_; ++i) {
            float delta = target_embedding[i] - emb[i];
            emb[i] += learning_rate * reward * delta;
        }
        
        normalize_embedding(emb);
    }
    
    // Normalize embedding
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
    
    // Get all tokens
    std::vector<std::string> get_vocabulary() const {
        std::vector<std::string> vocab;
        for (const auto& [token, emb] : token_embeddings_) {
            vocab.push_back(token);
        }
        return vocab;
    }
    
    // Statistics
    size_t size() const { return token_embeddings_.size(); }
    int dimension() const { return embedding_dim_; }
    
    // Persistence
    void save(const std::string& filename = "data/token_embeddings.bin") {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return;
        
        uint32_t count = token_embeddings_.size();
        uint32_t dim = embedding_dim_;
        
        file.write((char*)&count, sizeof(count));
        file.write((char*)&dim, sizeof(dim));
        
        for (const auto& [token, embedding] : token_embeddings_) {
            uint32_t token_len = token.length();
            file.write((char*)&token_len, sizeof(token_len));
            file.write(token.c_str(), token_len);
            file.write((char*)embedding.data(), embedding.size() * sizeof(float));
        }
    }
    
    void load(const std::string& filename = "data/token_embeddings.bin") {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return;
        
        uint32_t count, dim;
        file.read((char*)&count, sizeof(count));
        file.read((char*)&dim, sizeof(dim));
        
        if (dim != (uint32_t)embedding_dim_) {
            std::cerr << "Warning: Token embedding dimension mismatch\n";
            return;
        }
        
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t token_len;
            file.read((char*)&token_len, sizeof(token_len));
            
            std::string token(token_len, '\0');
            file.read(&token[0], token_len);
            
            std::vector<float> embedding(dim);
            file.read((char*)embedding.data(), dim * sizeof(float));
            
            token_embeddings_[token] = embedding;
        }
    }
    
private:
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::string current;
        
        for (char c : text) {
            if (std::isalnum(c) || c == '\'' || c == '-') {
                current += std::tolower(c);
            } else if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        
        if (!current.empty()) {
            tokens.push_back(current);
        }
        
        return tokens;
    }
};

} // namespace embeddings
} // namespace melvin

