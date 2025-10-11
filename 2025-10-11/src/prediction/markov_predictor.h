/*
 * MELVIN MARKOV PREDICTOR
 * 
 * Bigram/trigram Markov chain for fluent token prediction:
 * - Learns token sequences from training data
 * - Predicts next token probabilities
 * - Supports incremental learning from feedback
 * - Modular interface (can be replaced with neural predictor later)
 */

#pragma once
#include <map>
#include <vector>
#include <tuple>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

namespace melvin_prediction {

// ==================== MARKOV PREDICTOR ====================

class MarkovPredictor {
private:
    // Bigram counts: (token[i]) -> (token[i+1]) -> count
    std::map<int, std::map<int, float>> bigram_counts_;
    
    // Trigram counts: (token[i-1], token[i]) -> (token[i+1]) -> count
    std::map<std::pair<int,int>, std::map<int, float>> trigram_counts_;
    
    // Unigram counts for smoothing
    std::map<int, float> unigram_counts_;
    
    // Total tokens seen
    float total_tokens_ = 0.0f;
    
    // Smoothing parameter (configurable)
    float smoothing_ = 0.001f;
    
    // Vocabulary size estimate
    int vocab_size_ = 10000;
    
public:
    MarkovPredictor(float smoothing = 0.001f) : smoothing_(smoothing) {}
    
    // ==================== LEARNING ====================
    
    // Learn from token sequence
    void learn_sequence(const std::vector<int>& tokens, float weight = 1.0f) {
        if (tokens.size() < 2) return;
        
        // Update unigrams
        for (int token : tokens) {
            unigram_counts_[token] += weight;
            total_tokens_ += weight;
        }
        
        // Update bigrams
        for (size_t i = 0; i + 1 < tokens.size(); ++i) {
            bigram_counts_[tokens[i]][tokens[i+1]] += weight;
        }
        
        // Update trigrams
        for (size_t i = 0; i + 2 < tokens.size(); ++i) {
            auto key = std::make_pair(tokens[i], tokens[i+1]);
            trigram_counts_[key][tokens[i+2]] += weight;
        }
        
        // Update vocab size estimate
        vocab_size_ = std::max(vocab_size_, (int)unigram_counts_.size());
    }
    
    // Incremental update from feedback
    void update_from_feedback(const std::vector<int>& generated, float reward) {
        // Reward can be positive (good generation) or negative (bad)
        // Use it to adjust weights incrementally
        learn_sequence(generated, reward * 0.1f);  // Small learning rate
    }
    
    // ==================== PREDICTION ====================
    
    // Predict next token given context (returns top-k candidates with probabilities)
    std::vector<std::pair<int, float>> predict_next(
        const std::vector<int>& context,
        int top_k = 10
    ) const {
        std::map<int, float> probs;
        
        // Try trigram first (most context)
        if (context.size() >= 2) {
            auto key = std::make_pair(context[context.size()-2], context[context.size()-1]);
            auto it = trigram_counts_.find(key);
            
            if (it != trigram_counts_.end() && !it->second.empty()) {
                float total = 0.0f;
                for (const auto& [token, count] : it->second) {
                    total += count;
                }
                
                for (const auto& [token, count] : it->second) {
                    probs[token] = count / total;
                }
            }
        }
        
        // Fallback to bigram if trigram empty or low context
        if (probs.empty() && !context.empty()) {
            int last_token = context.back();
            auto it = bigram_counts_.find(last_token);
            
            if (it != bigram_counts_.end()) {
                float total = 0.0f;
                for (const auto& [token, count] : it->second) {
                    total += count;
                }
                
                for (const auto& [token, count] : it->second) {
                    probs[token] = (count + smoothing_) / (total + smoothing_ * vocab_size_);
                }
            }
        }
        
        // Fallback to unigram if no bigrams
        if (probs.empty()) {
            for (const auto& [token, count] : unigram_counts_) {
                probs[token] = (count + smoothing_) / (total_tokens_ + smoothing_ * vocab_size_);
            }
        }
        
        // Convert to sorted vector (top-k)
        std::vector<std::pair<int, float>> result;
        for (const auto& [token, prob] : probs) {
            result.push_back({token, prob});
        }
        
        std::sort(result.begin(), result.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (result.size() > (size_t)top_k) {
            result.resize(top_k);
        }
        
        // Renormalize
        float total = 0.0f;
        for (const auto& p : result) total += p.second;
        
        if (total > 0.0f) {
            for (auto& p : result) p.second /= total;
        }
        
        return result;
    }
    
    // ==================== STATISTICS ====================
    
    size_t bigram_count() const { return bigram_counts_.size(); }
    size_t trigram_count() const { return trigram_counts_.size(); }
    size_t vocab_size() const { return unigram_counts_.size(); }
    
    void print_stats() const {
        std::cout << "📊 Markov Predictor Stats:\n";
        std::cout << "   Vocabulary: " << unigram_counts_.size() << " tokens\n";
        std::cout << "   Bigrams:    " << bigram_counts_.size() << "\n";
        std::cout << "   Trigrams:   " << trigram_counts_.size() << "\n";
        std::cout << "   Total seen: " << (int)total_tokens_ << " tokens\n";
    }
    
    // ==================== PERSISTENCE ====================
    
    void save(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return;
        
        // Write header
        size_t uni_size = unigram_counts_.size();
        size_t bi_size = bigram_counts_.size();
        size_t tri_size = trigram_counts_.size();
        
        file.write(reinterpret_cast<const char*>(&uni_size), sizeof(uni_size));
        file.write(reinterpret_cast<const char*>(&bi_size), sizeof(bi_size));
        file.write(reinterpret_cast<const char*>(&tri_size), sizeof(tri_size));
        file.write(reinterpret_cast<const char*>(&total_tokens_), sizeof(total_tokens_));
        file.write(reinterpret_cast<const char*>(&vocab_size_), sizeof(vocab_size_));
        
        // Write unigrams
        for (const auto& [token, count] : unigram_counts_) {
            file.write(reinterpret_cast<const char*>(&token), sizeof(token));
            file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        }
        
        // Write bigrams
        for (const auto& [from, to_map] : bigram_counts_) {
            for (const auto& [to, count] : to_map) {
                file.write(reinterpret_cast<const char*>(&from), sizeof(from));
                file.write(reinterpret_cast<const char*>(&to), sizeof(to));
                file.write(reinterpret_cast<const char*>(&count), sizeof(count));
            }
        }
        
        // Write trigrams
        for (const auto& [context, to_map] : trigram_counts_) {
            for (const auto& [to, count] : to_map) {
                file.write(reinterpret_cast<const char*>(&context.first), sizeof(context.first));
                file.write(reinterpret_cast<const char*>(&context.second), sizeof(context.second));
                file.write(reinterpret_cast<const char*>(&to), sizeof(to));
                file.write(reinterpret_cast<const char*>(&count), sizeof(count));
            }
        }
    }
    
    bool load(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;
        
        // Read header
        size_t uni_size, bi_size, tri_size;
        file.read(reinterpret_cast<char*>(&uni_size), sizeof(uni_size));
        file.read(reinterpret_cast<char*>(&bi_size), sizeof(bi_size));
        file.read(reinterpret_cast<char*>(&tri_size), sizeof(tri_size));
        file.read(reinterpret_cast<char*>(&total_tokens_), sizeof(total_tokens_));
        file.read(reinterpret_cast<char*>(&vocab_size_), sizeof(vocab_size_));
        
        // Read unigrams
        for (size_t i = 0; i < uni_size; ++i) {
            int token;
            float count;
            file.read(reinterpret_cast<char*>(&token), sizeof(token));
            file.read(reinterpret_cast<char*>(&count), sizeof(count));
            unigram_counts_[token] = count;
        }
        
        // Read bigrams
        for (size_t i = 0; i < bi_size; ++i) {
            int from, to;
            float count;
            file.read(reinterpret_cast<char*>(&from), sizeof(from));
            file.read(reinterpret_cast<char*>(&to), sizeof(to));
            file.read(reinterpret_cast<char*>(&count), sizeof(count));
            bigram_counts_[from][to] = count;
        }
        
        // Read trigrams
        for (size_t i = 0; i < tri_size; ++i) {
            int ctx1, ctx2, to;
            float count;
            file.read(reinterpret_cast<char*>(&ctx1), sizeof(ctx1));
            file.read(reinterpret_cast<char*>(&ctx2), sizeof(ctx2));
            file.read(reinterpret_cast<char*>(&to), sizeof(to));
            file.read(reinterpret_cast<char*>(&count), sizeof(count));
            trigram_counts_[{ctx1, ctx2}][to] = count;
        }
        
        return true;
    }
};

} // namespace melvin_prediction

