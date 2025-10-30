#include "global_activation_field.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>

namespace melvin {
namespace cognitive_field {

GlobalActivationField::GlobalActivationField(size_t embedding_dim)
    : embedding_dim_(embedding_dim) {
    working_buffer_.reserve(WORKING_BUFFER_SIZE);
}

// ============================================================================
// Core Activation Interface
// ============================================================================

void GlobalActivationField::inject_energy(int node_id, float energy,
                                         const std::vector<float>& embedding,
                                         int modality) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto& node = nodes_[node_id];
    node.energy += energy;
    node.activation = std::tanh(node.energy / 10.0f);  // Sigmoid-like
    node.modality = modality;
    node.last_active = std::chrono::high_resolution_clock::now();
    
    // Initialize or update embedding
    if (node.embedding.empty()) {
        node.embedding = embedding;
        node.history_index = 0;
        for (int i = 0; i < 10; ++i) node.activation_history[i] = 0.0f;
    } else {
        // Blend embeddings (moving average)
        for (size_t i = 0; i < std::min(embedding.size(), node.embedding.size()); ++i) {
            node.embedding[i] = node.embedding[i] * 0.9f + embedding[i] * 0.1f;
        }
    }
    
    update_activation_history(node);
}

float GlobalActivationField::get_activation(int node_id) const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    auto it = nodes_.find(node_id);
    return (it != nodes_.end()) ? it->second.activation : 0.0f;
}

float GlobalActivationField::get_energy(int node_id) const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    auto it = nodes_.find(node_id);
    return (it != nodes_.end()) ? it->second.energy : 0.0f;
}

std::vector<float> GlobalActivationField::get_embedding(int node_id) const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    auto it = nodes_.find(node_id);
    return (it != nodes_.end()) ? it->second.embedding : std::vector<float>();
}

// ============================================================================
// Continuous Dynamics
// ============================================================================

void GlobalActivationField::update(float dt) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    // Decay all activations
    for (auto& pair : nodes_) {
        auto& node = pair.second;
        node.energy *= std::pow(decay_rate_, dt * 30.0f);  // Scale by frame rate
        node.activation = std::tanh(node.energy / 10.0f);
        
        // Remove dead nodes
        if (node.activation < min_activation_) {
            node.energy = 0.0f;
            node.activation = 0.0f;
        }
    }
    
    // Update working buffer
    update_working_buffer();
}

void GlobalActivationField::spread_activation(int source_id,
                                             const std::vector<int>& neighbor_ids,
                                             const std::vector<float>& edge_weights,
                                             float spread_rate) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = nodes_.find(source_id);
    if (it == nodes_.end() || it->second.activation < min_activation_) {
        return;
    }
    
    float source_energy = it->second.energy;
    float energy_to_spread = source_energy * spread_rate;
    
    for (size_t i = 0; i < neighbor_ids.size(); ++i) {
        int neighbor_id = neighbor_ids[i];
        float weight = edge_weights[i];
        
        // Energy flows proportional to edge weight
        float transferred_energy = energy_to_spread * weight;
        
        auto& neighbor = nodes_[neighbor_id];
        neighbor.energy += transferred_energy;
        neighbor.activation = std::tanh(neighbor.energy / 10.0f);
        neighbor.last_active = std::chrono::high_resolution_clock::now();
        
        update_activation_history(neighbor);
    }
    
    // Source loses energy
    it->second.energy -= energy_to_spread;
}

// ============================================================================
// Multi-Modal Resonance
// ============================================================================

float GlobalActivationField::compute_binding_strength(int node_a, int node_b) const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it_a = nodes_.find(node_a);
    auto it_b = nodes_.find(node_b);
    
    if (it_a == nodes_.end() || it_b == nodes_.end()) {
        return 0.0f;
    }
    
    const auto& a = it_a->second;
    const auto& b = it_b->second;
    
    // 1. Co-activation strength (from history)
    float co_activation = 0.0f;
    for (int i = 0; i < 10; ++i) {
        co_activation += a.activation_history[i] * b.activation_history[i];
    }
    co_activation /= 10.0f;
    
    // 2. Temporal overlap
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
        a.last_active - b.last_active
    ).count();
    float temporal_overlap = std::exp(-std::abs(time_diff) / 200.0f);  // 200ms window
    
    // 3. Embedding similarity
    float embedding_sim = compute_cosine_similarity(a.embedding, b.embedding);
    
    // Combined binding strength
    return co_activation * 0.4f + temporal_overlap * 0.3f + embedding_sim * 0.3f;
}

std::vector<std::pair<int, float>> GlobalActivationField::find_resonant_nodes(
    int query_node, float threshold) const {
    
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    std::vector<std::pair<int, float>> resonant;
    
    for (const auto& pair : nodes_) {
        int node_id = pair.first;
        if (node_id == query_node) continue;
        
        const auto& node = pair.second;
        if (node.activation < threshold) continue;
        
        float binding = compute_binding_strength(query_node, node_id);
        if (binding > resonance_threshold_) {
            resonant.emplace_back(node_id, binding);
        }
    }
    
    // Sort by binding strength
    std::sort(resonant.begin(), resonant.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return resonant;
}

// ============================================================================
// Context Integration
// ============================================================================

std::vector<float> GlobalActivationField::compute_context_vector() const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    std::vector<float> context(embedding_dim_, 0.0f);
    float total_activation = 0.0f;
    
    for (const auto& pair : nodes_) {
        const auto& node = pair.second;
        if (node.activation < min_activation_) continue;
        
        for (size_t i = 0; i < embedding_dim_; ++i) {
            context[i] += node.embedding[i] * node.activation;
        }
        total_activation += node.activation;
    }
    
    // Normalize
    if (total_activation > 0.0f) {
        for (float& val : context) {
            val /= total_activation;
        }
    }
    
    return context;
}

std::vector<int> GlobalActivationField::get_top_active_nodes(size_t k) const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    std::vector<std::pair<int, float>> node_activations;
    for (const auto& pair : nodes_) {
        if (pair.second.activation > min_activation_) {
            node_activations.emplace_back(pair.first, pair.second.activation);
        }
    }
    
    std::sort(node_activations.begin(), node_activations.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<int> top_nodes;
    for (size_t i = 0; i < std::min(k, node_activations.size()); ++i) {
        top_nodes.push_back(node_activations[i].first);
    }
    
    return top_nodes;
}

std::unordered_map<int, float> GlobalActivationField::propagate_context(
    int seed_node, int hops) const {
    
    // Multi-hop context propagation
    std::unordered_map<int, float> context_activations;
    context_activations[seed_node] = 1.0f;
    
    // BFS with activation decay
    std::unordered_map<int, int> visited;
    visited[seed_node] = 0;
    
    std::vector<int> current_frontier = {seed_node};
    
    for (int hop = 0; hop < hops; ++hop) {
        std::vector<int> next_frontier;
        
        for (int node_id : current_frontier) {
            float current_activation = context_activations[node_id];
            float decayed_activation = current_activation * 0.7f;  // Decay per hop
            
            // Find neighbors (would need edge data structure here)
            // For now, use similarity-based neighbors
            auto similar = find_similar_nodes(node_id, 10, 0.3f);
            
            for (const auto& [neighbor_id, similarity] : similar) {
                if (visited.find(neighbor_id) == visited.end()) {
                    visited[neighbor_id] = hop + 1;
                    context_activations[neighbor_id] = decayed_activation * similarity;
                    next_frontier.push_back(neighbor_id);
                }
            }
        }
        
        current_frontier = std::move(next_frontier);
    }
    
    return context_activations;
}

// ============================================================================
// Working Context Buffer
// ============================================================================

std::vector<GlobalActivationField::WorkingConcept> 
GlobalActivationField::get_working_buffer() const {
    return working_buffer_;
}

void GlobalActivationField::update_working_buffer() {
    // Get top active nodes
    auto top_nodes = get_top_active_nodes(WORKING_BUFFER_SIZE * 2);
    
    working_buffer_.clear();
    auto now = std::chrono::high_resolution_clock::now();
    
    for (int node_id : top_nodes) {
        if (working_buffer_.size() >= WORKING_BUFFER_SIZE) break;
        
        auto it = nodes_.find(node_id);
        if (it == nodes_.end()) continue;
        
        WorkingConcept concept;
        concept.node_id = node_id;
        concept.activation = it->second.activation;
        concept.decay_rate = 0.9f;
        concept.last_update = now;
        
        working_buffer_.push_back(concept);
    }
}

// ============================================================================
// Embedding-Based Similarity
// ============================================================================

float GlobalActivationField::cosine_similarity(int node_a, int node_b) const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it_a = nodes_.find(node_a);
    auto it_b = nodes_.find(node_b);
    
    if (it_a == nodes_.end() || it_b == nodes_.end()) {
        return 0.0f;
    }
    
    return compute_cosine_similarity(it_a->second.embedding, it_b->second.embedding);
}

std::vector<std::pair<int, float>> GlobalActivationField::find_similar_nodes(
    int query_node, size_t k, float min_similarity) const {
    
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it_query = nodes_.find(query_node);
    if (it_query == nodes_.end()) {
        return {};
    }
    
    const auto& query_emb = it_query->second.embedding;
    std::vector<std::pair<int, float>> similarities;
    
    for (const auto& pair : nodes_) {
        if (pair.first == query_node) continue;
        
        float sim = compute_cosine_similarity(query_emb, pair.second.embedding);
        if (sim >= min_similarity) {
            similarities.emplace_back(pair.first, sim);
        }
    }
    
    std::sort(similarities.begin(), similarities.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (similarities.size() > k) {
        similarities.resize(k);
    }
    
    return similarities;
}

// ============================================================================
// Statistics
// ============================================================================

GlobalActivationField::Stats GlobalActivationField::get_stats() const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    Stats stats = {0, 0, 0.0f, 0.0f, 0.0f, 0.0f};
    stats.total_nodes = nodes_.size();
    
    for (const auto& pair : nodes_) {
        const auto& node = pair.second;
        stats.total_energy += node.energy;
        
        if (node.activation > min_activation_) {
            stats.active_nodes++;
            stats.avg_activation += node.activation;
            stats.max_activation = std::max(stats.max_activation, node.activation);
        }
    }
    
    if (stats.active_nodes > 0) {
        stats.avg_activation /= stats.active_nodes;
    }
    
    return stats;
}

void GlobalActivationField::reset() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    nodes_.clear();
    working_buffer_.clear();
}

// ============================================================================
// Helper Functions
// ============================================================================

float GlobalActivationField::compute_cosine_similarity(
    const std::vector<float>& a, const std::vector<float>& b) const {
    
    if (a.size() != b.size() || a.empty()) {
        return 0.0f;
    }
    
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    if (norm_a == 0.0f || norm_b == 0.0f) {
        return 0.0f;
    }
    
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

void GlobalActivationField::update_activation_history(NodeState& node) {
    node.activation_history[node.history_index] = node.activation;
    node.history_index = (node.history_index + 1) % 10;
}

} // namespace cognitive_field
} // namespace melvin

