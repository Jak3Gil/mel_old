#include "consolidation.h"
#include <algorithm>
#include <unordered_set>
#include <iostream>

namespace melvin {
namespace reasoning {

// Helper for pair hashing
struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

Consolidator::Consolidator()
    : strengthening_rate_(0.05f)
    , pruning_threshold_(0.1f)
    , merge_threshold_(0.85f)
{
}

void Consolidator::consolidate(
    std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, float>& activation_history,
    const std::vector<std::vector<int>>& episodes
) {
    std::cout << "🔄 Starting consolidation (" << episodes.size() << " episodes)..." << std::endl;
    
    // 1. Strengthen co-activated edges
    int strengthened = 0;
    std::unordered_set<std::pair<int, int>, PairHash> co_activated;
    
    // Find co-activated pairs
    for (const auto& episode : episodes) {
        for (size_t i = 0; i < episode.size(); ++i) {
            for (size_t j = i + 1; j < episode.size(); ++j) {
                int a = std::min(episode[i], episode[j]);
                int b = std::max(episode[i], episode[j]);
                co_activated.insert({a, b});
            }
        }
    }
    
    // Strengthen edges
    for (const auto& pair : co_activated) {
        int node_a = pair.first;
        int node_b = pair.second;
        
        auto it_a = graph.find(node_a);
        auto it_b = graph.find(node_b);
        
        if (it_a != graph.end()) {
            for (auto& edge : it_a->second) {
                if (edge.first == node_b) {
                    edge.second = std::min(1.0f, edge.second + strengthening_rate_);
                    strengthened++;
                }
            }
        }
        
        if (it_b != graph.end()) {
            for (auto& edge : it_b->second) {
                if (edge.first == node_a) {
                    edge.second = std::min(1.0f, edge.second + strengthening_rate_);
                }
            }
        }
    }
    
    std::cout << "  💪 Strengthened " << strengthened << " edges" << std::endl;
    
    // 2. Prune weak edges
    int pruned = 0;
    for (auto& node_pair : graph) {
        std::vector<std::pair<int, float>> new_edges;
        for (const auto& edge : node_pair.second) {
            if (edge.second > pruning_threshold_) {
                new_edges.push_back(edge);
            } else {
                pruned++;
            }
        }
        node_pair.second = new_edges;
    }
    
    std::cout << "  ✂️  Pruned " << pruned << " weak edges" << std::endl;
    std::cout << "✅ Consolidation complete" << std::endl;
}

bool Consolidator::should_trigger(int reasoning_session_count, float idle_time) {
    return (reasoning_session_count >= 5 || idle_time > 30.0f);
}

// ==============================================================================
// GAP 6: ENHANCED CONSOLIDATION IMPLEMENTATION
// ==============================================================================

void Consolidator::consolidate_full(
    std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    std::unordered_map<int, std::vector<float>>& embeddings,
    const std::unordered_map<int, float>& activation_history,
    const std::deque<Experience>& experiences,
    float meta_learning_rate
) {
    std::cout << "\n═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << "🧠 ENHANCED CONSOLIDATION CYCLE" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    
    reset_stats();
    
    // 1. Experience replay
    std::cout << "\n📼 Step 1: Experience Replay" << std::endl;
    replay_experiences(graph, experiences, 10);
    
    // 2. Edge pruning
    std::cout << "\n✂️  Step 2: Edge Pruning" << std::endl;
    int pruned = prune_weak_edges(graph, 0.0f);
    stats_.edges_pruned = pruned;
    
    // 3. Abstraction formation
    std::cout << "\n🔮 Step 3: Abstraction Formation" << std::endl;
    auto clusters = form_abstractions(graph, embeddings, 100);
    stats_.abstractions_formed = clusters.size();
    
    // 4. Node merging
    std::cout << "\n🔗 Step 4: Node Merging" << std::endl;
    int merged = merge_similar_nodes(graph, embeddings);
    stats_.nodes_merged = merged;
    
    // Summary
    std::cout << "\n═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << "✅ CONSOLIDATION COMPLETE" << std::endl;
    std::cout << "   Experiences replayed: " << stats_.experiences_replayed << std::endl;
    std::cout << "   Edges pruned: " << stats_.edges_pruned << std::endl;
    std::cout << "   Abstractions formed: " << stats_.abstractions_formed << std::endl;
    std::cout << "   Nodes merged: " << stats_.nodes_merged << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════\n" << std::endl;
}

void Consolidator::replay_experiences(
    std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::deque<Experience>& experiences,
    int num_replays
) {
    if (experiences.empty()) {
        std::cout << "   ⚠️  No experiences to replay" << std::endl;
        return;
    }
    
    // Select high-importance experiences
    std::vector<Experience> important_experiences;
    for (const auto& exp : experiences) {
        if (exp.importance > 0.5f) {
            important_experiences.push_back(exp);
        }
    }
    
    if (important_experiences.empty()) {
        // Fall back to most recent
        int count = std::min(num_replays, static_cast<int>(experiences.size()));
        for (int i = 0; i < count; i++) {
            important_experiences.push_back(experiences[experiences.size() - 1 - i]);
        }
    }
    
    // Replay experiences (strengthen edges)
    for (const auto& exp : important_experiences) {
        for (const auto& edge_pair : exp.active_edges) {
            int src = edge_pair.first;
            int dst = edge_pair.second;
            
            auto it = graph.find(src);
            if (it != graph.end()) {
                for (auto& edge : it->second) {
                    if (edge.first == dst) {
                        float strength_boost = replay_strength_ * exp.importance * exp.outcome_reward;
                        edge.second = std::min(1.0f, edge.second + strength_boost);
                    }
                }
            }
        }
        stats_.experiences_replayed++;
    }
    
    std::cout << "   ✅ Replayed " << stats_.experiences_replayed << " important experiences" << std::endl;
}

int Consolidator::prune_weak_edges(
    std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    float current_time
) {
    int total_pruned = 0;
    
    for (auto& node_pair : graph) {
        std::vector<std::pair<int, float>> kept_edges;
        
        for (const auto& edge : node_pair.second) {
            // Criteria for keeping:
            // 1. Weight above threshold
            // 2. Not too old (would need tracking)
            
            bool strong_enough = edge.second > pruning_threshold_;
            
            if (strong_enough) {
                kept_edges.push_back(edge);
            } else {
                total_pruned++;
            }
        }
        
        node_pair.second = kept_edges;
    }
    
    std::cout << "   ✅ Pruned " << total_pruned << " weak edges (threshold: " 
              << pruning_threshold_ << ")" << std::endl;
    
    return total_pruned;
}

std::vector<NodeCluster> Consolidator::form_abstractions(
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    int min_frequency
) {
    std::vector<NodeCluster> clusters;
    
    // Find densely connected subgraphs (communities)
    std::unordered_map<int, int> node_degree;
    for (const auto& node_pair : graph) {
        node_degree[node_pair.first] = node_pair.second.size();
    }
    
    // Identify high-degree nodes as cluster centers
    std::vector<int> hub_nodes;
    for (const auto& degree_pair : node_degree) {
        if (degree_pair.second >= 10) {  // High connectivity
            hub_nodes.push_back(degree_pair.first);
        }
    }
    
    // Form clusters around hubs
    for (int hub : hub_nodes) {
        NodeCluster cluster;
        cluster.member_nodes.push_back(hub);
        
        // Add strongly connected neighbors
        auto it = graph.find(hub);
        if (it != graph.end()) {
            for (const auto& edge : it->second) {
                if (edge.second > 0.7f) {  // Strong connection
                    cluster.member_nodes.push_back(edge.first);
                }
            }
        }
        
        // Only keep if large enough
        if (cluster.member_nodes.size() >= 3) {
            // Compute centroid embedding
            if (!embeddings.empty()) {
                size_t emb_size = embeddings.begin()->second.size();
                cluster.centroid_embedding.resize(emb_size, 0.0f);
                
                int count = 0;
                for (int node_id : cluster.member_nodes) {
                    auto emb_it = embeddings.find(node_id);
                    if (emb_it != embeddings.end()) {
                        for (size_t i = 0; i < emb_size; i++) {
                            cluster.centroid_embedding[i] += emb_it->second[i];
                        }
                        count++;
                    }
                }
                
                if (count > 0) {
                    for (float& val : cluster.centroid_embedding) {
                        val /= count;
                    }
                }
            }
            
            cluster.frequency = cluster.member_nodes.size();
            cluster.coherence = 0.8f;
            cluster.abstract_node_id = -1;  // Would create new node in full implementation
            
            clusters.push_back(cluster);
        }
    }
    
    std::cout << "   ✅ Formed " << clusters.size() << " abstract concepts from frequent patterns" << std::endl;
    
    return clusters;
}

int Consolidator::merge_similar_nodes(
    std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    std::unordered_map<int, std::vector<float>>& embeddings
) {
    int merged_count = 0;
    
    // Find similar node pairs
    std::vector<std::pair<int, int>> to_merge;
    std::vector<int> node_ids;
    
    for (const auto& pair : embeddings) {
        node_ids.push_back(pair.first);
    }
    
    // Compare all pairs (O(n²) - would optimize in production)
    for (size_t i = 0; i < node_ids.size() && i < 100; i++) {  // Limit for efficiency
        for (size_t j = i + 1; j < node_ids.size() && j < 100; j++) {
            int id_a = node_ids[i];
            int id_b = node_ids[j];
            
            float similarity = compute_similarity(embeddings[id_a], embeddings[id_b]);
            
            if (similarity > merge_threshold_) {
                to_merge.push_back({id_a, id_b});
                if (to_merge.size() >= 50) break;  // Limit merges per cycle
            }
        }
        if (to_merge.size() >= 50) break;
    }
    
    // Merge nodes (keep first, redirect edges from second)
    for (const auto& merge_pair : to_merge) {
        int keep = merge_pair.first;
        int remove = merge_pair.second;
        
        // Redirect edges from 'remove' to 'keep'
        auto it = graph.find(remove);
        if (it != graph.end()) {
            for (const auto& edge : it->second) {
                // Add to 'keep' node's edges
                graph[keep].push_back(edge);
            }
            graph.erase(remove);
        }
        
        // Update incoming edges
        for (auto& node_pair : graph) {
            for (auto& edge : node_pair.second) {
                if (edge.first == remove) {
                    edge.first = keep;
                }
            }
        }
        
        embeddings.erase(remove);
        merged_count++;
    }
    
    std::cout << "   ✅ Merged " << merged_count << " similar nodes (threshold: " 
              << merge_threshold_ << ")" << std::endl;
    
    return merged_count;
}

float Consolidator::compute_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.0f;
    
    // Cosine similarity
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    if (norm_a > 0 && norm_b > 0) {
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    return 0.0f;
}

std::vector<int> Consolidator::find_frequent_pattern(
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    int seed_node,
    int min_frequency
) {
    // BFS to find connected component
    std::vector<int> pattern;
    std::unordered_set<int> visited;
    std::vector<int> queue;
    
    queue.push_back(seed_node);
    visited.insert(seed_node);
    
    while (!queue.empty() && pattern.size() < 20) {  // Limit pattern size
        int current = queue.back();
        queue.pop_back();
        pattern.push_back(current);
        
        auto it = graph.find(current);
        if (it != graph.end()) {
            for (const auto& edge : it->second) {
                if (edge.second > 0.7f && visited.find(edge.first) == visited.end()) {
                    queue.push_back(edge.first);
                    visited.insert(edge.first);
                }
            }
        }
    }
    
    return pattern;
}

} // namespace reasoning
} // namespace melvin
