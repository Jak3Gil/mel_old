#include "melvin_leap_nodes.h"
#include "text_norm.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

// Structures from melvin.cpp (kept in sync)
struct Node {
    uint64_t id = 0;
    std::string text;
    uint32_t type = 0;
    int freq = 0;
    bool pinned = false;
    std::vector<float> emb; // Real embeddings for semantic similarity
    std::vector<float> embedding; // LLM-style semantic embedding
    float attention_weight = 0.0f; // Current attention weight
    uint64_t last_accessed = 0; // Timestamp for recency
    float semantic_strength = 1.0f; // Semantic coherence strength
    float activation = 0.0f;  // Current activation level
};

struct Edge {
    uint64_t u, v;          // Source and destination node IDs (note: stored as u, v)
    uint64_t locB;          // Destination (used in adjacency lists)
    float weight;           // Combined weight [0,1]
    float w_core;           // Core durable weight
    float w_ctx;            // Context weight
    uint32_t count;         // Usage count
    uint8_t rel;            // Relation type
    float last_used;        // Timestamp of last use
    float freq_ratio = 0.0f; // Frequency ratio for prediction
    
    Edge() : u(0), v(0), locB(0), weight(0.5f), w_core(0.3f), w_ctx(0.2f), count(1), rel(0), last_used(0.0f), freq_ratio(0.0f) {}
};

// External globals from melvin.cpp - these connect to the main knowledge graph
extern std::unordered_map<uint64_t, Node> G_nodes;
extern std::vector<Edge> G_edges;
extern std::unordered_map<uint64_t, std::vector<size_t>> G_adj;

namespace melvin {

// ==================== UTILITY FUNCTIONS ====================

float compute_entropy(const std::vector<Candidate>& candidates) {
    if (candidates.empty()) return 0.0f;
    
    float entropy = 0.0f;
    float sum = 0.0f;
    
    // Sum up all scores (assuming they're already in log space or we convert them)
    for (const auto& c : candidates) {
        sum += std::exp(c.score);
    }
    
    if (sum < 1e-9f) return 0.0f;
    
    // Compute entropy: -Σ p(x) log p(x)
    for (const auto& c : candidates) {
        float prob = std::exp(c.score) / sum;
        if (prob > 1e-9f) {
            entropy -= prob * std::log(prob);
        }
    }
    
    return entropy;
}

float compute_path_coherence(const std::vector<uint64_t>& path) {
    if (path.size() < 2) return 1.0f;
    
    float total_coherence = 0.0f;
    int valid_pairs = 0;
    
    for (size_t i = 0; i < path.size() - 1; ++i) {
        auto emb_a = get_node_embedding(path[i]);
        auto emb_b = get_node_embedding(path[i + 1]);
        
        if (!emb_a.empty() && !emb_b.empty()) {
            total_coherence += compute_similarity(emb_a, emb_b);
            valid_pairs++;
        }
    }
    
    return valid_pairs > 0 ? total_coherence / valid_pairs : 0.5f;
}

std::vector<float> get_node_embedding(uint64_t node_id) {
    auto it = G_nodes.find(node_id);
    if (it != G_nodes.end()) {
        return it->second.emb;
    }
    return std::vector<float>();
}

std::string get_node_text(uint64_t node_id) {
    auto it = G_nodes.find(node_id);
    if (it != G_nodes.end()) {
        return it->second.text;
    }
    return "";
}

float compute_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty()) return 0.0f;
    
    float dot_product = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    size_t min_size = std::min(a.size(), b.size());
    for (size_t i = 0; i < min_size; ++i) {
        dot_product += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    if (norm_a < 1e-9f || norm_b < 1e-9f) return 0.0f;
    
    return dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

// ==================== CLUSTER MANAGER ====================

ClusterManager::ClusterManager(const LeapConfig& config)
    : config_(config), last_update_time_(0) {
}

void ClusterManager::update_clusters(const std::vector<uint64_t>& active_nodes) {
    auto now = std::chrono::high_resolution_clock::now();
    last_update_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    detect_clusters(active_nodes);
}

std::vector<SemanticCluster> ClusterManager::get_active_clusters() const {
    return active_clusters_;
}

std::vector<SemanticCluster> ClusterManager::get_top_clusters(
    int n, const std::vector<uint64_t>& context) {
    
    std::vector<SemanticCluster> clusters = active_clusters_;
    
    // Score each cluster by activation * cohesion * novelty
    for (auto& cluster : clusters) {
        cluster.novelty = compute_cluster_novelty(cluster, context);
    }
    
    // Sort by combined score
    std::sort(clusters.begin(), clusters.end(), 
        [](const SemanticCluster& a, const SemanticCluster& b) {
            float score_a = a.activation * a.cohesion * (1.0f + a.novelty);
            float score_b = b.activation * b.cohesion * (1.0f + b.novelty);
            return score_a > score_b;
        });
    
    // Return top n
    if (n < (int)clusters.size()) {
        clusters.resize(n);
    }
    
    return clusters;
}

float ClusterManager::compute_cluster_cohesion(const std::vector<uint64_t>& members) {
    if (members.size() < 2) return 1.0f;
    
    float total_similarity = 0.0f;
    int pair_count = 0;
    
    // Compute average pairwise similarity
    for (size_t i = 0; i < members.size(); ++i) {
        for (size_t j = i + 1; j < members.size(); ++j) {
            float sim = get_cached_similarity(members[i], members[j]);
            total_similarity += sim;
            pair_count++;
        }
    }
    
    return pair_count > 0 ? total_similarity / pair_count : 0.0f;
}

float ClusterManager::compute_cluster_novelty(
    const SemanticCluster& cluster, 
    const std::vector<uint64_t>& context) {
    
    // Count how many cluster members appear in recent context
    std::unordered_set<uint64_t> context_set(context.begin(), context.end());
    
    int overlap_count = 0;
    for (uint64_t member : cluster.members) {
        if (context_set.count(member) > 0) {
            overlap_count++;
        }
    }
    
    // Novelty is high when overlap is low
    float overlap_ratio = cluster.members.empty() ? 0.0f : 
                         (float)overlap_count / cluster.members.size();
    return 1.0f - overlap_ratio;
}

std::vector<float> ClusterManager::compute_cluster_centroid(
    const std::vector<uint64_t>& members) {
    
    if (members.empty()) return std::vector<float>();
    
    // Determine embedding size from first member
    auto first_emb = get_node_embedding(members[0]);
    if (first_emb.empty()) return std::vector<float>();
    
    std::vector<float> centroid(first_emb.size(), 0.0f);
    int valid_count = 0;
    
    for (uint64_t member : members) {
        auto emb = get_node_embedding(member);
        if (!emb.empty() && emb.size() == centroid.size()) {
            for (size_t i = 0; i < centroid.size(); ++i) {
                centroid[i] += emb[i];
            }
            valid_count++;
        }
    }
    
    // Average
    if (valid_count > 0) {
        for (float& val : centroid) {
            val /= valid_count;
        }
    }
    
    return centroid;
}

void ClusterManager::clear_stale_clusters(uint64_t max_age_ns) {
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    auto it = std::remove_if(active_clusters_.begin(), active_clusters_.end(),
        [current_time, max_age_ns](const SemanticCluster& cluster) {
            return (current_time - cluster.last_used_time) > max_age_ns;
        });
    
    active_clusters_.erase(it, active_clusters_.end());
}

int ClusterManager::get_cluster_for_node(uint64_t node_id) const {
    auto it = node_to_cluster_.find(node_id);
    if (it != node_to_cluster_.end()) {
        return it->second;
    }
    return -1;
}

void ClusterManager::detect_clusters(const std::vector<uint64_t>& active_nodes) {
    // Find connected components based on edge connectivity and similarity
    auto components = find_connected_components(active_nodes, config_.min_cluster_cohesion);
    
    active_clusters_.clear();
    node_to_cluster_.clear();
    
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    for (size_t i = 0; i < components.size(); ++i) {
        const auto& component = components[i];
        
        if ((int)component.size() < config_.min_cluster_size) {
            continue;  // Too small to be a cluster
        }
        
        SemanticCluster cluster;
        cluster.members = component;
        cluster.cohesion = compute_cluster_cohesion(component);
        
        if (cluster.cohesion < config_.min_cluster_cohesion) {
            continue;  // Not cohesive enough
        }
        
        // Compute mean activation
        float total_activation = 0.0f;
        for (uint64_t node_id : component) {
            if (G_nodes.count(node_id) > 0) {
                total_activation += G_nodes[node_id].activation;
            }
        }
        cluster.activation = total_activation / component.size();
        
        // Compute centroid
        cluster.centroid = compute_cluster_centroid(component);
        
        // Find dominant concept (node with highest activation)
        uint64_t dominant_node = component[0];
        float max_activation = 0.0f;
        for (uint64_t node_id : component) {
            if (G_nodes.count(node_id) > 0) {
                if (G_nodes[node_id].activation > max_activation) {
                    max_activation = G_nodes[node_id].activation;
                    dominant_node = node_id;
                }
            }
        }
        cluster.dominant_concept = get_node_text(dominant_node);
        cluster.last_used_time = current_time;
        
        // Add to active clusters
        int cluster_idx = active_clusters_.size();
        active_clusters_.push_back(cluster);
        
        // Index all members
        for (uint64_t node_id : component) {
            node_to_cluster_[node_id] = cluster_idx;
        }
    }
}

std::vector<std::vector<uint64_t>> ClusterManager::find_connected_components(
    const std::vector<uint64_t>& nodes, 
    float min_similarity) {
    
    std::vector<std::vector<uint64_t>> components;
    std::unordered_set<uint64_t> visited;
    
    for (uint64_t node : nodes) {
        if (visited.count(node) > 0) continue;
        
        // BFS to find connected component
        std::vector<uint64_t> component;
        std::vector<uint64_t> queue = {node};
        visited.insert(node);
        
        while (!queue.empty()) {
            uint64_t current = queue.back();
            queue.pop_back();
            component.push_back(current);
            
            // Check all neighbors via edges
            if (G_adj.count(current) > 0) {
                for (size_t edge_idx : G_adj[current]) {
                    if (edge_idx >= G_edges.size()) continue;
                    
                    const auto& edge = G_edges[edge_idx];
                    uint64_t neighbor = edge.locB;
                    
                    // Only include if in our active set and not visited
                    if (std::find(nodes.begin(), nodes.end(), neighbor) != nodes.end() &&
                        visited.count(neighbor) == 0) {
                        
                        // Check if connection is strong enough
                        float sim = get_cached_similarity(current, neighbor);
                        if (sim >= min_similarity || edge.freq_ratio > 0.3f) {
                            visited.insert(neighbor);
                            queue.push_back(neighbor);
                        }
                    }
                }
            }
        }
        
        if (!component.empty()) {
            components.push_back(component);
        }
    }
    
    return components;
}

float ClusterManager::get_cached_similarity(uint64_t a, uint64_t b) {
    std::string key = make_cache_key(a, b);
    
    auto it = similarity_cache_.find(key);
    if (it != similarity_cache_.end()) {
        return it->second;
    }
    
    // Compute and cache
    auto emb_a = get_node_embedding(a);
    auto emb_b = get_node_embedding(b);
    float sim = compute_similarity(emb_a, emb_b);
    
    similarity_cache_[key] = sim;
    return sim;
}

std::string ClusterManager::make_cache_key(uint64_t a, uint64_t b) const {
    // Ensure consistent ordering
    if (a > b) std::swap(a, b);
    return std::to_string(a) + "_" + std::to_string(b);
}

// ==================== LEAP CONTROLLER ====================

LeapController::LeapController(const LeapConfig& config)
    : cluster_manager_(config), config_(config),
      total_leaps_created_(0), total_leaps_succeeded_(0),
      total_leaps_failed_(0), total_promotions_(0) {
}

bool LeapController::should_trigger_leap(
    const std::vector<Candidate>& candidates,
    const std::vector<uint64_t>& context) {
    
    if (!config_.enable_leap_nodes) return false;
    if (candidates.empty()) return false;
    
    // Check 1: Entropy threshold
    float entropy = compute_entropy(candidates);
    if (entropy > config_.leap_entropy_threshold) {
        return true;
    }
    
    // Check 2: Repetition detection
    int repetition_count = detect_repetition(context);
    if (repetition_count >= config_.repetition_threshold) {
        return true;
    }
    
    // Check 3: Low confidence (all candidates have low scores)
    float max_score = 0.0f;
    for (const auto& c : candidates) {
        max_score = std::max(max_score, c.score);
    }
    if (max_score < 0.3f) {  // Arbitrary low threshold
        return true;
    }
    
    return false;
}

LeapNode LeapController::create_leap_node(
    const std::vector<uint64_t>& context,
    const std::vector<Candidate>& candidates) {
    
    // Update clusters based on current context
    cluster_manager_.update_clusters(context);
    
    // Get top clusters ranked by activation × cohesion × novelty
    auto top_clusters = cluster_manager_.get_top_clusters(
        config_.max_clusters_per_leap, context);
    
    if (top_clusters.empty()) {
        // No valid clusters - return empty leap
        return LeapNode();
    }
    
    // Create leap node
    LeapNode leap;
    leap.creation_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Merge clusters
    std::vector<int> cluster_indices;
    for (size_t i = 0; i < top_clusters.size(); ++i) {
        const auto& cluster = top_clusters[i];
        cluster_indices.push_back(i);
        
        // Add all members
        leap.members.insert(leap.members.end(), 
                           cluster.members.begin(), 
                           cluster.members.end());
        
        // Accumulate activation and cohesion
        leap.activation += cluster.activation;
        leap.cohesion += cluster.cohesion;
    }
    
    // Average activation and cohesion
    if (!top_clusters.empty()) {
        leap.activation /= top_clusters.size();
        leap.cohesion /= top_clusters.size();
    }
    
    leap.source_clusters = cluster_indices;
    
    // Compute concept vector as centroid of all members
    leap.concept_vector = cluster_manager_.compute_cluster_centroid(leap.members);
    
    // Generate label
    leap.label = generate_leap_label(leap, total_leaps_created_);
    
    // Record initial entropy
    leap.entropy_before = compute_entropy(candidates);
    
    // Add to active leaps
    active_leaps_.push_back(leap);
    total_leaps_created_++;
    
    std::cout << "🧠 Created LeapNode: " << leap.label 
              << " (clusters=" << top_clusters.size() 
              << ", members=" << leap.members.size() 
              << ", cohesion=" << std::fixed << std::setprecision(2) << leap.cohesion 
              << ")\n";
    
    return leap;
}

LeapBiasField LeapController::compute_leap_bias(const LeapNode& leap_node) {
    LeapBiasField bias_field;
    
    if (leap_node.members.empty()) return bias_field;
    
    // Get all nodes connected to leap members
    std::unordered_set<uint64_t> connected_nodes;
    for (uint64_t member : leap_node.members) {
        if (G_adj.count(member) > 0) {
            for (size_t edge_idx : G_adj[member]) {
                if (edge_idx < G_edges.size()) {
                    connected_nodes.insert(G_edges[edge_idx].locB);
                }
            }
        }
    }
    
    // Compute bias for each connected node
    for (uint64_t node : connected_nodes) {
        auto node_emb = get_node_embedding(node);
        if (node_emb.empty()) continue;
        
        // Similarity to leap concept vector
        float similarity = compute_similarity(node_emb, leap_node.concept_vector);
        
        // Find max edge weight from any leap member to this node
        float max_edge_weight = 0.0f;
        for (uint64_t member : leap_node.members) {
            if (G_adj.count(member) > 0) {
                for (size_t edge_idx : G_adj[member]) {
                    if (edge_idx < G_edges.size()) {
                        const auto& edge = G_edges[edge_idx];
                        if (edge.locB == node) {
                            max_edge_weight = std::max(max_edge_weight, edge.freq_ratio);
                        }
                    }
                }
            }
        }
        
        // Compute bias: activation × cohesion × (similarity + edge_weight)
        float bias = leap_node.activation * leap_node.cohesion * 
                    (similarity + max_edge_weight);
        
        bias_field[node] = bias;
    }
    
    return bias_field;
}

void LeapController::apply_leap_bias_to_candidates(
    std::vector<Candidate>& candidates,
    const LeapBiasField& bias_field) {
    
    for (auto& candidate : candidates) {
        auto it = bias_field.find(candidate.node_id);
        if (it != bias_field.end()) {
            // Apply bias with lambda scaling
            candidate.score += config_.lambda_leap_bias * it->second;
        }
    }
}

void LeapController::record_leap_usage(
    LeapNode& leap_node, 
    float entropy_after,
    float coherence_gain) {
    
    leap_node.usage_count++;
    leap_node.entropy_after = entropy_after;
    leap_node.coherence_gain = coherence_gain;
    
    // Determine if successful
    float entropy_improvement = leap_node.entropy_before - entropy_after;
    
    bool successful = (entropy_improvement > config_.entropy_improvement_threshold) &&
                     (coherence_gain > config_.coherence_improvement_threshold);
    
    if (successful) {
        reinforce_success(leap_node);
    } else {
        penalize_failure(leap_node);
    }
}

void LeapController::reinforce_success(LeapNode& leap_node) {
    leap_node.success_count++;
    leap_node.success_score = (float)leap_node.success_count / 
                              std::max(1, leap_node.usage_count);
    
    total_leaps_succeeded_++;
    
    // Check for promotion
    if (leap_node.success_count >= config_.leap_promotion_threshold &&
        !leap_node.is_permanent) {
        promote_to_permanent(leap_node);
    }
}

void LeapController::penalize_failure(LeapNode& leap_node) {
    leap_node.failure_count++;
    leap_node.success_score = (float)leap_node.success_count / 
                              std::max(1, leap_node.usage_count);
    
    total_leaps_failed_++;
}

void LeapController::promote_to_permanent(LeapNode& leap_node) {
    if (leap_node.is_permanent) return;
    
    // Mark as permanent
    leap_node.is_permanent = true;
    
    // Move to permanent leaps
    permanent_leaps_.push_back(leap_node);
    
    // Create pattern signature for indexing
    std::string pattern = compute_leap_pattern_signature(leap_node.source_clusters);
    leap_pattern_index_[pattern] = permanent_leaps_.size() - 1;
    
    total_promotions_++;
    
    std::cout << "⭐ Promoted LeapNode to permanent: " << leap_node.label 
              << " (success_rate=" << std::fixed << std::setprecision(2) 
              << leap_node.success_score << ")\n";
    
    // TODO: Create actual node in knowledge graph with GENERALIZATION edges
    // This would involve calling into the storage layer to create a new node
    // and connecting it to the member clusters
}

void LeapController::cleanup_failed_leaps() {
    // Remove leaps that have failed too many times
    auto it = std::remove_if(active_leaps_.begin(), active_leaps_.end(),
        [](const LeapNode& leap) {
            return leap.failure_count > 3 && leap.success_score < 0.3f;
        });
    
    active_leaps_.erase(it, active_leaps_.end());
}

void LeapController::update_leap_novelty(const std::vector<uint64_t>& context) {
    // Update novelty scores for active leaps based on current context
    std::unordered_set<uint64_t> context_set(context.begin(), context.end());
    
    for (auto& leap : active_leaps_) {
        int overlap_count = 0;
        for (uint64_t member : leap.members) {
            if (context_set.count(member) > 0) {
                overlap_count++;
            }
        }
        // Could store novelty in leap metadata if needed
    }
}

LeapNode* LeapController::find_matching_leap_pattern(const std::vector<int>& cluster_ids) {
    std::string pattern = compute_leap_pattern_signature(cluster_ids);
    
    auto it = leap_pattern_index_.find(pattern);
    if (it != leap_pattern_index_.end() && it->second < (int)permanent_leaps_.size()) {
        return &permanent_leaps_[it->second];
    }
    
    return nullptr;
}

void LeapController::print_statistics() const {
    std::cout << "\n📊 Leap System Statistics:\n";
    std::cout << "  Total leaps created: " << total_leaps_created_ << "\n";
    std::cout << "  Successful leaps: " << total_leaps_succeeded_ << "\n";
    std::cout << "  Failed leaps: " << total_leaps_failed_ << "\n";
    std::cout << "  Promotions to permanent: " << total_promotions_ << "\n";
    std::cout << "  Active temporary leaps: " << active_leaps_.size() << "\n";
    std::cout << "  Permanent leap patterns: " << permanent_leaps_.size() << "\n";
    
    if (total_leaps_created_ > 0) {
        float success_rate = (float)total_leaps_succeeded_ / total_leaps_created_;
        std::cout << "  Overall success rate: " << std::fixed << std::setprecision(1)
                  << (success_rate * 100) << "%\n";
    }
}

float LeapController::compute_entropy(const std::vector<Candidate>& candidates) {
    return melvin::compute_entropy(candidates);
}

int LeapController::detect_repetition(const std::vector<uint64_t>& context) {
    if (context.size() < 2) return 0;
    
    // Look at recent context window
    int window_size = std::min((int)context.size(), 
                              config_.context_window_for_repetition);
    
    std::unordered_map<uint64_t, int> node_counts;
    for (int i = context.size() - window_size; i < (int)context.size(); ++i) {
        node_counts[context[i]]++;
    }
    
    // Find max repetition count
    int max_count = 0;
    for (const auto& pair : node_counts) {
        max_count = std::max(max_count, pair.second);
    }
    
    return max_count;
}

std::string LeapController::generate_leap_label(const LeapNode& leap_node, int leap_index) {
    std::ostringstream oss;
    
    // Find dominant concept from members
    std::string dominant = "concept";
    float max_activation = 0.0f;
    
    for (uint64_t member : leap_node.members) {
        if (G_nodes.count(member) > 0) {
            if (G_nodes[member].activation > max_activation) {
                max_activation = G_nodes[member].activation;
                dominant = get_node_text(member);
            }
        }
    }
    
    // Truncate if too long
    if (dominant.length() > 20) {
        dominant = dominant.substr(0, 20);
    }
    
    // Normalize to make it a valid identifier
    std::replace(dominant.begin(), dominant.end(), ' ', '_');
    
    oss << dominant << "_leap_" << leap_index;
    return oss.str();
}

std::string LeapController::compute_leap_pattern_signature(const std::vector<int>& cluster_ids) {
    std::ostringstream oss;
    std::vector<int> sorted_ids = cluster_ids;
    std::sort(sorted_ids.begin(), sorted_ids.end());
    
    for (size_t i = 0; i < sorted_ids.size(); ++i) {
        if (i > 0) oss << "_";
        oss << sorted_ids[i];
    }
    
    return oss.str();
}

float LeapController::compute_coherence_score(const std::vector<uint64_t>& generated_path) {
    return compute_path_coherence(generated_path);
}

std::vector<uint64_t> LeapController::get_connected_nodes(const LeapNode& leap_node) {
    std::unordered_set<uint64_t> connected;
    
    for (uint64_t member : leap_node.members) {
        if (G_adj.count(member) > 0) {
            for (size_t edge_idx : G_adj[member]) {
                if (edge_idx < G_edges.size()) {
                    connected.insert(G_edges[edge_idx].locB);
                }
            }
        }
    }
    
    return std::vector<uint64_t>(connected.begin(), connected.end());
}

// ==================== LEAP INTEGRATOR ====================

void LeapIntegrator::apply_leap_to_generation(
    std::vector<Candidate>& candidates,
    const std::vector<uint64_t>& context,
    LeapController& controller,
    bool& leap_triggered) {
    
    leap_triggered = false;
    
    // Check if leap should be triggered
    if (!controller.should_trigger_leap(candidates, context)) {
        return;
    }
    
    // Create leap node
    LeapNode leap = controller.create_leap_node(context, candidates);
    
    if (leap.members.empty()) {
        return;  // Failed to create leap
    }
    
    // Compute bias field
    LeapBiasField bias_field = controller.compute_leap_bias(leap);
    
    // Apply bias to candidates
    controller.apply_leap_bias_to_candidates(candidates, bias_field);
    
    leap_triggered = true;
}

void LeapIntegrator::record_generation_feedback(
    LeapController& controller,
    const std::vector<uint64_t>& generated_path,
    float initial_entropy) {
    
    // Find the most recent active leap
    const auto& active_leaps = controller.get_active_leaps();
    if (active_leaps.empty()) return;
    
    // We need to access the controller's internal vector to modify it
    // This is a limitation of the const reference - in production, we'd
    // add a method to controller for this
    (void)generated_path;  // Suppress unused warning for now
    (void)initial_entropy;
    
    // TODO: Add controller.record_last_leap_feedback() method
    // For now, feedback is recorded during apply_leap_to_generation
}

} // namespace melvin

