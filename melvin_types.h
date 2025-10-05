#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <cmath>
#include <sstream>
#include <iostream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace melvin {

// Binary ID types - all content-addressed, 32 bytes each
using NodeID = std::array<uint8_t, 32>;
using EdgeID = std::array<uint8_t, 32>;
using PathID = std::array<uint8_t, 32>;

// Binary 2-bit relation codes for Neural-Graph Hybrid System
// These encode fundamental relationship types between concepts
enum class Rel : uint32_t {
    EXACT = 0,          // 00 - Direct semantic match, definition, identity
    TEMPORAL = 1,       // 01 - Sequential, causal, temporal ordering
    LEAP = 2,           // 10 - Inference, hypothesis, creative connection
    GENERALIZATION = 3, // 11 - Abstraction, pattern, category membership
    MAX_REL = 3
};

// Node types for Neural-Graph Hybrid System
enum class NodeType : uint32_t {
    SYMBOL = 0,           // Raw token/word
    CONCEPT = 1,          // Single semantic unit
    PATH = 2,             // Reasoning chain
    META_COG_STEP = 3,    // Cognitive operation trace
    ABSTRACTION = 4,      // Pooled/hierarchical concept
    PHRASE = 5,           // Token pair/triple (first pooling level)
    MODALITY_AUDIO = 6,   // Audio frame/segment
    MODALITY_IMAGE = 7,   // Image embedding
    THOUGHT_TRACE = 8,    // Replayable reasoning chain
    MAX_TYPE = 255
};

// Node flags
enum NodeFlags : uint32_t {
    HYPOTHESIS = 1 << 0,
    DURABLE = 1 << 1,
    ORACLE_USED = 1 << 2,
    DEPRECATED = 1 << 3,
    LOCKED = 1 << 4,
    GENERALIZED = 1 << 5,
    ANCHOR = 1 << 6        // Anchor flag for stability
};

// Edge flags
enum EdgeFlags : uint32_t {
    EDGE_HYPOTHESIS = 1 << 0,
    EDGE_DEPRECATED = 1 << 1,
    EDGE_LOCKED = 1 << 2,
    EDGE_GENERALIZED = 1 << 3,
    EDGE_INFERRED = 1 << 4,
    EDGE_ANCHOR = 1 << 5    // Anchor flag for stability
};

// View kinds for output generation
enum class ViewKind : uint8_t {
    TEXT_VIEW = 0,
    AUDIO_VIEW = 1,
    CONTROL_VIEW = 2,
    DEBUG_VIEW = 3
};

// VM opcodes
enum class Opcode : uint8_t {
    LINK = 0,
    WALK = 1,
    COMPOSE = 2,
    REINFORCE = 3,
    DECAY = 4,
    MERGE = 5,
    CONTRADICT = 6,
    EMIT = 7,
    TRACE = 8,
    HALT = 255
};

// Instinct driver vector (5 dimensions)
struct Drivers {
    float curiosity = 0.5f;
    float efficiency = 0.5f;
    float consistency = 0.5f;
    float social = 0.5f;
    float survival = 0.5f;
    
    Drivers() = default;
    Drivers(float c, float e, float co, float s, float su) 
        : curiosity(c), efficiency(e), consistency(co), social(s), survival(su) {}
};

// Output Evolution Framework Variables
struct OutputEvolutionConfig {
    // Content Selection - EVOLVABLE PARAMETERS
    float match_strength_thresh = 0.1f;    // minimum edge weight before using a path in output (evolvable)
    float context_bias = 0.7f;             // how much recent context influences choice (0–1) (evolvable)
    float leap_chance = 0.15f;             // probability of including leap connections in answers (evolvable)
    int max_depth = 6;                     // how far to traverse before forming an output (evolvable)
    
    // Answer Quality Thresholds - EVOLVABLE PARAMETERS
    float min_confidence_threshold = 0.1f; // minimum confidence to accept an answer (evolvable)
    float min_path_score = 0.05f;          // minimum path score to consider (evolvable)
    float min_semantic_coherence = 0.1f;   // minimum semantic coherence (evolvable)
    float min_relation_strength = 0.1f;    // minimum relation strength (evolvable)
    
    // Formulation - EVOLVABLE PARAMETERS
    float compression_level = 0.5f;        // how much to condense multiple nodes into one phrase (evolvable)
    float variation_rate = 0.3f;           // how often Melvin tries new wordings vs repeating stored chains (evolvable)
    float confidence_scaling = 0.8f;       // maps weight strength to output confidence (evolvable)
    float anchor_lock = 0.9f;              // how strongly Melvin sticks to anchor nodes (core personality) (evolvable)
    
    // Modality
    enum class OutputMode : uint8_t {
        TEXT = 0,
        SPEECH = 1,
        ACTION = 2,
        MULTI_MODAL = 3
    } output_mode = OutputMode::TEXT;
    
    float audio_variance = 0.2f;           // adds natural variation in sound output (pitch, speed)
    enum class ImageSemanticMode : uint8_t {
        SIMILARITY_CLUSTER = 0,
        LABEL = 1,
        ABSTRACTION = 2
    } image_semantic_mode = ImageSemanticMode::LABEL;
    float multi_modal_blend = 0.1f;        // chance to mix text + sound + image references in one output
    
    // Reinforcement
    bool reinforce_on_output = true;       // whether outputs increase connection weights
    float novelty_bonus = 0.2f;            // extra reinforcement when Melvin tries a new phrasing
    float stability_bias = 0.6f;           // preference for reusing older thought nodes vs generating new ones
    
    // Evolutionary Hooks (self-adjusting)
    float leap_chance_growth_rate = 0.01f; // how fast leap_chance grows when stuck in loops
    float variation_rate_growth_rate = 0.02f; // how fast variation_rate increases when feedback = "too repetitive"
    float confidence_scaling_adjustment = 0.05f; // how much confidence_scaling shifts if outputs are often wrong
    float anchor_lock_stability = 0.95f;   // anchor_lock stays constant (prevents personality drift)
    
    // Feedback tracking for evolution
    uint32_t repetition_feedback_count = 0;
    uint32_t wrong_output_count = 0;
    uint32_t total_outputs = 0;
    uint64_t last_evolution_update = 0;
    
    OutputEvolutionConfig() = default;
    
    // Evolution methods
    void evolve_based_on_feedback();
    void reset_feedback_counters();
    bool should_evolve() const;
};

// COG_STEP operation types
enum class CogOp : uint8_t {
    INGEST = 0,
    RECALL = 1,
    EXPLORE = 2,
    INTEGRATE = 3,
    DECIDE = 4,
    OUTPUT = 5,
    REINFORCE = 6,
    PRUNE = 7,
    PATCH = 8
};

// Error flags for COG_STEP
enum ErrorFlags : uint32_t {
    NO_ERROR = 0,
    REPETITION_DETECTED = 1 << 0,
    OSCILLATION_DETECTED = 1 << 1,
    CONTRADICTION_LOOP = 1 << 2,
    LOW_CONFIDENCE = 1 << 3,
    BEAM_EXHAUSTED = 1 << 4,
    DEPTH_LIMIT_REACHED = 1 << 5,
    MEMORY_EXHAUSTED = 1 << 6
};

// Neural Network Components
struct AttentionHead {
    std::vector<std::vector<float>> query_weights;   // Query transformation matrix
    std::vector<std::vector<float>> key_weights;     // Key transformation matrix  
    std::vector<std::vector<float>> value_weights;   // Value transformation matrix
    float dropout_rate = 0.1f;
    size_t head_dim = 64;
    
    AttentionHead(size_t model_dim, size_t head_dim) : head_dim(head_dim) {
        // Initialize weight matrices
        query_weights.resize(model_dim, std::vector<float>(head_dim, 0.0f));
        key_weights.resize(model_dim, std::vector<float>(head_dim, 0.0f));
        value_weights.resize(model_dim, std::vector<float>(head_dim, 0.0f));
        
        // Xavier initialization
        float scale = std::sqrt(2.0f / (model_dim + head_dim));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, scale);
        
        for (auto& row : query_weights) {
            for (auto& val : row) val = dist(gen);
        }
        for (auto& row : key_weights) {
            for (auto& val : row) val = dist(gen);
        }
        for (auto& row : value_weights) {
            for (auto& val : row) val = dist(gen);
        }
    }
};

struct MultiHeadAttention {
    std::vector<AttentionHead> heads;
    std::vector<std::vector<float>> output_weights;  // Final linear transformation
    size_t num_heads;
    size_t model_dim;
    
    MultiHeadAttention(size_t model_dim, size_t num_heads) 
        : num_heads(num_heads), model_dim(model_dim) {
        size_t head_dim = model_dim / num_heads;
        
        // Create attention heads
        for (size_t i = 0; i < num_heads; ++i) {
            heads.emplace_back(model_dim, head_dim);
        }
        
        // Initialize output projection
        output_weights.resize(model_dim, std::vector<float>(model_dim, 0.0f));
        float scale = std::sqrt(2.0f / model_dim);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, scale);
        
        for (auto& row : output_weights) {
            for (auto& val : row) val = dist(gen);
        }
    }
};

struct FeedForwardNetwork {
    std::vector<std::vector<float>> linear1_weights;  // First linear layer
    std::vector<std::vector<float>> linear2_weights;  // Second linear layer
    std::vector<float> bias1;                         // First layer bias
    std::vector<float> bias2;                         // Second layer bias
    size_t hidden_dim;
    size_t model_dim;
    
    FeedForwardNetwork(size_t model_dim, size_t hidden_dim) 
        : hidden_dim(hidden_dim), model_dim(model_dim) {
        
        // Initialize first linear layer (model_dim -> hidden_dim)
        linear1_weights.resize(model_dim, std::vector<float>(hidden_dim, 0.0f));
        bias1.resize(hidden_dim, 0.0f);
        
        // Initialize second linear layer (hidden_dim -> model_dim)
        linear2_weights.resize(hidden_dim, std::vector<float>(model_dim, 0.0f));
        bias2.resize(model_dim, 0.0f);
        
        // Xavier initialization
        float scale1 = std::sqrt(2.0f / (model_dim + hidden_dim));
        float scale2 = std::sqrt(2.0f / (hidden_dim + model_dim));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist1(0.0f, scale1);
        std::normal_distribution<float> dist2(0.0f, scale2);
        
        for (auto& row : linear1_weights) {
            for (auto& val : row) val = dist1(gen);
        }
        for (auto& val : bias1) val = dist1(gen);
        
        for (auto& row : linear2_weights) {
            for (auto& val : row) val = dist2(gen);
        }
        for (auto& val : bias2) val = dist2(gen);
    }
};

struct TransformerLayer {
    MultiHeadAttention self_attention;
    FeedForwardNetwork feed_forward;
    std::vector<float> layer_norm1_gamma;  // Layer normalization parameters
    std::vector<float> layer_norm1_beta;
    std::vector<float> layer_norm2_gamma;
    std::vector<float> layer_norm2_beta;
    size_t model_dim;
    
    TransformerLayer(size_t model_dim, size_t num_heads, size_t hidden_dim)
        : self_attention(model_dim, num_heads), 
          feed_forward(model_dim, hidden_dim),
          model_dim(model_dim) {
        
        // Initialize layer normalization parameters
        layer_norm1_gamma.resize(model_dim, 1.0f);
        layer_norm1_beta.resize(model_dim, 0.0f);
        layer_norm2_gamma.resize(model_dim, 1.0f);
        layer_norm2_beta.resize(model_dim, 0.0f);
    }
};

struct EmbeddingLayer {
    std::vector<std::vector<float>> token_embeddings;  // Token to vector mapping
    std::vector<std::vector<float>> positional_embeddings;  // Position to vector mapping
    size_t vocab_size;
    size_t model_dim;
    size_t max_seq_length;
    
    EmbeddingLayer(size_t vocab_size, size_t model_dim, size_t max_seq_length)
        : vocab_size(vocab_size), model_dim(model_dim), max_seq_length(max_seq_length) {
        
        // Initialize token embeddings
        token_embeddings.resize(vocab_size, std::vector<float>(model_dim, 0.0f));
        
        // Initialize positional embeddings
        positional_embeddings.resize(max_seq_length, std::vector<float>(model_dim, 0.0f));
        
        // Xavier initialization for token embeddings
        float scale = std::sqrt(2.0f / model_dim);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, scale);
        
        for (auto& embedding : token_embeddings) {
            for (auto& val : embedding) val = dist(gen);
        }
        
        // Sinusoidal positional encoding
        for (size_t pos = 0; pos < max_seq_length; ++pos) {
            for (size_t i = 0; i < model_dim; ++i) {
                if (i % 2 == 0) {
                    positional_embeddings[pos][i] = std::sin(pos / std::pow(10000.0f, i / static_cast<float>(model_dim)));
                } else {
                    positional_embeddings[pos][i] = std::cos(pos / std::pow(10000.0f, (i-1) / static_cast<float>(model_dim)));
                }
            }
        }
    }
};

// Neural Network Activation Functions
namespace NeuralActivations {
    inline float relu(float x) {
        return std::max(0.0f, x);
    }
    
    inline float gelu(float x) {
        return 0.5f * x * (1.0f + std::tanh(std::sqrt(2.0f / M_PI) * (x + 0.044715f * x * x * x)));
    }
    
    inline float swish(float x) {
        return x / (1.0f + std::exp(-x));
    }
    
    inline float sigmoid(float x) {
        return 1.0f / (1.0f + std::exp(-x));
    }
    
    inline float tanh_activation(float x) {
        return std::tanh(x);
    }
    
    // Softmax for attention weights
    inline std::vector<float> softmax(const std::vector<float>& logits) {
        std::vector<float> result(logits.size());
        float max_logit = *std::max_element(logits.begin(), logits.end());
        float sum_exp = 0.0f;
        
        for (size_t i = 0; i < logits.size(); ++i) {
            result[i] = std::exp(logits[i] - max_logit);
            sum_exp += result[i];
        }
        
        for (auto& val : result) {
            val /= sum_exp;
        }
        
        return result;
    }
    
    // Layer normalization
    inline std::vector<float> layer_norm(const std::vector<float>& input, 
                                        const std::vector<float>& gamma, 
                                        const std::vector<float>& beta, 
                                        float epsilon = 1e-5f) {
        std::vector<float> result(input.size());
        
        // Calculate mean
        float mean = 0.0f;
        for (float val : input) mean += val;
        mean /= input.size();
        
        // Calculate variance
        float variance = 0.0f;
        for (float val : input) {
            float diff = val - mean;
            variance += diff * diff;
        }
        variance /= input.size();
        
        // Normalize and scale
        float std_dev = std::sqrt(variance + epsilon);
        for (size_t i = 0; i < input.size(); ++i) {
            result[i] = gamma[i] * (input[i] - mean) / std_dev + beta[i];
        }
        
        return result;
    }
}

// Multi-Modal Token Types
enum class TokenModality : uint8_t {
    TEXT = 0,
    AUDIO = 1,
    IMAGE = 2,
    SENSOR = 3
};

// Audio token quantization (10ms frames)
struct AudioToken {
    uint16_t code = 0;           // 1-2 byte quantized code
    uint64_t timestamp_ms = 0;   // Temporal position
    float amplitude = 0.0f;      // Preserve for reconstruction
    NodeID graph_node = {};      // Link to graph memory
};

// Image token (compressed embedding)
struct ImageToken {
    std::vector<float> embedding; // 256-1024 dim embedding (1-4KB)
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t timestamp_ms = 0;
    NodeID graph_node = {};       // Link to graph memory
};

// Hierarchy formation: Phrase nodes (pooled tokens)
struct PhraseNode {
    std::vector<uint32_t> constituent_tokens; // Component tokens
    NodeID graph_node = {};                   // Link to graph
    float activation_frequency = 0.0f;        // How often this phrase occurs
    uint32_t cooccurrence_count = 0;          // Number of times seen together
    std::vector<float> pooled_embedding;      // Combined embedding
};

// Concept pooling: Abstract concepts from frequent phrases
struct ConceptNode {
    std::vector<NodeID> constituent_phrases;  // Phrases that form this concept
    std::vector<NodeID> constituent_concepts; // Sub-concepts (recursive)
    NodeID graph_node = {};                   // Link to graph
    float abstraction_level = 0.0f;           // 0=concrete, 1=highly abstract
    std::vector<float> concept_embedding;     // Hierarchical embedding
    std::string label;                        // Human-readable label
};

// Replayable thought trace
struct ThoughtTrace {
    NodeID trace_node = {};                   // Node representing this trace
    std::vector<NodeID> reasoning_path;       // Nodes traversed
    std::vector<EdgeID> edges_used;           // Edges followed
    std::vector<Rel> relation_types;          // Relation type at each step
    float confidence = 0.0f;                  // Overall confidence
    uint64_t timestamp = 0;                   // When this thought occurred
    std::string query;                        // Original query
    std::string conclusion;                   // Final output
    bool reinforced = false;                  // Whether this trace was strengthened
};

// Anchor classification
enum class AnchorType : uint8_t {
    CORE = 0,        // Immutable: tokenizer, I/O, file format
    SOFT = 1,        // Adaptable: weights, thresholds, hyperparameters
    META = 2         // Identity + continuity preservation
};

// Tokenization system
struct Tokenizer {
    std::unordered_map<std::string, uint32_t> token_to_id;
    std::unordered_map<uint32_t, std::string> id_to_token;
    uint32_t next_token_id = 0;
    size_t vocab_size = 0;
    
    // Special tokens
    static constexpr uint32_t UNK_TOKEN = 0;
    static constexpr uint32_t PAD_TOKEN = 1;
    static constexpr uint32_t BOS_TOKEN = 2;
    static constexpr uint32_t EOS_TOKEN = 3;
    
    Tokenizer() {
        // Initialize special tokens
        token_to_id["<UNK>"] = UNK_TOKEN;
        token_to_id["<PAD>"] = PAD_TOKEN;
        token_to_id["<BOS>"] = BOS_TOKEN;
        token_to_id["<EOS>"] = EOS_TOKEN;
        
        id_to_token[UNK_TOKEN] = "<UNK>";
        id_to_token[PAD_TOKEN] = "<PAD>";
        id_to_token[BOS_TOKEN] = "<BOS>";
        id_to_token[EOS_TOKEN] = "<EOS>";
        
        next_token_id = 4;
        vocab_size = 4;
    }
    
    std::vector<uint32_t> tokenize(const std::string& text) {
        std::vector<uint32_t> tokens;
        tokens.push_back(BOS_TOKEN);
        
        std::istringstream iss(text);
        std::string word;
        while (iss >> word) {
            // Simple word-level tokenization
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            
            if (token_to_id.find(word) == token_to_id.end()) {
                token_to_id[word] = next_token_id;
                id_to_token[next_token_id] = word;
                next_token_id++;
                vocab_size++;
            }
            
            tokens.push_back(token_to_id[word]);
        }
        
        tokens.push_back(EOS_TOKEN);
        return tokens;
    }
    
    std::string detokenize(const std::vector<uint32_t>& tokens) {
        std::string result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] == BOS_TOKEN || tokens[i] == EOS_TOKEN) continue;
            if (tokens[i] == PAD_TOKEN) break;
            
            if (i > 0 && tokens[i-1] != BOS_TOKEN) result += " ";
            result += id_to_token[tokens[i]];
        }
        return result;
    }
};

// Backpropagation and Training Components
struct GradientBuffer {
    std::vector<std::vector<float>> gradients;
    size_t model_dim;
    
    GradientBuffer(size_t dim) : model_dim(dim) {
        gradients.resize(dim, std::vector<float>(dim, 0.0f));
    }
    
    void zero() {
        for (auto& row : gradients) {
            std::fill(row.begin(), row.end(), 0.0f);
        }
    }
    
    void add(const GradientBuffer& other) {
        for (size_t i = 0; i < gradients.size(); ++i) {
            for (size_t j = 0; j < gradients[i].size(); ++j) {
                gradients[i][j] += other.gradients[i][j];
            }
        }
    }
    
    void scale(float factor) {
        for (auto& row : gradients) {
            for (auto& val : row) {
                val *= factor;
            }
        }
    }
};

struct AdamOptimizer {
    std::vector<std::vector<float>> m;  // First moment estimate
    std::vector<std::vector<float>> v;  // Second moment estimate
    float beta1 = 0.9f;
    float beta2 = 0.999f;
    float epsilon = 1e-8f;
    float learning_rate = 0.001f;
    size_t t = 0;  // Time step
    
    AdamOptimizer(size_t model_dim, float lr = 0.001f) : learning_rate(lr) {
        m.resize(model_dim, std::vector<float>(model_dim, 0.0f));
        v.resize(model_dim, std::vector<float>(model_dim, 0.0f));
    }
    
    void update(std::vector<std::vector<float>>& weights, const GradientBuffer& gradients) {
        t++;
        
        for (size_t i = 0; i < weights.size(); ++i) {
            for (size_t j = 0; j < weights[i].size(); ++j) {
                float grad = gradients.gradients[i][j];
                
                // Update biased first moment estimate
                m[i][j] = beta1 * m[i][j] + (1.0f - beta1) * grad;
                
                // Update biased second raw moment estimate
                v[i][j] = beta2 * v[i][j] + (1.0f - beta2) * grad * grad;
                
                // Compute bias-corrected first moment estimate
                float m_hat = m[i][j] / (1.0f - std::pow(beta1, t));
                
                // Compute bias-corrected second raw moment estimate
                float v_hat = v[i][j] / (1.0f - std::pow(beta2, t));
                
                // Update weights
                weights[i][j] -= learning_rate * m_hat / (std::sqrt(v_hat) + epsilon);
            }
        }
    }
};

struct LanguageModelHead {
    std::vector<std::vector<float>> projection_weights;  // Hidden to vocab projection
    std::vector<float> bias;
    size_t vocab_size;
    size_t model_dim;
    
    LanguageModelHead(size_t model_dim, size_t vocab_size) 
        : vocab_size(vocab_size), model_dim(model_dim) {
        
        projection_weights.resize(model_dim, std::vector<float>(vocab_size, 0.0f));
        bias.resize(vocab_size, 0.0f);
        
        // Xavier initialization
        float scale = std::sqrt(2.0f / (model_dim + vocab_size));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, scale);
        
        for (auto& row : projection_weights) {
            for (auto& val : row) val = dist(gen);
        }
        for (auto& val : bias) val = dist(gen);
    }
    
    std::vector<float> forward(const std::vector<float>& hidden_state) {
        std::vector<float> logits(vocab_size);
        
        for (size_t i = 0; i < vocab_size; ++i) {
            logits[i] = bias[i];
            for (size_t j = 0; j < model_dim; ++j) {
                logits[i] += hidden_state[j] * projection_weights[j][i];
            }
        }
        
        return logits;
    }
};

// Loss Functions
namespace LossFunctions {
    inline float cross_entropy_loss(const std::vector<float>& logits, uint32_t target_token) {
        if (target_token >= logits.size()) return 0.0f;
        
        // Compute softmax
        auto probs = NeuralActivations::softmax(logits);
        
        // Return negative log probability of target token
        return -std::log(std::max(probs[target_token], 1e-8f));
    }
    
    inline std::vector<float> cross_entropy_gradient(const std::vector<float>& logits, uint32_t target_token) {
        auto probs = NeuralActivations::softmax(logits);
        std::vector<float> grad(logits.size());
        
        for (size_t i = 0; i < logits.size(); ++i) {
            grad[i] = probs[i];
            if (i == target_token) {
                grad[i] -= 1.0f;
            }
        }
        
        return grad;
    }
    
    inline float mean_squared_error(const std::vector<float>& predictions, const std::vector<float>& targets) {
        if (predictions.size() != targets.size()) return 0.0f;
        
        float mse = 0.0f;
        for (size_t i = 0; i < predictions.size(); ++i) {
            float diff = predictions[i] - targets[i];
            mse += diff * diff;
        }
        
        return mse / predictions.size();
    }
}

// Dropout for regularization
struct Dropout {
    float dropout_rate;
    std::mt19937 gen;
    std::uniform_real_distribution<float> dist;
    
    Dropout(float rate = 0.1f) : dropout_rate(rate), gen(std::random_device{}()), dist(0.0f, 1.0f) {}
    
    std::vector<float> forward(const std::vector<float>& input, bool training = true) {
        if (!training || dropout_rate == 0.0f) {
            return input;
        }
        
        std::vector<float> output = input;
        float scale = 1.0f / (1.0f - dropout_rate);
        
        for (auto& val : output) {
            if (dist(gen) < dropout_rate) {
                val = 0.0f;
            } else {
                val *= scale;
            }
        }
        
        return output;
    }
};

// Learning Rate Scheduler
struct LearningRateScheduler {
    float initial_lr;
    float current_lr;
    size_t warmup_steps;
    size_t total_steps;
    float min_lr;
    
    LearningRateScheduler(float lr = 0.001f, size_t warmup = 1000, size_t total = 10000, float min_lr = 1e-6f)
        : initial_lr(lr), current_lr(lr), warmup_steps(warmup), total_steps(total), min_lr(min_lr) {}
    
    void step(size_t step_count) {
        if (step_count < warmup_steps) {
            // Linear warmup
            current_lr = initial_lr * step_count / warmup_steps;
        } else {
            // Cosine decay
            float progress = static_cast<float>(step_count - warmup_steps) / (total_steps - warmup_steps);
            current_lr = min_lr + (initial_lr - min_lr) * 0.5f * (1.0f + std::cos(M_PI * progress));
        }
        
        current_lr = std::max(current_lr, min_lr);
    }
    
    float get_lr() const { return current_lr; }
};

// Gradient Clipping
namespace GradientUtils {
    inline float compute_gradient_norm(const std::vector<std::vector<float>>& gradients) {
        float norm = 0.0f;
        for (const auto& row : gradients) {
            for (float val : row) {
                norm += val * val;
            }
        }
        return std::sqrt(norm);
    }
    
    inline void clip_gradients(std::vector<std::vector<float>>& gradients, float max_norm = 1.0f) {
        float norm = compute_gradient_norm(gradients);
        if (norm > max_norm) {
            float scale = max_norm / norm;
            for (auto& row : gradients) {
                for (auto& val : row) {
                    val *= scale;
                }
            }
        }
    }
}

// Utility functions for binary operations
template<typename ID>
inline ::std::enable_if_t<::std::is_same_v<ID, NodeID> || ::std::is_same_v<ID, EdgeID> || ::std::is_same_v<ID, PathID>, bool>
is_zero_id(const ID& id) {
    return ::std::all_of(id.begin(), id.end(), [](uint8_t b) { return b == 0; });
}

// Hash comparison for IDs
struct NodeIDHash {
    size_t operator()(const NodeID& id) const {
        size_t hash = 0;
        for (size_t i = 0; i < 8; ++i) {
            hash ^= (static_cast<size_t>(id[i*4]) << 24) |
                   (static_cast<size_t>(id[i*4+1]) << 16) |
                   (static_cast<size_t>(id[i*4+2]) << 8) |
                   static_cast<size_t>(id[i*4+3]);
        }
        return hash;
    }
};

} // namespace melvin

// Specialize std::hash for NodeID (outside melvin namespace)
namespace std {
    template<>
    struct hash<melvin::NodeID> {
        size_t operator()(const melvin::NodeID& id) const {
            size_t hash = 0;
            for (size_t i = 0; i < 8; ++i) {
                hash ^= (static_cast<size_t>(id[i*4]) << 24) |
                       (static_cast<size_t>(id[i*4+1]) << 16) |
                       (static_cast<size_t>(id[i*4+2]) << 8) |
                       static_cast<size_t>(id[i*4+3]);
            }
            return hash;
        }
    };
    
    template<>
    struct hash<std::pair<melvin::NodeID, melvin::NodeID>> {
        size_t operator()(const std::pair<melvin::NodeID, melvin::NodeID>& pair) const {
            std::hash<melvin::NodeID> hasher;
            size_t h1 = hasher(pair.first);
            size_t h2 = hasher(pair.second);
            return h1 ^ (h2 << 1);
        }
    };
}

namespace melvin {

struct EdgeIDHash {
    size_t operator()(const EdgeID& id) const {
        size_t hash = 0;
        for (size_t i = 0; i < 8; ++i) {
            hash ^= (static_cast<size_t>(id[i*4]) << 24) |
                   (static_cast<size_t>(id[i*4+1]) << 16) |
                   (static_cast<size_t>(id[i*4+2]) << 8) |
                   static_cast<size_t>(id[i*4+3]);
        }
        return hash;
    }
};

struct PathIDHash {
    size_t operator()(const PathID& id) const {
        size_t hash = 0;
        for (size_t i = 0; i < 8; ++i) {
            hash ^= (static_cast<size_t>(id[i*4]) << 24) |
                   (static_cast<size_t>(id[i*4+1]) << 16) |
                   (static_cast<size_t>(id[i*4+2]) << 8) |
                   static_cast<size_t>(id[i*4+3]);
        }
        return hash;
    }
};

// Equality operators for IDs
template<typename ID>
inline ::std::enable_if_t<::std::is_same_v<ID, NodeID> || ::std::is_same_v<ID, EdgeID> || ::std::is_same_v<ID, PathID>, bool>
operator==(const ID& a, const ID& b) {
    return ::std::equal(a.begin(), a.end(), b.begin());
}

template<typename ID>
inline ::std::enable_if_t<::std::is_same_v<ID, NodeID> || ::std::is_same_v<ID, EdgeID> || ::std::is_same_v<ID, PathID>, bool>
operator!=(const ID& a, const ID& b) {
    return !(a == b);
}

// Less-than operators for ordering (lexicographic)
template<typename ID>
inline ::std::enable_if_t<::std::is_same_v<ID, NodeID> || ::std::is_same_v<ID, EdgeID> || ::std::is_same_v<ID, PathID>, bool>
operator<(const ID& a, const ID& b) {
    return ::std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

} // namespace melvin
