/*
 * MELVIN SEMANTIC CLUSTERS
 * 
 * Groups activated nodes into semantic clusters for biasing:
 * - Clusters nodes by shared neighbor similarity
 * - Caches cluster centroids between turns for multi-turn context
 * - Computes bias weights for token predictions
 */

#pragma once
#include "../../include/melvin_storage.h"
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>

namespace melvin_prediction {

using namespace melvin_storage;

// ==================== SEMANTIC CLUSTER ====================

struct SemanticCluster {
    std::vector<int> member_nodes;
    std::vector<float> member_weights;
    float centroid_activation = 0.0f;
    int dominant_node = -1;
    uint64_t last_update = 0;
    
    SemanticCluster() {
        last_update = std::chrono::system_clock::now().time_since_epoch().count();
    }
    
    void update_centroid() {
        centroid_activation = 0.0f;
        dominant_node = -1;
        float max_weight = 0.0f;
        
        for (size_t i = 0; i < member_nodes.size(); ++i) {
            centroid_activation += member_weights[i];
            if (member_weights[i] > max_weight) {
                max_weight = member_weights[i];
                dominant_node = member_nodes[i];
            }
        }
        
        if (!member_nodes.empty()) {
            centroid_activation /= member_nodes.size();
        }
    }
    
    void decay(float rate = 0.95f) {
        for (auto& w : member_weights) {
            w *= rate;
        }
        centroid_activation *= rate;
        last_update = std::chrono::system_clock::now().time_since_epoch().count();
    }
};

// ==================== CLUSTER MANAGER ====================

class ClusterManager {
private:
    // Cached clusters from previous turns
    std::vector<SemanticCluster> cached_clusters_;
    
    // Decay rate for cached clusters
    float decay_rate_ = 0.95f;
    
    // Compute Jaccard similarity between two nodes (shared neighbors)
    float compute_similarity(int node_a, int node_b) const {
        if (node_a == node_b) return 1.0f;
        if (node_a < 0 || node_a >= (int)nodes.size()) return 0.0f;
        if (node_b < 0 || node_b >= (int)nodes.size()) return 0.0f;
        
        std::set<int> neighbors_a, neighbors_b;
        
        // Get neighbors of node_a
        auto it_a = adjacency.find(node_a);
        if (it_a != adjacency.end()) {
            for (int edge_id : it_a->second) {
                if (edge_id >= 0 && edge_id < (int)edges.size()) {
                    neighbors_a.insert(edges[edge_id].b);
                }
            }
        }
        
        // Get neighbors of node_b
        auto it_b = adjacency.find(node_b);
        if (it_b != adjacency.end()) {
            for (int edge_id : it_b->second) {
                if (edge_id >= 0 && edge_id < (int)edges.size()) {
                    neighbors_b.insert(edges[edge_id].b);
                }
            }
        }
        
        // Compute Jaccard similarity
        std::vector<int> intersection;
        std::set_intersection(neighbors_a.begin(), neighbors_a.end(),
                             neighbors_b.begin(), neighbors_b.end(),
                             std::back_inserter(intersection));
        
        if (neighbors_a.empty() && neighbors_b.empty()) return 0.0f;
        
        size_t union_size = neighbors_a.size() + neighbors_b.size() - intersection.size();
        return union_size > 0 ? (float)intersection.size() / union_size : 0.0f;
    }
    
public:
    ClusterManager(float decay_rate = 0.95f) : decay_rate_(decay_rate) {}
    
    // ==================== CLUSTER FORMATION ====================
    
    // Form clusters from activated nodes using shared neighbor similarity
    std::vector<SemanticCluster> form_clusters(
        const std::vector<float>& context_field,
        float min_activation = 0.1f
    ) {
        std::vector<SemanticCluster> clusters;
        
        // Get activated nodes
        std::vector<std::pair<int, float>> activated;
        for (size_t i = 0; i < context_field.size(); ++i) {
            if (context_field[i] >= min_activation) {
                activated.push_back({(int)i, context_field[i]});
            }
        }
        
        if (activated.empty()) return clusters;
        
        // Sort by activation (highest first)
        std::sort(activated.rbegin(), activated.rend(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });
        
        // Greedy clustering: group similar nodes
        std::vector<bool> assigned(activated.size(), false);
        
        for (size_t i = 0; i < activated.size(); ++i) {
            if (assigned[i]) continue;
            
            SemanticCluster cluster;
            cluster.member_nodes.push_back(activated[i].first);
            cluster.member_weights.push_back(activated[i].second);
            assigned[i] = true;
            
            // Find similar nodes to add to this cluster
            for (size_t j = i + 1; j < activated.size(); ++j) {
                if (assigned[j]) continue;
                
                float similarity = compute_similarity(activated[i].first, activated[j].first);
                
                // If similar enough, add to cluster
                if (similarity > 0.3f) {
                    cluster.member_nodes.push_back(activated[j].first);
                    cluster.member_weights.push_back(activated[j].second);
                    assigned[j] = true;
                }
            }
            
            cluster.update_centroid();
            clusters.push_back(cluster);
        }
        
        return clusters;
    }
    
    // ==================== BIAS COMPUTATION ====================
    
    // Compute bias weights for tokens based on cluster membership
    std::map<int, float> compute_cluster_bias(
        const std::vector<SemanticCluster>& clusters
    ) const {
        std::map<int, float> bias;
        
        for (const auto& cluster : clusters) {
            // Bias toward all members of the cluster
            for (size_t i = 0; i < cluster.member_nodes.size(); ++i) {
                int node = cluster.member_nodes[i];
                float weight = cluster.member_weights[i];
                
                // Increase bias for this token
                bias[node] += weight * cluster.centroid_activation;
                
                // Also bias toward neighbors (semantic spreading)
                auto it = adjacency.find(node);
                if (it != adjacency.end()) {
                    for (int edge_id : it->second) {
                        if (edge_id >= 0 && edge_id < (int)edges.size()) {
                            int neighbor = edges[edge_id].b;
                            float edge_weight = edges[edge_id].w;
                            
                            // Spread bias to neighbors (attenuated)
                            bias[neighbor] += weight * edge_weight * 0.5f;
                        }
                    }
                }
            }
        }
        
        // Normalize bias to [0, 1] range
        if (!bias.empty()) {
            float max_bias = 0.0f;
            for (const auto& [token, b] : bias) {
                max_bias = std::max(max_bias, b);
            }
            
            if (max_bias > 0.0f) {
                for (auto& [token, b] : bias) {
                    b /= max_bias;
                }
            }
        }
        
        return bias;
    }
    
    // ==================== CENTROID CACHING ====================
    
    // Cache clusters for multi-turn context
    void cache_clusters(const std::vector<SemanticCluster>& clusters) {
        cached_clusters_ = clusters;
    }
    
    // Get cached clusters (with decay)
    std::vector<SemanticCluster> get_cached_clusters() {
        // Decay cached clusters
        for (auto& cluster : cached_clusters_) {
            cluster.decay(decay_rate_);
        }
        
        // Remove very weak clusters
        cached_clusters_.erase(
            std::remove_if(cached_clusters_.begin(), cached_clusters_.end(),
                          [](const SemanticCluster& c) { return c.centroid_activation < 0.01f; }),
            cached_clusters_.end()
        );
        
        return cached_clusters_;
    }
    
    // Merge new clusters with cached ones
    std::vector<SemanticCluster> merge_with_cache(
        const std::vector<SemanticCluster>& new_clusters
    ) {
        auto cached = get_cached_clusters();
        
        std::vector<SemanticCluster> merged = new_clusters;
        
        // Add cached clusters that don't overlap too much
        for (const auto& cached_cluster : cached) {
            bool overlaps = false;
            
            for (const auto& new_cluster : new_clusters) {
                // Check if clusters share dominant nodes
                if (cached_cluster.dominant_node == new_cluster.dominant_node) {
                    overlaps = true;
                    break;
                }
            }
            
            if (!overlaps) {
                merged.push_back(cached_cluster);
            }
        }
        
        return merged;
    }
    
    // Clear cache
    void clear_cache() {
        cached_clusters_.clear();
    }
    
    // ==================== STATISTICS ====================
    
    void print_cluster_info(const std::vector<SemanticCluster>& clusters) const {
        std::cout << "🔮 Clusters: " << clusters.size() << "\n";
        for (size_t i = 0; i < clusters.size() && i < 3; ++i) {
            const auto& cluster = clusters[i];
            std::cout << "   [";
            
            for (size_t j = 0; j < cluster.member_nodes.size() && j < 3; ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << nodes[cluster.member_nodes[j]].data;
            }
            
            if (cluster.member_nodes.size() > 3) {
                std::cout << ", +" << (cluster.member_nodes.size() - 3) << " more";
            }
            
            std::cout << "]\n";
        }
    }
};

} // namespace melvin_prediction

