#pragma once

#include "adaptive_window_config.h"
#include "types.h"
#include "optimized_storage.h"
#include <deque>
#include <unordered_map>
#include <vector>
#include <cmath>

namespace melvin {
namespace adaptive {

/**
 * ⚡ Adaptive Window Manager
 * 
 * Manages temporal buffer and calculates adaptive EXACT window sizes
 * for context-aware sequential edge creation.
 * 
 * Key responsibilities:
 * - Maintain sliding window of recent nodes
 * - Calculate adaptive N_exact for each new node
 * - Generate EXACT edges with temporal decay weights
 * - Track statistics for tuning
 */
class AdaptiveWindowManager {
public:
    /**
     * Constructor
     * 
     * @param storage Pointer to optimized storage
     * @param config Adaptive window configuration
     */
    AdaptiveWindowManager(optimized::OptimizedStorage* storage, 
                         const AdaptiveWindowConfig& config = AdaptiveWindowConfig())
        : storage_(storage), config_(config) {
        
        if (!config_.validate()) {
            throw std::runtime_error("Invalid AdaptiveWindowConfig");
        }
        
        if (config_.verbose_logging) {
            config_.print();
        }
    }
    
    /**
     * Add a new node to the temporal buffer and create EXACT edges
     * 
     * @param node_id The new node to add
     * @return Vector of edges to create (from, to, relation, weight)
     */
    std::vector<std::tuple<NodeID, NodeID, RelationType, float>> 
    add_node_to_buffer(NodeID node_id) {
        std::vector<std::tuple<NodeID, NodeID, RelationType, float>> edges;
        
        // Update node activation count
        storage_->increment_node_activation(node_id);
        
        // Calculate novelty and strength for this node
        float novelty = calculate_novelty(node_id);
        float strength = calculate_strength(node_id);
        
        // Calculate adaptive window size
        size_t window_size = config_.calculate_window_size(novelty, strength);
        
        // Cache window size if enabled
        if (config_.cache_window_sizes) {
            cached_window_sizes_[node_id] = window_size;
        }
        
        // Update statistics
        if (config_.enable_stats) {
            stats_.update_window_size(window_size);
        }
        
        if (config_.verbose_logging) {
            printf("  Node %u: novelty=%.3f, strength=%.3f, window=%zu\n",
                   node_id, novelty, strength, window_size);
        }
        
        // Create EXACT edges to previous nodes within window
        size_t connections = std::min(window_size, temporal_buffer_.size());
        
        for (size_t i = 0; i < connections; ++i) {
            NodeID prev_node = temporal_buffer_[temporal_buffer_.size() - 1 - i];
            
            // Calculate temporal weight (exponential decay)
            float weight = config_.calculate_temporal_weight(i + 1);
            
            if (weight >= config_.min_edge_weight) {
                edges.emplace_back(prev_node, node_id, RelationType::EXACT, weight);
                
                if (config_.enable_stats) {
                    stats_.update_edge_weight(weight);
                }
            } else if (config_.enable_stats) {
                stats_.total_edges_skipped++;
            }
        }
        
        // Add node to temporal buffer
        temporal_buffer_.push_back(node_id);
        
        // Keep buffer size manageable (2x max window size)
        size_t max_buffer_size = config_.N_max * 2;
        if (temporal_buffer_.size() > max_buffer_size) {
            temporal_buffer_.pop_front();
        }
        
        return edges;
    }
    
    /**
     * Add multiple nodes to buffer (batch processing)
     * 
     * @param node_ids Vector of node IDs to add
     * @return Vector of all edges to create
     */
    std::vector<std::tuple<NodeID, NodeID, RelationType, float>>
    add_nodes_to_buffer(const std::vector<NodeID>& node_ids) {
        std::vector<std::tuple<NodeID, NodeID, RelationType, float>> all_edges;
        
        for (NodeID node_id : node_ids) {
            auto edges = add_node_to_buffer(node_id);
            all_edges.insert(all_edges.end(), edges.begin(), edges.end());
        }
        
        return all_edges;
    }
    
    /**
     * Clear the temporal buffer
     */
    void clear_buffer() {
        temporal_buffer_.clear();
        cached_window_sizes_.clear();
    }
    
    /**
     * Get current buffer size
     */
    size_t buffer_size() const {
        return temporal_buffer_.size();
    }
    
    /**
     * Get configuration
     */
    const AdaptiveWindowConfig& get_config() const {
        return config_;
    }
    
    /**
     * Update configuration
     */
    void set_config(const AdaptiveWindowConfig& config) {
        if (!config.validate()) {
            throw std::runtime_error("Invalid AdaptiveWindowConfig");
        }
        config_ = config;
        cached_window_sizes_.clear();  // Invalidate cache
    }
    
    /**
     * Get statistics
     */
    const AdaptiveWindowStats& get_stats() const {
        return stats_;
    }
    
    /**
     * Print statistics
     */
    void print_stats() const {
        if (config_.enable_stats) {
            stats_.print();
        } else {
            printf("Statistics tracking is disabled. Enable with config.enable_stats = true\n");
        }
    }
    
    /**
     * Reset statistics
     */
    void reset_stats() {
        stats_.reset();
    }
    
private:
    optimized::OptimizedStorage* storage_;
    AdaptiveWindowConfig config_;
    AdaptiveWindowStats stats_;
    
    // Temporal buffer: sliding window of recent nodes
    std::deque<NodeID> temporal_buffer_;
    
    // Cached window sizes for nodes (if caching enabled)
    std::unordered_map<NodeID, size_t> cached_window_sizes_;
    
    /**
     * Calculate novelty score for a node
     * Based on activation count
     */
    float calculate_novelty(NodeID node_id) const {
        uint64_t activations = storage_->get_node_activation_count(node_id);
        return config_.calculate_novelty(activations);
    }
    
    /**
     * Calculate strength score for a node
     * Based on edge weights and/or activation frequency
     */
    float calculate_strength(NodeID node_id) const {
        float edge_strength = storage_->calculate_node_strength(node_id);
        
        switch (config_.strength_mode) {
            case 0:
                // Mode 0: Use only edge weights
                return edge_strength;
                
            case 1: {
                // Mode 1: Use only activation frequency
                uint64_t activations = storage_->get_node_activation_count(node_id);
                float max_activations = 1000.0f;  // Normalize to 1000 activations
                return std::min(1.0f, static_cast<float>(activations) / max_activations);
            }
            
            case 2:
            default: {
                // Mode 2: Weighted combination
                uint64_t activations = storage_->get_node_activation_count(node_id);
                float max_activations = 1000.0f;
                float activation_freq = std::min(1.0f, static_cast<float>(activations) / max_activations);
                
                return edge_strength * config_.strength_blend + 
                       activation_freq * (1.0f - config_.strength_blend);
            }
        }
    }
};

}} // namespace melvin::adaptive

