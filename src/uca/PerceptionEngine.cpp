#include "src/uca/PerceptionEngine.hpp"
#include <cctype>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace uca {

// Mathematical constants for perception
static constexpr float TFIDF_BASE = 2.0f;           // F1: Base for TF-IDF calculation
static constexpr float BM25_K1 = 1.2f;              // F2: BM25 parameter k1
static constexpr float BM25_B = 0.75f;              // F2: BM25 parameter b

// Mock document frequency storage (in real implementation, this would be persistent)
static std::unordered_map<std::string, uint32_t> mock_document_frequencies;
static uint32_t total_documents = 0;

bool PerceptionEngine::configure(const DynamicGenome&) { 
    // Initialize mock document frequencies
    if (mock_document_frequencies.empty()) {
        // Common words have high document frequency
        mock_document_frequencies["the"] = 1000;
        mock_document_frequencies["is"] = 800;
        mock_document_frequencies["are"] = 700;
        mock_document_frequencies["and"] = 900;
        mock_document_frequencies["a"] = 850;
        mock_document_frequencies["an"] = 600;
        
        // Informative words have lower document frequency
        mock_document_frequencies["cats"] = 50;
        mock_document_frequencies["mammals"] = 30;
        mock_document_frequencies["animals"] = 40;
        mock_document_frequencies["what"] = 200;
        mock_document_frequencies["how"] = 150;
        mock_document_frequencies["why"] = 100;
        
        total_documents = 1000;  // Mock total document count
    }
    return true; 
}

// F1: Token rarity weight (TF-IDF-lite)
float compute_token_rarity_weight(const std::string& token) {
    auto it = mock_document_frequencies.find(token);
    uint32_t df = (it != mock_document_frequencies.end()) ? it->second : 1;
    
    // TF-IDF-lite: weight = 1 / (1 + log(1 + df))
    return 1.0f / (1.0f + std::log(1.0f + static_cast<float>(df)));
}

// F2: BM25-Mini for node retrieval (simplified)
float compute_bm25_score(const std::vector<std::string>& query_tokens, 
                        const std::vector<std::string>& document_tokens) {
    if (query_tokens.empty() || document_tokens.empty()) return 0.0f;
    
    float score = 0.0f;
    float avg_doc_length = static_cast<float>(document_tokens.size());
    
    for (const auto& query_token : query_tokens) {
        // Count term frequency in document
        uint32_t tf = 0;
        for (const auto& doc_token : document_tokens) {
            if (doc_token == query_token) tf++;
        }
        
        if (tf == 0) continue;
        
        // Get document frequency
        auto it = mock_document_frequencies.find(query_token);
        uint32_t df = (it != mock_document_frequencies.end()) ? it->second : 1;
        
        // IDF component
        float idf = std::log(static_cast<float>(total_documents) / static_cast<float>(df));
        
        // BM25 formula: IDF × (tf × (k1 + 1)) / (tf + k1 × (1 - b + b × (|d| / avgdl)))
        float doc_length_factor = 1.0f - BM25_B + BM25_B * (avg_doc_length / avg_doc_length);
        float bm25_component = (static_cast<float>(tf) * (BM25_K1 + 1.0f)) / 
                             (static_cast<float>(tf) + BM25_K1 * doc_length_factor);
        
        score += idf * bm25_component;
    }
    
    return score;
}

// Enhanced tokenization with rarity weighting
std::vector<std::pair<std::string, float>> tokenize_with_weights(const std::string& text) {
    std::vector<std::pair<std::string, float>> weighted_tokens;
    std::string current_token;
    
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!current_token.empty()) {
                float rarity_weight = compute_token_rarity_weight(current_token);
                weighted_tokens.push_back({current_token, rarity_weight});
                current_token.clear();
            }
        } else {
            current_token.push_back(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    
    // Add final token if exists
    if (!current_token.empty()) {
        float rarity_weight = compute_token_rarity_weight(current_token);
        weighted_tokens.push_back({current_token, rarity_weight});
    }
    
    return weighted_tokens;
}

Percept PerceptionEngine::perceive(const InputConcept& in) {
    Percept p; 
    p.modality = in.modality; 
    p.t_ns = in.t_ns;
    
    if (in.modality == "text") {
        // Enhanced tokenization with rarity weighting
        auto weighted_tokens = tokenize_with_weights(in.raw);
        
        // Extract tokens and store weights for reasoning
        for (const auto& [token, weight] : weighted_tokens) {
            p.tokens.push_back(token);
            // In a real implementation, weights would be stored in a separate field
            // For now, we'll use the token itself to encode weight information
            if (weight > 0.5f) {  // High rarity tokens
                p.tokens.push_back(token + "_rare");
            }
        }
        
        // Sort tokens by rarity weight (most informative first)
        std::sort(p.tokens.begin(), p.tokens.end(), 
                 [](const std::string& a, const std::string& b) {
                     float weight_a = compute_token_rarity_weight(a);
                     float weight_b = compute_token_rarity_weight(b);
                     return weight_a > weight_b;  // Higher weight first
                 });
    }
    
    return p;
}

}
