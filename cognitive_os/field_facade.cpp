/**
 * @file field_facade.cpp
 * @brief Implementation of thread-safe activation field
 */

#include "field_facade.h"
#include <algorithm>
#include <cmath>

namespace melvin {
namespace cognitive_os {

FieldFacade::FieldFacade(
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings
) : graph_(graph), embeddings_(embeddings) {}

void FieldFacade::activate(int node_id, float delta, const std::string& source) {
    std::lock_guard<std::mutex> lock(mutex_);
    activations_[node_id] += delta;
    activation_count_.fetch_add(1, std::memory_order_relaxed);
}

float FieldFacade::get_activation(int node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = activations_.find(node_id);
    return (it != activations_.end()) ? it->second : 0.0f;
}

std::vector<int> FieldFacade::get_active(float threshold) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<int> active;
    for (const auto& [node_id, activation] : activations_) {
        if (activation >= threshold) {
            active.push_back(node_id);
        }
    }
    
    return active;
}

std::unordered_map<int, float> FieldFacade::get_activations(const std::vector<int>& node_ids) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<int, float> result;
    for (int node_id : node_ids) {
        auto it = activations_.find(node_id);
        if (it != activations_.end()) {
            result[node_id] = it->second;
        }
    }
    
    return result;
}

void FieldFacade::decay(float decay_rate) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<int> to_remove;
    for (auto& [node_id, activation] : activations_) {
        activation *= (1.0f - decay_rate);
        if (activation < 0.001f) {
            to_remove.push_back(node_id);
        }
    }
    
    for (int node_id : to_remove) {
        activations_.erase(node_id);
    }
}

void FieldFacade::normalize_degrees() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [node_id, activation] : activations_) {
        auto it = graph_.find(node_id);
        if (it != graph_.end()) {
            size_t degree = it->second.size();
            if (degree > 0) {
                activation /= std::sqrt(static_cast<float>(degree));
            }
        }
    }
}

void FieldFacade::apply_kwta(int k) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (activations_.empty()) return;
    
    // Sort by activation
    std::vector<std::pair<int, float>> sorted;
    for (const auto& [node_id, activation] : activations_) {
        sorted.push_back({node_id, activation});
    }
    
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Keep top k, inhibit rest
    size_t keep_count = std::min(static_cast<size_t>(k), sorted.size());
    
    std::unordered_map<int, float> new_activations;
    for (size_t i = 0; i < keep_count; i++) {
        new_activations[sorted[i].first] = sorted[i].second;
    }
    
    activations_ = new_activations;
}

FieldFacade::Metrics FieldFacade::get_metrics() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Metrics m;
    m.active_nodes = activations_.size();
    
    if (activations_.empty()) {
        m.energy_variance = 0.0f;
        m.sparsity = 1.0f;
        m.entropy = 0.0f;
        m.mean_activation = 0.0f;
        m.max_activation = 0.0f;
        return m;
    }
    
    // Compute statistics
    float sum = 0.0f;
    float max_act = 0.0f;
    
    for (const auto& [node_id, activation] : activations_) {
        sum += activation;
        if (activation > max_act) {
            max_act = activation;
        }
    }
    
    m.mean_activation = sum / activations_.size();
    m.max_activation = max_act;
    
    // Variance
    float var_sum = 0.0f;
    for (const auto& [node_id, activation] : activations_) {
        float diff = activation - m.mean_activation;
        var_sum += diff * diff;
    }
    m.energy_variance = std::sqrt(var_sum / activations_.size());
    
    // Sparsity (proportion inactive)
    size_t total_nodes = graph_.size();
    m.sparsity = 1.0f - (static_cast<float>(activations_.size()) / total_nodes);
    
    // Entropy
    m.entropy = 0.0f;
    for (const auto& [node_id, activation] : activations_) {
        if (activation > 0.001f && sum > 0.001f) {
            float p = activation / sum;
            m.entropy -= p * std::log2(p);
        }
    }
    
    return m;
}

void FieldFacade::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    activations_.clear();
}

} // namespace cognitive_os
} // namespace melvin

