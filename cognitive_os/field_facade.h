/**
 * @file field_facade.h
 * @brief Thread-safe wrapper around global activation field
 * 
 * Shared by ALL services for reading/writing activations
 */

#ifndef MELVIN_FIELD_FACADE_H
#define MELVIN_FIELD_FACADE_H

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>

namespace melvin {
namespace cognitive_os {

/**
 * @brief Thread-safe activation field
 * 
 * All services read/write through this facade
 */
class FieldFacade {
public:
    FieldFacade(
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings
    );
    
    ~FieldFacade() = default;
    
    /**
     * @brief Activate a node (thread-safe)
     * 
     * @param node_id Node to activate
     * @param delta Energy to add
     * @param source Source identifier (e.g., "vision", "audio")
     */
    void activate(int node_id, float delta, const std::string& source = "");
    
    /**
     * @brief Get current activation level
     */
    float get_activation(int node_id);
    
    /**
     * @brief Get all active nodes above threshold
     */
    std::vector<int> get_active(float threshold = 0.01f);
    
    /**
     * @brief Get activation levels for multiple nodes
     */
    std::unordered_map<int, float> get_activations(const std::vector<int>& node_ids);
    
    /**
     * @brief Decay all activations (called by scheduler)
     */
    void decay(float decay_rate);
    
    /**
     * @brief Apply degree normalization (spread energy)
     */
    void normalize_degrees();
    
    /**
     * @brief Apply k-WTA inhibition
     */
    void apply_kwta(int k);
    
    /**
     * @brief Get field metrics
     */
    struct Metrics {
        int active_nodes;
        float energy_variance;
        float sparsity;
        float entropy;
        float mean_activation;
        float max_activation;
    };
    
    Metrics get_metrics();
    
    /**
     * @brief Clear all activations
     */
    void clear();
    
    /**
     * @brief Get graph reference
     */
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph() const {
        return graph_;
    }
    
    const std::unordered_map<int, std::vector<float>>& embeddings() const {
        return embeddings_;
    }
    
private:
    std::unordered_map<int, std::vector<std::pair<int, float>>> graph_;
    std::unordered_map<int, std::vector<float>> embeddings_;
    std::unordered_map<int, float> activations_;
    std::mutex mutex_;
    
    std::atomic<uint64_t> activation_count_{0};
};

} // namespace cognitive_os
} // namespace melvin

#endif // MELVIN_FIELD_FACADE_H

