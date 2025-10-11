/*
 * MELVIN CONTINUOUS LEARNING + EVOLUTION PIPELINE
 * 
 * Memory consolidation system for replay, decay, and optimization
 * of Melvin's knowledge graph during idle periods.
 */

#pragma once

#include <vector>
#include <unordered_set>
#include <queue>
#include <random>
#include <algorithm>
#include <chrono>
#include "melvin_types.h"
#include "predictive_sampler.h"

namespace melvin {

// ==================== MEMORY CONSOLIDATION ====================

class MemoryConsolidator {
private:
    std::mt19937 rng_;
    std::vector<uint64_t> thought_nodes_;  // Queue of Thought nodes for replay
    std::chrono::system_clock::time_point last_consolidation_;
    std::chrono::minutes consolidation_interval_{30};  // Consolidate every 30 minutes
    
    // Consolidation parameters
    float decay_rate_ = 0.001f;        // How fast to decay unused edges
    float replay_probability_ = 0.1f;   // Probability of replaying a Thought node
    int max_replay_depth_ = 5;          // Maximum depth for replay traversals
    float clustering_threshold_ = 0.8f; // Similarity threshold for node clustering
    
public:
    MemoryConsolidator() : rng_(std::random_device{}()), 
                          last_consolidation_(std::chrono::system_clock::now()) {}
    
    // Main consolidation process (called during idle or scheduled intervals)
    void consolidate() {
        auto now = std::chrono::system_clock::now();
        if (now - last_consolidation_ < consolidation_interval_) {
            return;  // Not time for consolidation yet
        }
        
        std::cout << "🧠 Starting memory consolidation...\n";
        
        // 1. Replay Thought nodes
        replay_thought_nodes();
        
        // 2. Decay rarely used edges
        decay_unused_edges();
        
        // 3. Cluster redundant nodes
        cluster_redundant_nodes();
        
        // 4. Sample random traversals for coherence testing
        test_graph_coherence();
        
        last_consolidation_ = now;
        std::cout << "🧠 Memory consolidation complete\n";
    }
    
    // Add a Thought node for future replay
    void add_thought_node(uint64_t thought_id) {
        thought_nodes_.push_back(thought_id);
        
        // Keep only recent Thought nodes (limit memory)
        if (thought_nodes_.size() > 1000) {
            thought_nodes_.erase(thought_nodes_.begin(), 
                               thought_nodes_.begin() + 500);
        }
    }
    
    // Set consolidation parameters
    void set_decay_rate(float rate) { decay_rate_ = rate; }
    void set_replay_probability(float prob) { replay_probability_ = prob; }
    void set_consolidation_interval(std::chrono::minutes interval) { 
        consolidation_interval_ = interval; 
    }
    
private:
    // Replay successful Thought nodes to reinforce patterns
    void replay_thought_nodes() {
        if (thought_nodes_.empty()) return;
        
        std::cout << "   📚 Replaying " << thought_nodes_.size() << " Thought nodes...\n";
        
        int replayed = 0;
        int successful_replays = 0;
        
        for (uint64_t thought_id : thought_nodes_) {
            if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) < replay_probability_) {
                replayed++;
                
                // Get the Thought node
                if (G_nodes.find(thought_id) != G_nodes.end()) {
                    const auto& thought_node = G_nodes[thought_id];
                    
                    // Parse the thought pattern (simplified)
                    std::vector<uint64_t> path = parse_thought_path(thought_node.text);
                    
                    if (path.size() >= 2) {
                        // Replay the path with current parameters
                        bool success = replay_path(path);
                        if (success) successful_replays++;
                    }
                }
            }
        }
        
        std::cout << "   📚 Replayed " << replayed << " thoughts, " 
                  << successful_replays << " successful\n";
    }
    
    // Decay edges that haven't been used recently
    void decay_unused_edges() {
        std::cout << "   🗑️  Decaying unused edges...\n";
        
        int decayed_edges = 0;
        auto now = std::chrono::system_clock::now();
        
        for (auto& edge : G_edges) {
            // Simple decay based on frequency ratio
            if (edge.freq_ratio < decay_rate_) {
                edge.freq_ratio *= (1.0f - decay_rate_);
                decayed_edges++;
            }
        }
        
        std::cout << "   🗑️  Decayed " << decayed_edges << " edges\n";
    }
    
    // Cluster nodes that are too similar
    void cluster_redundant_nodes() {
        std::cout << "   🔗 Clustering redundant nodes...\n";
        
        std::vector<uint64_t> node_ids;
        for (const auto& pair : G_nodes) {
            node_ids.push_back(pair.first);
        }
        
        int clustered = 0;
        std::unordered_set<uint64_t> processed;
        
        for (size_t i = 0; i < node_ids.size(); ++i) {
            if (processed.find(node_ids[i]) != processed.end()) continue;
            
            std::vector<uint64_t> cluster = {node_ids[i]};
            
            // Find similar nodes
            for (size_t j = i + 1; j < node_ids.size(); ++j) {
                if (processed.find(node_ids[j]) != processed.end()) continue;
                
                if (nodes_similar(node_ids[i], node_ids[j])) {
                    cluster.push_back(node_ids[j]);
                    processed.insert(node_ids[j]);
                }
            }
            
            if (cluster.size() > 1) {
                merge_cluster(cluster);
                clustered += cluster.size() - 1;
                for (uint64_t node_id : cluster) {
                    processed.insert(node_id);
                }
            }
        }
        
        std::cout << "   🔗 Clustered " << clustered << " redundant nodes\n";
    }
    
    // Test graph coherence with random traversals
    void test_graph_coherence() {
        std::cout << "   🧪 Testing graph coherence...\n";
        
        int coherent_paths = 0;
        int total_tests = 100;
        
        for (int i = 0; i < total_tests; ++i) {
            // Pick random starting node
            if (G_nodes.empty()) continue;
            
            auto it = G_nodes.begin();
            std::advance(it, std::uniform_int_distribution<size_t>(0, G_nodes.size() - 1)(rng_));
            uint64_t start_node = it->first;
            
            // Try a short traversal
            std::vector<uint64_t> path = {start_node};
            uint64_t current = start_node;
            
            for (int depth = 0; depth < max_replay_depth_ && 
                 G_adj.find(current) != G_adj.end() && !G_adj[current].empty(); ++depth) {
                
                // Pick random edge
                const auto& edges = G_adj[current];
                size_t edge_idx = std::uniform_int_distribution<size_t>(0, edges.size() - 1)(rng_);
                
                if (edge_idx < G_edges.size()) {
                    uint64_t next_node = G_edges[edges[edge_idx]].locB;
                    
                    // Check for loops
                    if (std::find(path.begin(), path.end(), next_node) != path.end()) {
                        break;
                    }
                    
                    path.push_back(next_node);
                    current = next_node;
                } else {
                    break;
                }
            }
            
            if (path.size() > 1) {
                coherent_paths++;
            }
        }
        
        float coherence_rate = static_cast<float>(coherent_paths) / total_tests;
        std::cout << "   🧪 Graph coherence: " << std::fixed << std::setprecision(2) 
                  << (coherence_rate * 100.0f) << "%\n";
    }
    
    // Helper functions
    std::vector<uint64_t> parse_thought_path(const std::string& thought_text) {
        // Parse "1110 [node1→node2→node3] 1110" format
        std::vector<uint64_t> path;
        
        size_t start = thought_text.find('[');
        size_t end = thought_text.find(']');
        
        if (start == std::string::npos || end == std::string::npos) {
            return path;
        }
        
        std::string path_str = thought_text.substr(start + 1, end - start - 1);
        std::stringstream ss(path_str);
        std::string node_name;
        
        while (std::getline(ss, node_name, '→')) {
            // Find node by name (simplified - in practice would use proper lookup)
            for (const auto& pair : G_nodes) {
                if (pair.second.text == node_name) {
                    path.push_back(pair.first);
                    break;
                }
            }
        }
        
        return path;
    }
    
    bool replay_path(const std::vector<uint64_t>& path) {
        // Simplified replay - just check if path is still valid
        for (size_t i = 0; i < path.size() - 1; ++i) {
            uint64_t src = path[i];
            uint64_t dst = path[i + 1];
            
            bool edge_exists = false;
            if (G_adj.find(src) != G_adj.end()) {
                for (size_t edge_idx : G_adj[src]) {
                    if (edge_idx < G_edges.size() && G_edges[edge_idx].locB == dst) {
                        edge_exists = true;
                        break;
                    }
                }
            }
            
            if (!edge_exists) {
                return false;
            }
        }
        
        return true;
    }
    
    bool nodes_similar(uint64_t node1_id, uint64_t node2_id) {
        if (G_nodes.find(node1_id) == G_nodes.end() || 
            G_nodes.find(node2_id) == G_nodes.end()) {
            return false;
        }
        
        const auto& node1 = G_nodes[node1_id];
        const auto& node2 = G_nodes[node2_id];
        
        // Simple similarity based on text (in practice would use embeddings)
        if (node1.text == node2.text) return true;
        
        // Check if one is substring of the other
        if (node1.text.find(node2.text) != std::string::npos ||
            node2.text.find(node1.text) != std::string::npos) {
            return true;
        }
        
        return false;
    }
    
    void merge_cluster(const std::vector<uint64_t>& cluster) {
        if (cluster.size() < 2) return;
        
        uint64_t primary_node = cluster[0];
        
        // Merge edges from other nodes to primary
        for (size_t i = 1; i < cluster.size(); ++i) {
            uint64_t node_to_merge = cluster[i];
            
            // Redirect edges
            if (G_adj.find(node_to_merge) != G_adj.end()) {
                for (size_t edge_idx : G_adj[node_to_merge]) {
                    if (edge_idx < G_edges.size()) {
                        G_edges[edge_idx].locA = primary_node;
                    }
                }
                
                // Merge adjacency lists
                G_adj[primary_node].insert(G_adj[primary_node].end(),
                                         G_adj[node_to_merge].begin(),
                                         G_adj[node_to_merge].end());
                
                G_adj.erase(node_to_merge);
            }
            
            // Remove merged node
            G_nodes.erase(node_to_merge);
        }
    }
};

// ==================== CONTINUOUS LEARNING PIPELINE ====================

class ContinuousLearningPipeline {
private:
    MemoryConsolidator consolidator_;
    std::vector<std::string> data_buffer_;
    size_t max_buffer_size_ = 10000;
    
public:
    ContinuousLearningPipeline() = default;
    
    // Process incoming data stream
    void process_data_stream(const std::vector<std::string>& new_data) {
        std::cout << "📥 Processing " << new_data.size() << " data items...\n";
        
        for (const std::string& data : new_data) {
            // 1. Normalize input
            std::string normalized = normalize_input(data);
            
            // 2. Create/update nodes and edges
            process_normalized_data(normalized);
            
            // 3. Run reasoning to produce predictions
            std::vector<uint64_t> context = get_recent_context();
            if (!context.empty()) {
                PredictiveConfig cfg;
                std::vector<uint64_t> prediction = generate_path(context, cfg);
                
                // 4. Update graph based on prediction success
                reinforce_prediction_edges(context, prediction);
            }
            
            // 5. Add to buffer for batch processing
            data_buffer_.push_back(normalized);
            if (data_buffer_.size() > max_buffer_size_) {
                data_buffer_.erase(data_buffer_.begin(), 
                                 data_buffer_.begin() + max_buffer_size_ / 2);
            }
        }
        
        // Trigger consolidation if needed
        consolidator_.consolidate();
    }
    
    // Get memory consolidator for external control
    MemoryConsolidator& get_consolidator() {
        return consolidator_;
    }
    
private:
    std::string normalize_input(const std::string& input) {
        // Simple normalization - in practice would use text_norm
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        
        // Remove punctuation
        result.erase(std::remove_if(result.begin(), result.end(),
                                   [](char c) { return !std::isalnum(c) && c != ' '; }),
                    result.end());
        
        return result;
    }
    
    void process_normalized_data(const std::string& data) {
        // Simple word-based processing - in practice would be more sophisticated
        std::stringstream ss(data);
        std::string word;
        std::vector<uint64_t> word_nodes;
        
        while (ss >> word) {
            uint64_t node_id = find_or_create_node(word);
            word_nodes.push_back(node_id);
        }
        
        // Create temporal edges between consecutive words
        for (size_t i = 0; i < word_nodes.size() - 1; ++i) {
            create_or_reinforce_edge(word_nodes[i], word_nodes[i + 1], Rel::TEMPORAL);
        }
    }
    
    uint64_t find_or_create_node(const std::string& text) {
        // Find existing node
        for (const auto& pair : G_nodes) {
            if (pair.second.text == text) {
                return pair.first;
            }
        }
        
        // Create new node
        uint64_t node_id = G_total_nodes++;
        G_nodes[node_id] = Node();
        G_nodes[node_id].id = node_id;
        G_nodes[node_id].text = text;
        G_nodes[node_id].freq = 1;
        
        return node_id;
    }
    
    void create_or_reinforce_edge(uint64_t src, uint64_t dst, Rel rel) {
        // Check if edge exists
        if (G_adj.find(src) != G_adj.end()) {
            for (size_t edge_idx : G_adj[src]) {
                if (edge_idx < G_edges.size() && G_edges[edge_idx].locB == dst) {
                    // Reinforce existing edge
                    G_edges[edge_idx].count++;
                    G_edges[edge_idx].freq_ratio = std::min(1.0f, 
                        G_edges[edge_idx].freq_ratio + 0.01f);
                    return;
                }
            }
        }
        
        // Create new edge
        size_t edge_idx = G_edges.size();
        G_edges.emplace_back();
        auto& edge = G_edges.back();
        edge.locA = src;
        edge.locB = dst;
        edge.rel = static_cast<uint8_t>(rel);
        edge.freq_ratio = 0.1f;
        edge.count = 1;
        
        G_adj[src].push_back(edge_idx);
    }
    
    std::vector<uint64_t> get_recent_context() {
        // Return recent nodes as context (simplified)
        std::vector<uint64_t> context;
        
        auto it = G_nodes.rbegin();
        for (int i = 0; i < 5 && it != G_nodes.rend(); ++i, ++it) {
            context.push_back(it->first);
        }
        
        return context;
    }
    
    void reinforce_prediction_edges(const std::vector<uint64_t>& context, 
                                  const std::vector<uint64_t>& prediction) {
        // Reinforce edges that led to successful predictions
        for (size_t i = 0; i < prediction.size() - 1; ++i) {
            create_or_reinforce_edge(prediction[i], prediction[i + 1], Rel::TEMPORAL);
        }
    }
};

} // namespace melvin
