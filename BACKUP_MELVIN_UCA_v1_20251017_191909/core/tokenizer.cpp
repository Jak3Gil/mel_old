#include "tokenizer.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <cctype>
#include <map>

namespace melvin {
namespace tokenizer {

// ============================================================================
// Helper Functions
// ============================================================================

static std::vector<std::string> split_whitespace(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

static std::string to_lowercase(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

// ============================================================================
// Config Implementation
// ============================================================================

Tokenizer::Config::Config() {}

// ============================================================================
// Implementation Class
// ============================================================================

class Tokenizer::Impl {
public:
    Config config_;
    
    // Vocabulary: token_id -> token_string
    std::vector<std::string> id_to_token_;
    
    // Reverse mapping: token_string -> token_id
    std::unordered_map<std::string, uint32_t> token_to_id_;
    
    // Token ↔ Node bidirectional mapping
    std::unordered_map<uint32_t, NodeID> token_to_node_;
    std::unordered_map<NodeID, uint32_t> node_to_token_;
    
    // BPE merge rules: (token1, token2) -> merged_token_id
    std::map<std::pair<std::string, std::string>, uint32_t> bpe_merges_;
    
    // Special token IDs
    uint32_t unk_id_ = 0;
    uint32_t bos_id_ = 1;
    uint32_t eos_id_ = 2;
    uint32_t pad_id_ = 3;
    
    // Statistics
    Stats stats_;
    
    Impl(const Config& config) : config_(config) {
        // Initialize special tokens
        add_token(config_.unk_token);
        add_token(config_.bos_token);
        add_token(config_.eos_token);
        add_token(config_.pad_token);
        
        unk_id_ = 0;
        bos_id_ = 1;
        eos_id_ = 2;
        pad_id_ = 3;
        
        // Initialize with basic ASCII characters
        for (int i = 32; i < 127; i++) {
            std::string ch(1, static_cast<char>(i));
            add_token(ch);
        }
    }
    
    uint32_t add_token(const std::string& token) {
        auto it = token_to_id_.find(token);
        if (it != token_to_id_.end()) {
            return it->second;
        }
        
        uint32_t id = id_to_token_.size();
        id_to_token_.push_back(token);
        token_to_id_[token] = id;
        stats_.total_tokens++;
        
        return id;
    }
    
    std::string get_token(uint32_t token_id) const {
        if (token_id < id_to_token_.size()) {
            return id_to_token_[token_id];
        }
        return config_.unk_token;
    }
    
    uint32_t get_token_id(const std::string& token) const {
        auto it = token_to_id_.find(token);
        if (it != token_to_id_.end()) {
            return it->second;
        }
        return unk_id_;
    }
    
    std::vector<std::string> tokenize(const std::string& text) const {
        std::string processed = text;
        if (config_.lowercase) {
            processed = to_lowercase(processed);
        }
        
        // Split on whitespace first
        auto words = split_whitespace(processed);
        std::vector<std::string> tokens;
        
        for (const auto& word : words) {
            // Split into characters
            std::vector<std::string> chars;
            for (char c : word) {
                chars.push_back(std::string(1, c));
            }
            
            // Apply BPE merges
            while (chars.size() > 1) {
                // Find best merge
                int best_i = -1;
                uint32_t best_merge_id = UINT32_MAX;
                
                for (size_t i = 0; i < chars.size() - 1; i++) {
                    auto pair = std::make_pair(chars[i], chars[i+1]);
                    auto it = bpe_merges_.find(pair);
                    if (it != bpe_merges_.end()) {
                        if (it->second < best_merge_id) {
                            best_merge_id = it->second;
                            best_i = i;
                        }
                    }
                }
                
                if (best_i == -1) break;  // No more merges
                
                // Perform merge
                std::string merged = chars[best_i] + chars[best_i+1];
                chars[best_i] = merged;
                chars.erase(chars.begin() + best_i + 1);
            }
            
            // Add tokens from this word
            for (const auto& token : chars) {
                tokens.push_back(token);
            }
        }
        
        return tokens;
    }
    
    std::vector<uint32_t> encode(const std::string& text) const {
        auto tokens = tokenize(text);
        std::vector<uint32_t> ids;
        ids.reserve(tokens.size());
        
        for (const auto& token : tokens) {
            ids.push_back(get_token_id(token));
        }
        
        return ids;
    }
    
    std::vector<uint32_t> encode_with_special(const std::string& text) const {
        auto ids = encode(text);
        ids.insert(ids.begin(), bos_id_);
        ids.push_back(eos_id_);
        return ids;
    }
    
    std::string decode(const std::vector<uint32_t>& tokens) const {
        std::string result;
        for (uint32_t token_id : tokens) {
            // Skip special tokens
            if (token_id == bos_id_ || token_id == eos_id_ || token_id == pad_id_) {
                continue;
            }
            result += get_token(token_id);
        }
        return result;
    }
    
    void train_from_corpus(const std::vector<std::string>& corpus) {
        if (config_.verbose) {
            std::cout << "[Tokenizer] Training BPE on " << corpus.size() << " documents..." << std::endl;
        }
        
        // Build vocabulary from corpus
        std::unordered_map<std::string, int> word_freq;
        
        for (const auto& doc : corpus) {
            std::string processed = doc;
            if (config_.lowercase) {
                processed = to_lowercase(processed);
            }
            
            auto words = split_whitespace(processed);
            for (const auto& word : words) {
                word_freq[word]++;
            }
        }
        
        // Learn BPE merges
        size_t target_merges = config_.vocab_size - id_to_token_.size();
        
        for (size_t merge_idx = 0; merge_idx < target_merges && merge_idx < 10000; merge_idx++) {
            // Count bigram frequencies
            std::map<std::pair<std::string, std::string>, int> bigram_freq;
            
            for (const auto& [word, freq] : word_freq) {
                // Split word into current tokens
                std::vector<std::string> chars;
                for (char c : word) {
                    chars.push_back(std::string(1, c));
                }
                
                // Apply existing merges
                for (const auto& [pair, merge_id] : bpe_merges_) {
                    for (size_t i = 0; i < chars.size() - 1; i++) {
                        if (chars[i] == pair.first && chars[i+1] == pair.second) {
                            chars[i] = pair.first + pair.second;
                            chars.erase(chars.begin() + i + 1);
                        }
                    }
                }
                
                // Count bigrams
                for (size_t i = 0; i < chars.size() - 1; i++) {
                    bigram_freq[{chars[i], chars[i+1]}] += freq;
                }
            }
            
            if (bigram_freq.empty()) break;
            
            // Find most frequent bigram
            auto best = std::max_element(bigram_freq.begin(), bigram_freq.end(),
                                        [](const auto& a, const auto& b) {
                                            return a.second < b.second;
                                        });
            
            // Add merge
            std::string merged = best->first.first + best->first.second;
            uint32_t merge_id = add_token(merged);
            bpe_merges_[best->first] = merge_id;
            
            if (config_.verbose && merge_idx % 1000 == 0) {
                std::cout << "[Tokenizer] Learned " << merge_idx << " merges..." << std::endl;
            }
        }
        
        if (config_.verbose) {
            std::cout << "[Tokenizer] Training complete. Vocabulary size: " 
                      << id_to_token_.size() << std::endl;
        }
    }
    
    NodeID token_to_node(uint32_t token_id) {
        auto it = token_to_node_.find(token_id);
        if (it != token_to_node_.end()) {
            return it->second;
        }
        
        // Create new mapping (node ID = token ID for simplicity)
        NodeID node_id = static_cast<NodeID>(token_id);
        token_to_node_[token_id] = node_id;
        node_to_token_[node_id] = token_id;
        stats_.mapped_tokens++;
        
        return node_id;
    }
    
    uint32_t node_to_token(NodeID node_id) const {
        auto it = node_to_token_.find(node_id);
        if (it != node_to_token_.end()) {
            return it->second;
        }
        return unk_id_;
    }
    
    bool has_token_mapping(uint32_t token_id) const {
        return token_to_node_.count(token_id) > 0;
    }
    
    bool has_node_mapping(NodeID node_id) const {
        return node_to_token_.count(node_id) > 0;
    }
    
    bool save(const std::string& path) const {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        
        // Write header
        uint32_t version = 1;
        uint32_t vocab_size = id_to_token_.size();
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&vocab_size), sizeof(vocab_size));
        
        // Write vocabulary
        for (const auto& token : id_to_token_) {
            uint32_t len = token.size();
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(token.data(), len);
        }
        
        // Write BPE merges
        uint32_t merge_count = bpe_merges_.size();
        file.write(reinterpret_cast<const char*>(&merge_count), sizeof(merge_count));
        for (const auto& [pair, merge_id] : bpe_merges_) {
            uint32_t len1 = pair.first.size();
            uint32_t len2 = pair.second.size();
            file.write(reinterpret_cast<const char*>(&len1), sizeof(len1));
            file.write(pair.first.data(), len1);
            file.write(reinterpret_cast<const char*>(&len2), sizeof(len2));
            file.write(pair.second.data(), len2);
            file.write(reinterpret_cast<const char*>(&merge_id), sizeof(merge_id));
        }
        
        // Write token-node mappings
        uint32_t mapping_count = token_to_node_.size();
        file.write(reinterpret_cast<const char*>(&mapping_count), sizeof(mapping_count));
        for (const auto& [token_id, node_id] : token_to_node_) {
            file.write(reinterpret_cast<const char*>(&token_id), sizeof(token_id));
            file.write(reinterpret_cast<const char*>(&node_id), sizeof(node_id));
        }
        
        return true;
    }
    
    bool load(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        
        // Read header
        uint32_t version, vocab_size;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        file.read(reinterpret_cast<char*>(&vocab_size), sizeof(vocab_size));
        
        if (version != 1) return false;
        
        // Read vocabulary
        id_to_token_.clear();
        token_to_id_.clear();
        id_to_token_.reserve(vocab_size);
        
        for (uint32_t i = 0; i < vocab_size; i++) {
            uint32_t len;
            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string token(len, '\0');
            file.read(&token[0], len);
            
            id_to_token_.push_back(token);
            token_to_id_[token] = i;
        }
        
        // Read BPE merges
        uint32_t merge_count;
        file.read(reinterpret_cast<char*>(&merge_count), sizeof(merge_count));
        bpe_merges_.clear();
        
        for (uint32_t i = 0; i < merge_count; i++) {
            uint32_t len1, len2;
            file.read(reinterpret_cast<char*>(&len1), sizeof(len1));
            std::string first(len1, '\0');
            file.read(&first[0], len1);
            
            file.read(reinterpret_cast<char*>(&len2), sizeof(len2));
            std::string second(len2, '\0');
            file.read(&second[0], len2);
            
            uint32_t merge_id;
            file.read(reinterpret_cast<char*>(&merge_id), sizeof(merge_id));
            
            bpe_merges_[{first, second}] = merge_id;
        }
        
        // Read token-node mappings
        uint32_t mapping_count;
        file.read(reinterpret_cast<char*>(&mapping_count), sizeof(mapping_count));
        token_to_node_.clear();
        node_to_token_.clear();
        
        for (uint32_t i = 0; i < mapping_count; i++) {
            uint32_t token_id;
            NodeID node_id;
            file.read(reinterpret_cast<char*>(&token_id), sizeof(token_id));
            file.read(reinterpret_cast<char*>(&node_id), sizeof(node_id));
            
            token_to_node_[token_id] = node_id;
            node_to_token_[node_id] = token_id;
        }
        
        stats_.total_tokens = vocab_size;
        stats_.mapped_tokens = mapping_count;
        
        return true;
    }
    
    Stats get_stats() const {
        Stats s = stats_;
        
        // Compute compression ratio
        size_t total_chars = 0;
        size_t total_tokens = 0;
        for (const auto& token : id_to_token_) {
            total_chars += token.size();
            total_tokens++;
        }
        s.compression_ratio = total_tokens > 0 ? (float)total_chars / total_tokens : 0.0f;
        
        return s;
    }
};

// ============================================================================
// Public Interface
// ============================================================================

Tokenizer::Tokenizer(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

Tokenizer::~Tokenizer() = default;

std::vector<uint32_t> Tokenizer::encode(const std::string& text) const {
    return impl_->encode(text);
}

std::string Tokenizer::decode(const std::vector<uint32_t>& tokens) const {
    return impl_->decode(tokens);
}

std::vector<uint32_t> Tokenizer::encode_with_special(const std::string& text) const {
    return impl_->encode_with_special(text);
}

std::vector<std::string> Tokenizer::tokenize(const std::string& text) const {
    return impl_->tokenize(text);
}

NodeID Tokenizer::token_to_node(uint32_t token_id) {
    return impl_->token_to_node(token_id);
}

uint32_t Tokenizer::node_to_token(NodeID node_id) const {
    return impl_->node_to_token(node_id);
}

bool Tokenizer::has_token_mapping(uint32_t token_id) const {
    return impl_->has_token_mapping(token_id);
}

bool Tokenizer::has_node_mapping(NodeID node_id) const {
    return impl_->has_node_mapping(node_id);
}

void Tokenizer::train_from_corpus(const std::vector<std::string>& corpus) {
    impl_->train_from_corpus(corpus);
}

uint32_t Tokenizer::add_token(const std::string& token) {
    return impl_->add_token(token);
}

std::string Tokenizer::get_token(uint32_t token_id) const {
    return impl_->get_token(token_id);
}

uint32_t Tokenizer::get_token_id(const std::string& token) const {
    return impl_->get_token_id(token);
}

size_t Tokenizer::vocab_size() const {
    return impl_->id_to_token_.size();
}

uint32_t Tokenizer::unk_token_id() const { return impl_->unk_id_; }
uint32_t Tokenizer::bos_token_id() const { return impl_->bos_id_; }
uint32_t Tokenizer::eos_token_id() const { return impl_->eos_id_; }
uint32_t Tokenizer::pad_token_id() const { return impl_->pad_id_; }

bool Tokenizer::save(const std::string& path) const {
    return impl_->save(path);
}

bool Tokenizer::load(const std::string& path) {
    return impl_->load(path);
}

Tokenizer::Stats Tokenizer::get_stats() const {
    return impl_->get_stats();
}

void Tokenizer::print_stats() const {
    auto stats = get_stats();
    std::cout << "\n=== Tokenizer Statistics ===\n";
    std::cout << "Total tokens: " << stats.total_tokens << "\n";
    std::cout << "Mapped tokens: " << stats.mapped_tokens << "\n";
    std::cout << "Unknown count: " << stats.unknown_count << "\n";
    std::cout << "Compression ratio: " << stats.compression_ratio << " chars/token\n";
    std::cout << "============================\n\n";
}

} // namespace tokenizer
} // namespace melvin

