#pragma once

#include "audio_tokenizer.h"
#include "../core/atomic_graph.h"
#include <vector>
#include <string>

namespace melvin {
namespace audio {

// Forward declarations
class PhonemeGraph;
struct PhonemeNode;

/**
 * PhonemeCluster - Learned phoneme prototype from audio patterns
 * 
 * Discovered through unsupervised clustering of audio tokens.
 * Each cluster represents a recurring sound pattern (like a phoneme).
 */
struct PhonemeCluster {
    uint64_t cluster_id;                // Graph node ID
    std::string symbol;                 // Learned symbol (e.g., "cluster_0")
    std::vector<float> centroid;        // Mean feature vector
    std::vector<float> variance;        // Feature variance
    std::vector<uint64_t> members;      // Token IDs in this cluster
    size_t occurrence_count;            // How many times observed
    
    PhonemeCluster() : cluster_id(0), occurrence_count(0) {}
};

/**
 * PhonemeClusterer - Unsupervised phoneme discovery
 * 
 * Philosophy:
 * - Melvin doesn't need pre-defined phonemes
 * - He discovers them from patterns in audio
 * - Repeated similar sounds → phoneme prototype
 * - Pure bottom-up learning
 * 
 * Process:
 * 1. Collect audio tokens over time
 * 2. Cluster similar tokens (k-means, DBSCAN)
 * 3. Create phoneme node for each cluster
 * 4. Link tokens → phoneme [INSTANCE_OF]
 * 5. Use clusters for synthesis and recognition
 */
class PhonemeClusterer {
public:
    /**
     * Configuration for clustering
     */
    struct Config {
        size_t num_clusters = 50;           // Target number of phoneme clusters
        float similarity_threshold = 0.7f;   // Minimum similarity for same cluster
        size_t min_cluster_size = 3;        // Minimum tokens to form cluster
        size_t max_iterations = 100;        // K-means iterations
        bool enable_online_learning = true; // Update clusters incrementally
    };
    
    PhonemeClusterer();
    explicit PhonemeClusterer(const Config& config);
    ~PhonemeClusterer();
    
    // ========================================================================
    // CLUSTERING
    // ========================================================================
    
    /**
     * Cluster audio tokens into phoneme prototypes
     * 
     * @param tokens Audio tokens to cluster
     * @param graph Target graph
     * @return Vector of cluster IDs
     */
    std::vector<uint64_t> cluster_tokens(const std::vector<AudioToken>& tokens,
                                        AtomicGraph& graph);
    
    /**
     * Assign token to nearest cluster
     * 
     * @param token Audio token
     * @return Cluster ID (0 if no match)
     */
    uint64_t assign_to_cluster(const AudioToken& token);
    
    /**
     * Create new cluster from token
     * 
     * @param token Seed token
     * @param graph Target graph
     * @return Cluster ID
     */
    uint64_t create_cluster(const AudioToken& token, AtomicGraph& graph);
    
    // ========================================================================
    // CLUSTER MANAGEMENT
    // ========================================================================
    
    /**
     * Update cluster centroid with new member
     * 
     * @param cluster_id Cluster to update
     * @param token New member
     */
    void update_cluster(uint64_t cluster_id, const AudioToken& token);
    
    /**
     * Get cluster by ID
     */
    const PhonemeCluster* get_cluster(uint64_t cluster_id) const;
    
    /**
     * Get all clusters
     */
    const std::vector<PhonemeCluster>& get_clusters() const { return clusters_; }
    
    // ========================================================================
    // ANALYSIS
    // ========================================================================
    
    /**
     * Find most common phoneme clusters
     * 
     * @param top_n Number of top clusters to return
     * @return Vector of cluster IDs
     */
    std::vector<uint64_t> get_most_common(size_t top_n = 10);
    
    /**
     * Compute cluster quality (tightness)
     * 
     * @param cluster_id Cluster to analyze
     * @return Quality score (0.0 to 1.0)
     */
    float compute_cluster_quality(uint64_t cluster_id);
    
    // ========================================================================
    // GRAPH INTEGRATION
    // ========================================================================
    
    /**
     * Link tokens to their clusters in graph
     * 
     * @param graph Target graph
     */
    void link_to_graph(AtomicGraph& graph);
    
    /**
     * Export clusters as phoneme nodes
     * 
     * @param phoneme_graph Target phoneme graph
     */
    void export_to_phonemes(PhonemeGraph& phoneme_graph);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t cluster_count() const { return clusters_.size(); }
    size_t total_tokens_clustered() const;
    
    void print_stats() const;
    void print_cluster_details() const;
    
private:
    Config config_;
    
    std::vector<PhonemeCluster> clusters_;
    uint64_t next_cluster_id_ = 1;
    
    // Helper methods
    float compute_distance(const std::vector<float>& f1, const std::vector<float>& f2);
    void update_centroid(PhonemeCluster& cluster);
    std::string generate_cluster_symbol(size_t cluster_index);
};

} // namespace audio
} // namespace melvin

