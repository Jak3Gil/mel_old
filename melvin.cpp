#include "melvin_types.h"
#include "storage.h"
#include "vm.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <iomanip>
#include <fstream>
#include <map>
#include <filesystem>
#include <future>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <queue>
#include <functional>

namespace melvin {

// Pattern miner for generalizations and abstractions
class PatternMiner {
private:
    Store* store_;
    
    // Pattern support tracking
    struct PatternSupport {
        std::vector<melvin::NodeID> instances;
        uint32_t count;
        float variance;
        bool is_stable;
        
        PatternSupport() : count(0), variance(0.0f), is_stable(false) {}
    };
    
    // Triplet pattern tracking: (superclass?, relation, object)
    std::unordered_map<std::string, PatternSupport> triplet_patterns_;
    
    // Configuration
    uint32_t k_support_ = 6;        // Minimum support count
    float variance_threshold_ = 0.1f; // Maximum variance for stability
    uint32_t delta_t_ = 2;           // Stability window (consolidation cycles)
    
public:
    PatternMiner(Store* store) : store_(store) {}
    
    // Observe patterns during daytime ingestion
    void observe(const std::vector<EdgeRec>& edges) {
        for (const auto& edge : edges) {
            std::string pattern_key = extract_triplet_pattern(edge);
            if (!pattern_key.empty()) {
                PatternSupport& support = triplet_patterns_[pattern_key];
                
                // Add instance
                melvin::NodeID src, dst;
                std::copy(edge.src, edge.src + 32, src.begin());
                std::copy(edge.dst, edge.dst + 32, dst.begin());
                
                support.instances.push_back(src);
                support.instances.push_back(dst);
                support.count++;
                
                // Calculate variance
                support.variance = calculate_variance(support.instances);
                
                // Check stability
                support.is_stable = is_pattern_stable(support);
            }
        }
    }
    
    // Crystallize generalizations during nightly consolidation
    void crystallize() {
        for (auto& [pattern_key, support] : triplet_patterns_) {
            if (should_promote(support, 0.8f)) { // Default high path score
                NodeID abstraction_id = create_abstraction(pattern_key, support);
                
                // Mark pattern as crystallized
                support.is_stable = true;
                
                // Create generalized edges
                create_generalized_edges(pattern_key, support, abstraction_id);
            }
        }
    }
    
    // Check if a pattern should be promoted to CORE
    bool should_promote(const PatternSupport& support, float path_score) const {
        return support.count >= k_support_ && 
               support.variance <= variance_threshold_ &&
               support.is_stable &&
               path_score >= 0.8f; // tau_high equivalent
    }
    
    // Create abstraction node from pattern
    NodeID create_abstraction(const std::string& pattern_key, const PatternSupport& support) {
        // Create abstraction node
        NodeRecHeader header;
        header.type = static_cast<uint32_t>(melvin::NodeType::ABSTRACTION);
        header.flags = static_cast<uint32_t>(NodeFlags::DURABLE | NodeFlags::GENERALIZED);
        header.ts_created = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        header.ts_updated = header.ts_created;
        
        // Create payload with pattern information
        std::stringstream payload_stream;
        payload_stream << "ABSTRACTION:" << pattern_key << ":SUPPORT:" << support.count;
        std::string payload_str = payload_stream.str();
        
        std::vector<uint8_t> payload(payload_str.begin(), payload_str.end());
        header.payload_len = static_cast<uint32_t>(payload.size());
        
        return store_->upsert_node(header, payload);
    }
    
private:
    // Extract triplet patterns from edges
    std::string extract_triplet_pattern(const EdgeRec& edge) {
        // Extract pattern: (superclass?, relation, object)
        melvin::Rel relation = static_cast<Rel>(edge.rel);
        
        // Get source and destination node content
        NodeRecHeader src_header, dst_header;
        std::vector<uint8_t> src_payload, dst_payload;
        
        NodeID src, dst;
        std::copy(edge.src, edge.src + 32, src.begin());
        std::copy(edge.dst, edge.dst + 32, dst.begin());
        
        if (!store_->get_node(src, src_header, src_payload) ||
            !store_->get_node(dst, dst_header, dst_payload)) {
            return "";
        }
        
        std::string src_content(src_payload.begin(), src_payload.end());
        std::string dst_content(dst_payload.begin(), dst_payload.end());
        
        // Create pattern key based on relation type
        std::stringstream pattern_stream;
        
        // Use 2-bit relation codes instead of hardcoded types
        uint8_t rel_code = static_cast<uint8_t>(relation) & 0x03;
        switch (rel_code) {
            case 0x00: // Exact
                pattern_stream << "EXACT:" << dst_content;
                break;
            case 0x01: // Temporal
                pattern_stream << "TEMPORAL:" << dst_content;
                break;
            case 0x02: // Leap
                pattern_stream << "LEAP:" << dst_content;
                break;
            default: // Reserved
                pattern_stream << "REL:" << static_cast<uint32_t>(relation) << ":" << dst_content;
                break;
        }
        
        return pattern_stream.str();
    }
    
    // Calculate pattern variance
    float calculate_variance(const std::vector<melvin::NodeID>& instances) {
        if (instances.size() < 2) return 0.0f;
        
        // Simple variance calculation based on node content similarity
        float variance = 0.0f;
        size_t comparisons = 0;
        
        for (size_t i = 0; i < instances.size(); ++i) {
            for (size_t j = i + 1; j < instances.size(); ++j) {
                // Compare node content (simplified)
                melvin::NodeRecHeader header1, header2;
                std::vector<uint8_t> payload1, payload2;
                
                if (store_->get_node(instances[i], header1, payload1) &&
                    store_->get_node(instances[j], header2, payload2)) {
                    
                    std::string content1(payload1.begin(), payload1.end());
                    std::string content2(payload2.begin(), payload2.end());
                    
                    // Simple string similarity (0 = identical, 1 = completely different)
                    float similarity = 0.0f;
                    size_t min_len = std::min(content1.length(), content2.length());
                    size_t max_len = std::max(content1.length(), content2.length());
                    
                    if (max_len > 0) {
                        size_t matches = 0;
                        for (size_t k = 0; k < min_len; ++k) {
                            if (content1[k] == content2[k]) matches++;
                        }
                        similarity = 1.0f - (static_cast<float>(matches) / max_len);
                    }
                    
                    variance += similarity;
                    comparisons++;
                }
            }
        }
        
        return comparisons > 0 ? variance / comparisons : 0.0f;
    }
    
    // Check pattern stability across consolidation windows
    bool is_pattern_stable(const PatternSupport& support) {
        // Check if pattern has been seen across multiple consolidation windows
        return support.count >= delta_t_ * 2; // Seen in at least 2 windows
    }
    
    void create_generalized_edges(const std::string& /* pattern_key */, 
                                 const PatternSupport& support, 
                                 const NodeID& abstraction_id) {
        // Create generalized edges from abstraction to instances
        for (const auto& instance_id : support.instances) {
            EdgeRec edge;
            std::copy(abstraction_id.begin(), abstraction_id.end(), edge.src);
            edge.rel = 0x00; // Exact relation (abstraction is exactly instance)
            std::copy(instance_id.begin(), instance_id.end(), edge.dst);
            edge.layer = 2; // generalized layer
            edge.w = 0.9f; // High weight for abstractions
            edge.w_core = 0.8f; // Durable
            edge.w_ctx = 0.1f; // Low context
            edge.count = support.count;
            edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            edge.flags = static_cast<uint32_t>(EdgeFlags::EDGE_GENERALIZED);
            edge.pad = 0;
            
            store_->upsert_edge(edge);
        }
    }
};

// Nightly consolidation system
class ConsolidationSystem {
private:
    Store* store_;
    std::unique_ptr<PatternMiner> pattern_miner_;
    
    // Configuration
    float beta_ctx_ = 0.10f;    // CTX decay rate
    float beta_core_ = 0.01f;   // CORE decay rate
    float tau_mid_ = 0.01f;    // Hypothesis threshold (very low for best guessing)
    float tau_high_ = 0.05f;   // Durable threshold (very low for best guessing)
    float alpha_core_ = 0.1f;  // CORE promotion increment
    float epsilon_ = 0.02f;    // GC threshold
    uint32_t T_sleeps_ = 3;    // GC age threshold
    
public:
    // Statistics
    struct ConsolidationStats {
        size_t generalizations_formed = 0;
        size_t generalizations_used = 0;
        size_t contradictions_seen = 0;
        size_t nodes_pruned = 0;
        size_t edges_pruned = 0;
        uint64_t consolidation_time_us = 0;
    };
    
    ConsolidationStats stats_;
    
public:
    ConsolidationSystem(Store* store) 
        : store_(store), pattern_miner_(std::make_unique<PatternMiner>(store)) {}
    
    // Run nightly consolidation pass
    void run_consolidation() {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Reset stats
        stats_ = ConsolidationStats{};
        
        // 1. Decay CTX, spare CORE
        decay_pass();
        
        // 2. Pattern mining and generalization
        mine_patterns();
        
        // 3. Promote patterns to CORE
        promote_patterns();
        
        // 4. Handle contradictions
        handle_contradictions();
        
        // 5. Garbage collection
        gc_sweep();
        
        // 6. Compaction and snapshots
        compact_and_snapshot();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        stats_.consolidation_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time).count();
    }
    
    // Decay pass with CTX/CORE differentiation
    void decay_pass() {
        // Apply different decay rates to CTX vs CORE
        store_->decay_pass(beta_ctx_, beta_core_);
    }
    
    // Pattern mining and generalization
    void mine_patterns() {
        // Get all edges for pattern mining
        // In a real implementation, this would be more efficient
        std::vector<EdgeRec> all_edges;
        
        // This is a simplified approach - in reality would iterate through store
        // For now, we'll observe patterns during the consolidation pass
        
        pattern_miner_->crystallize();
        stats_.generalizations_formed++;
    }
    
    // Garbage collection for trivia
    void gc_sweep() {
        // GC any node/edge that is:
        // degree <= 1 AND w_ctx < ε AND age > T
        
        // uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
        //     std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        // This would iterate through all edges and nodes
        // For now, just call the existing compact method
        store_->compact();
    }
    
    // Compaction and snapshot management
    void compact_and_snapshot() {
        // Compact segments, verify checksums/Merkle, keep last N_snapshots
        store_->compact();
        store_->verify_checksums();
    }
    
    // Get consolidation statistics
    const ConsolidationStats& get_stats() const { return stats_; }
    
    // Configuration
    void set_decay_params(float beta_ctx, float beta_core) {
        beta_ctx_ = beta_ctx;
        beta_core_ = beta_core;
    }
    
    void set_thresholds(float tau_mid, float tau_high) {
        tau_mid_ = tau_mid;
        tau_high_ = tau_high;
    }
    
    void set_gc_params(float epsilon, uint32_t T_sleeps) {
        epsilon_ = epsilon;
        T_sleeps_ = T_sleeps;
    }
    
private:
    // Promote patterns to CORE based on support and stability
    void promote_patterns() {
        // Promote patterns that meet criteria to CORE
        // This would iterate through pattern miner results and promote qualifying patterns
    }
    
    // Handle contradictions by keeping both claims
    void handle_contradictions() {
        // Keep both claims, add CONTRADICTS edges
        // Prefer higher w_core at answer time
        // Tag [Uncertain] when close
    }
    
    // Calculate node/edge age in consolidation cycles
    uint32_t calculate_age(uint64_t timestamp) const {
        uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        // Convert to consolidation cycles (simplified)
        uint64_t age_ns = now - timestamp;
        return static_cast<uint32_t>(age_ns / (24ULL * 60 * 60 * 1000000000ULL)); // Days
    }
    
    // Check if node/edge should be garbage collected
    bool should_gc(const EdgeRec& edge, uint32_t age) const {
        return edge.w_ctx < epsilon_ && age > T_sleeps_;
    }
    
    bool should_gc(const NodeRecHeader& header, uint32_t age) const {
        return age > T_sleeps_ && header.degree_hint <= 1;
    }
};

// Health metrics and monitoring
class HealthMetrics {
private:
    Store* store_;
    
    struct Metrics {
        size_t live_set_size = 0;
        size_t churn_rate = 0;
        uint64_t compaction_time_us = 0;
        size_t snapshot_delta = 0;
        size_t generalizations_formed = 0;
        size_t generalizations_used = 0;
        size_t hop_2_success = 0;
        size_t hop_3_success = 0;
        float uncertainty_rate = 0.0f;
        size_t repetition_incidents = 0;
        
        // Path scoring metrics
        std::vector<float> path_scores;
        std::vector<float> confidence_scores;
        std::vector<bool> used_generalization;
        std::vector<bool> contradictions_seen;
    };
    
    Metrics current_metrics_;
    std::vector<Metrics> historical_metrics_;
    
public:
    HealthMetrics(Store* store) : store_(store) {}
    
    // Record metrics during operation
    void record_path_score(float score, float confidence, bool used_gen, bool contradiction) {
        current_metrics_.path_scores.push_back(score);
        current_metrics_.confidence_scores.push_back(confidence);
        current_metrics_.used_generalization.push_back(used_gen);
        current_metrics_.contradictions_seen.push_back(contradiction);
        
        // Note: contradictions_seen is a vector<bool>, not a counter
    }
    
    void record_hop_success(size_t hop_count) {
        if (hop_count == 2) {
            current_metrics_.hop_2_success++;
        } else if (hop_count == 3) {
            current_metrics_.hop_3_success++;
        }
    }
    
    void record_repetition() {
        current_metrics_.repetition_incidents++;
    }
    
    void record_uncertainty() {
        // Calculate uncertainty rate
        if (!current_metrics_.confidence_scores.empty()) {
            size_t uncertain_count = 0;
            for (float conf : current_metrics_.confidence_scores) {
                if (conf < 0.5f) uncertain_count++;
            }
            current_metrics_.uncertainty_rate = static_cast<float>(uncertain_count) / 
                                              current_metrics_.confidence_scores.size();
        }
    }
    
    // Update metrics at consolidation time
    void update_consolidation_metrics(const ConsolidationSystem::ConsolidationStats& stats) {
        current_metrics_.generalizations_formed = stats.generalizations_formed;
        current_metrics_.generalizations_used = stats.generalizations_used;
        // Note: contradictions_seen is a vector<bool>, not a counter
        current_metrics_.compaction_time_us = stats.consolidation_time_us;
    }
    
    // Export metrics to CSV
    void export_to_csv(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        
        // CSV header
        file << "timestamp,path_score,confidence,used_generalization,contradiction\n";
        
        // Export current metrics
        for (size_t i = 0; i < current_metrics_.path_scores.size(); ++i) {
            file << std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count() << ",";
            file << current_metrics_.path_scores[i] << ",";
            file << current_metrics_.confidence_scores[i] << ",";
            file << (current_metrics_.used_generalization[i] ? "true" : "false") << ",";
            file << (current_metrics_.contradictions_seen[i] ? "true" : "false") << "\n";
        }
    }
    
    // Get current metrics
    const Metrics& get_current_metrics() const { return current_metrics_; }
    
    // Calculate health score
    float calculate_health_score() const {
        float score = 1.0f;
        
        // Penalize high uncertainty
        score -= current_metrics_.uncertainty_rate * 0.3f;
        
        // Penalize repetition incidents
        score -= std::min(1.0f, static_cast<float>(current_metrics_.repetition_incidents) / 10.0f) * 0.2f;
        
        // Reward successful hops
        float hop_success_rate = static_cast<float>(current_metrics_.hop_2_success + current_metrics_.hop_3_success) / 
                               std::max(1.0f, static_cast<float>(current_metrics_.path_scores.size()));
        score += hop_success_rate * 0.3f;
        
        // Reward generalizations
        if (current_metrics_.generalizations_formed > 0) {
            score += 0.2f;
        }
        
        return std::clamp(score, 0.0f, 1.0f);
    }
    
private:
    void update_live_set_size() {
        current_metrics_.live_set_size = store_->node_count() + store_->edge_count();
    }
    
    void calculate_churn_rate() {
        if (historical_metrics_.size() > 1) {
            const auto& prev = historical_metrics_.back();
            current_metrics_.churn_rate = current_metrics_.live_set_size - prev.live_set_size;
        }
    }
};

// Path structure for beam search
struct PathCandidate {
    std::vector<EdgeRec> edges;
    float score;
    float confidence;
    bool is_durable;
    
    PathCandidate() : score(0.0f), confidence(0.0f), is_durable(false) {}
    
    bool operator<(const PathCandidate& other) const {
        return score < other.score; // For max-heap
    }
};

// Blended Reasoning Engine with beam search
class BlendedReasoningEngine {
private:
    Store* store_;
    Drivers drivers_;
    float driver_lambda_curiosity_;
    float tau_mid_;
    float tau_high_;
    std::mt19937_64 rng_;
    
    // Anti-repeat state
    std::unordered_set<std::string> recent_bigrams_;
    std::unordered_set<std::string> recent_trigrams_;
    std::unordered_map<std::string, std::string> variation_buffer_;
    
    // Query context for response generation
    std::string current_query_context_;
    
    // Data structures for dynamic learning
    struct CooccurrencePattern {
        NodeID node1;
        NodeID node2;
        float strength;
        std::string context;
        uint64_t timestamp;
    };
    
    struct LearnedPattern {
        NodeID node1;
        NodeID node2;
        melvin::Rel relation;
        float confidence;
        std::string pattern;
        uint64_t timestamp;
    };
    
    std::vector<CooccurrencePattern> cooccurrence_patterns_;
    std::vector<LearnedPattern> learned_relationship_patterns_;
    
    // Helper functions for dynamic learning
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
public:
    BlendedReasoningEngine(Store* store, const Drivers& drivers, float tau_mid, float tau_high, uint64_t seed)
        : store_(store), drivers_(drivers), tau_mid_(tau_mid), tau_high_(tau_high), rng_(seed) {
        driver_lambda_curiosity_ = drivers.curiosity;
    }
    
    // Path scoring implementation
    float score_path(const std::vector<EdgeRec>& edges) {
        if (edges.empty()) return 0.0f;
        
        float product = 1.0f;
        for (const auto& e : edges) {
            // Mix durable vs context weight
            float ew = mix(e.w_core, e.w_ctx, driver_lambda_curiosity_);
            float rec = recency(e.ts_last);
            float tr = trust(e);
            product *= ew * rec * tr;
        }
        
        float geo_mean = std::pow(product, 1.0f / edges.size());
        float continuity_bonus = calc_continuity(edges);
        float coverage_bonus = calc_coverage(edges);
        
        return geo_mean * continuity_bonus * coverage_bonus;
    }
    
    // Helper functions for path scoring
    float mix(float w_core, float w_ctx, float lambda) {
        return lambda * w_ctx + (1.0f - lambda) * w_core;
    }
    
    // Calculate size-relative weight scaling factor
    float get_size_scaling_factor() const {
        size_t node_count = store_->node_count();
        size_t edge_count = store_->edge_count();
        
        // Use geometric mean of node and edge counts for scaling
        float db_size = std::sqrt(static_cast<float>(node_count) * static_cast<float>(edge_count));
        
        // Scale factor: smaller databases get higher weights
        // For a database with 100 nodes/edges, scaling = 1.0
        // For a database with 10,000 nodes/edges, scaling = 0.1
        float scaling = 10.0f / std::max(1.0f, std::sqrt(db_size));
        
        return std::clamp(scaling, 0.01f, 10.0f); // Clamp between 0.01 and 10.0
    }
    
    float recency(uint64_t ts_last) {
        uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        uint64_t age_ns = now - ts_last;
        double age_seconds = static_cast<double>(age_ns) / 1e9;
        return std::exp(-age_seconds / 86400.0); // Decay over days
    }
    
    float trust(const EdgeRec& edge) {
        // Trust based on count and flags
        float count_factor = std::min(1.0f, static_cast<float>(edge.count) / 10.0f);
        float flag_factor = (edge.flags & EDGE_DEPRECATED) ? 0.1f : 1.0f;
        return count_factor * flag_factor;
    }
    
    float calc_continuity(const std::vector<EdgeRec>& edges) {
        if (edges.size() < 2) return 1.0f;
        
        float continuity_score = 1.0f;
        for (size_t i = 1; i < edges.size(); ++i) {
            Rel prev_rel = static_cast<Rel>(edges[i-1].rel);
            Rel curr_rel = static_cast<Rel>(edges[i].rel);
            
            // Boost for consistent relations (same 2-bit code)
            if ((static_cast<uint8_t>(prev_rel) & 0x03) == (static_cast<uint8_t>(curr_rel) & 0x03)) {
                continuity_score *= 1.2f;
            }
        }
        
        return continuity_score;
    }
    
    float calc_coverage(const std::vector<EdgeRec>& edges) {
        if (edges.empty()) return 1.0f;
        
        // Reward touching diverse subgraphs (avoid hub lock-in)
        std::unordered_set<NodeID, NodeIDHash> unique_nodes;
        for (const auto& edge : edges) {
            NodeID src, dst;
            std::copy(edge.src, edge.src + 32, src.begin());
            std::copy(edge.dst, edge.dst + 32, dst.begin());
            unique_nodes.insert(src);
            unique_nodes.insert(dst);
        }
        
        float diversity_ratio = static_cast<float>(unique_nodes.size()) / (edges.size() + 1);
        return 0.5f + 0.5f * diversity_ratio; // Range [0.5, 1.0]
    }
    
    // Generalized path finding with multiple strategies
    struct SearchConfig {
        // Traversal parameters
        size_t max_depth = 3;
        size_t beam_size = 5;
        float min_score_threshold = 0.0f; // No threshold - pick highest weight
        
        // Search strategies
        bool use_recall_track = true;
        bool use_explore_track = true;
        bool use_bidirectional = false;
        bool use_semantic_expansion = true;
        
        // Relation preferences based on 2-bit codes
        std::unordered_map<uint8_t, float> relation_weights = {
            {0x00, 1.0f}, // Exact - highest weight
            {0x01, 0.8f}, // Temporal - medium weight  
            {0x02, 0.5f}, // Leap - lower weight
            {0x03, 0.3f}  // Reserved - lowest weight
        };
        
        // Context awareness
        std::string query_context = "";
        std::vector<melvin::NodeID> target_hints = {};
    };
    
    // Multi-strategy path finding
    std::vector<PathCandidate> find_paths_generalized(const NodeID& src, const SearchConfig& config) {
        std::vector<PathCandidate> all_results;
        
        // Strategy 1: Recall Track (strong, durable connections)
        if (config.use_recall_track) {
            RelMask recall_mask;
            recall_mask.set(static_cast<melvin::Rel>(0x00)); // Exact relations
            recall_mask.set(static_cast<melvin::Rel>(0x01)); // Temporal relations
            
            auto recall_paths = find_paths_with_config(src, recall_mask, config);
            all_results.insert(all_results.end(), recall_paths.begin(), recall_paths.end());
        }
        
        // Strategy 2: Explore Track (weaker, contextual connections)
        if (config.use_explore_track) {
            RelMask explore_mask;
            explore_mask.set(static_cast<melvin::Rel>(0x02)); // Leap relations
            explore_mask.set(static_cast<melvin::Rel>(0x03)); // Reserved relations
            
            auto explore_paths = find_paths_with_config(src, explore_mask, config);
            all_results.insert(all_results.end(), explore_paths.begin(), explore_paths.end());
        }
        
        // Strategy 3: Bidirectional search (if enabled)
        if (config.use_bidirectional) {
            auto bidirectional_paths = find_bidirectional_paths(src, config);
            all_results.insert(all_results.end(), bidirectional_paths.begin(), bidirectional_paths.end());
        }
        
        // Strategy 4: Semantic expansion (if enabled)
        if (config.use_semantic_expansion && !config.query_context.empty()) {
            auto semantic_paths = find_semantic_paths(src, config.query_context, config);
            all_results.insert(all_results.end(), semantic_paths.begin(), semantic_paths.end());
        }
        
        // Remove duplicates and sort by weighted score
        std::cout << "🔍 Before deduplication: " << all_results.size() << " paths\n";
        auto final_results = deduplicate_and_rank_paths(all_results, config);
        std::cout << "🔍 After deduplication: " << final_results.size() << " paths\n";
        return final_results;
    }
    
    // Core path finding with configuration
    std::vector<PathCandidate> find_paths_with_config(const NodeID& src, const RelMask& rel_mask, 
                                                     const SearchConfig& config) {
        std::vector<PathCandidate> results;
        std::priority_queue<PathCandidate> beam;
        
        // Initialize with empty path from source
        PathCandidate initial;
        initial.score = 1.0f; // Give initial path a non-zero score
        std::cout << "🔍 Initial beam: pushing initial candidate\n";
        beam.push(initial);
        std::cout << "🔍 Initial beam: size=" << beam.size() << "\n";
        
        for (size_t depth = 0; depth < config.max_depth && !beam.empty(); ++depth) {
            std::cout << "🔍 Beam search depth " << depth << ": beam.size()=" << beam.size() << "\n";
            std::priority_queue<PathCandidate> next_beam;
            
            // Collect results from current beam (paths of length depth+1)
            if (depth > 0) {  // Don't collect empty paths
                std::priority_queue<PathCandidate> beam_copy = beam;
                while (!beam_copy.empty() && results.size() < config.beam_size) {
                    results.push_back(beam_copy.top());
                    beam_copy.pop();
                }
            }
            
            // Process current beam
            while (!beam.empty() && next_beam.size() < config.beam_size) {
                PathCandidate current = beam.top();
                beam.pop();
                
                // Get current node (last node in path)
                NodeID current_node = src;
                if (!current.edges.empty()) {
                    const auto& last_edge = current.edges.back();
                    std::copy(last_edge.dst, last_edge.dst + 32, current_node.begin());
                }
                
                // Expand edges from current node
                auto edges = store_->get_edges_from(current_node);
                for (const auto& edge : edges) {
                    Rel edge_rel = static_cast<Rel>(edge.rel);
                    if (!rel_mask.test(edge_rel)) continue;
                    
                    // Apply relation weight
                    float relation_weight = 0.1f;
                    auto it = config.relation_weights.find(static_cast<uint8_t>(edge_rel) & 0x03);
                    if (it != config.relation_weights.end()) {
                        relation_weight = it->second;
                    }
                    
                    // Create new path candidate
                    PathCandidate candidate = current;
                    candidate.edges.push_back(edge);
                    candidate.score = score_path_weighted(candidate.edges, config);
                    candidate.confidence = compute_confidence(candidate.score);
                    candidate.is_durable = true; // All candidates valid since we pick highest weight
                    
                    // Add all candidates to beam - we'll pick the highest weight later
                        next_beam.push(candidate);
                    std::cout << "🔍 Added to next_beam, size=" << next_beam.size() << ", score=" << candidate.score << "\n";
                }
            }
            
            beam = next_beam;
            std::cout << "🔍 After depth " << depth << ": beam.size()=" << beam.size() << ", next_beam.size()=" << next_beam.size() << "\n";
        }
        
        // Extract top-K results
        std::cout << "🔍 Beam search: beam.size()=" << beam.size() << ", config.beam_size=" << config.beam_size << "\n";
        while (!beam.empty() && results.size() < config.beam_size) {
            results.push_back(beam.top());
            beam.pop();
        }
        
        std::cout << "🔍 Beam search returning " << results.size() << " results\n";
        return results;
    }
    
    // Bidirectional path finding
    std::vector<PathCandidate> find_bidirectional_paths(const NodeID& src, const SearchConfig& config) {
        std::vector<PathCandidate> results;
        
        // Forward search from source
        RelMask forward_mask;
        forward_mask.set_all(); // Allow all relations for bidirectional
        
        auto forward_paths = find_paths_with_config(src, forward_mask, config);
        results.insert(results.end(), forward_paths.begin(), forward_paths.end());
        
        // Backward search to source (if we have target hints)
        for (const auto& target : config.target_hints) {
            RelMask backward_mask;
            backward_mask.set_all();
            
            auto backward_paths = find_paths_with_config(target, backward_mask, config);
            // Reverse the paths to go from src to target
            for (auto& path : backward_paths) {
                std::reverse(path.edges.begin(), path.edges.end());
                results.push_back(path);
            }
        }
        
        return results;
    }
    
    // Semantic-aware path finding
    std::vector<PathCandidate> find_semantic_paths(const NodeID& src, const std::string& query_context, 
                                                   const SearchConfig& config) {
        std::vector<PathCandidate> results;
        
        // Extract semantic hints from query context
        std::vector<std::string> query_words;
        std::istringstream iss(query_context);
        std::string word;
        while (iss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            query_words.push_back(word);
        }
        
        // Find nodes that match query words
        std::vector<melvin::NodeID> semantic_targets;
        for (const auto& word : query_words) {
            auto word_nodes = ingest_text(word);
            semantic_targets.insert(semantic_targets.end(), word_nodes.begin(), word_nodes.end());
        }
        
        // Create semantic search config
        SearchConfig semantic_config = config;
        semantic_config.target_hints = semantic_targets;
        semantic_config.use_bidirectional = true;
        
        // Find paths to semantic targets
        return find_bidirectional_paths(src, semantic_config);
    }
    
    // Weighted path scoring
    float score_path_weighted(const std::vector<EdgeRec>& edges, const SearchConfig& config) {
        if (edges.empty()) return 0.0f;
        
        // Get size-relative scaling factor
        float size_scaling = get_size_scaling_factor();
        
        float product = 1.0f;
        for (const auto& e : edges) {
            // Mix durable vs context weight
            float ew = mix(e.w_core, e.w_ctx, driver_lambda_curiosity_);
            
            // Apply size-relative scaling - facts are more important in smaller databases
            ew *= size_scaling;
            
            float rec = recency(e.ts_last);
            float tr = trust(e);
            
            // Apply relation weight
            Rel edge_rel = static_cast<Rel>(e.rel);
            float rel_weight = 1.0f; // Default to 1.0 instead of 0.1f
            auto it = config.relation_weights.find(static_cast<uint8_t>(edge_rel) & 0x03);
            if (it != config.relation_weights.end()) {
                rel_weight = it->second;
            }
            
            // Ensure minimum values to avoid zero scores
            ew = std::max(ew, 0.1f);
            rec = std::max(rec, 0.1f);
            tr = std::max(tr, 0.1f);
            rel_weight = std::max(rel_weight, 0.1f);
            
            product *= ew * rec * tr * rel_weight;
        }
        
        float geo_mean = std::pow(product, 1.0f / edges.size());
        float continuity_bonus = calc_continuity(edges);
        float coverage_bonus = calc_coverage(edges);
        
        // Ensure bonuses don't reduce score too much
        continuity_bonus = std::max(continuity_bonus, 0.5f);
        coverage_bonus = std::max(coverage_bonus, 0.5f);
        
        float final_score = geo_mean * continuity_bonus * coverage_bonus;
        
        // Ensure minimum score
        final_score = std::max(final_score, 0.1f);
        
        // Debug output
        std::cout << "🔍 score_path_weighted: " << edges.size() << " edges, score=" << final_score 
                  << " (geo_mean=" << geo_mean << ", continuity=" << continuity_bonus 
                  << ", coverage=" << coverage_bonus << ", size_scaling=" << size_scaling << ")\n";
        
        return final_score;
    }
    
    // Deduplicate and rank paths
    std::vector<PathCandidate> deduplicate_and_rank_paths(const std::vector<PathCandidate>& paths, 
                                                          const SearchConfig& /* config */) {
        std::unordered_set<std::string> seen_paths;
        std::vector<PathCandidate> unique_paths;
        
        for (const auto& path : paths) {
            // Create path signature for deduplication
            std::string signature;
            for (const auto& edge : path.edges) {
                signature += std::to_string(edge.src[0]) + "-" + 
                           std::to_string(edge.rel) + "-" + 
                           std::to_string(edge.dst[0]) + "|";
            }
            
            if (seen_paths.find(signature) == seen_paths.end()) {
                seen_paths.insert(signature);
                unique_paths.push_back(path);
            }
        }
        
        // Sort by weighted score
        std::sort(unique_paths.begin(), unique_paths.end(), 
                 [](const PathCandidate& a, const PathCandidate& b) {
                     return a.score > b.score;
                 });
        
        return unique_paths;
    }
    
    float compute_confidence(float path_score) {
        // Map raw score to confidence [0, 1]
        return std::min(1.0f, std::max(0.0f, path_score));
    }
    
    // Enhanced reasoning interface with flexible pattern recognition
    std::string reason_about(const std::string& query) {
        // Store query context for response generation
        set_current_query_context(query);
        
        // Emit COG_STEP for RECALL
        emit_cog_step(CogOp::RECALL, query, "enhanced_reasoning");
        
        // Convert query to nodes
        auto query_nodes = ingest_text(query);
        if (query_nodes.empty()) {
            return "[Uncertain] No clear connection found";
        }
        
        // For "what do X" queries, try to find the main subject
        NodeID start_node = query_nodes[0];
        if (query_nodes.size() >= 3) {
            std::string lower_query = query;
            std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
            
            // Look for "what do" pattern
            size_t what_do_pos = lower_query.find("what do ");
            if (what_do_pos != std::string::npos) {
                // Extract the word after "what do "
                std::string after_what_do = query.substr(what_do_pos + 8);
                std::istringstream iss(after_what_do);
                std::string subject;
                if (iss >> subject) {
                    // Find the node for this subject
                    auto subject_nodes = ingest_text(subject);
                    if (!subject_nodes.empty()) {
                        start_node = subject_nodes[0];
                    }
                }
            }
        }
        
        // Create search configuration based on query type
        SearchConfig config;
        config.query_context = query;
        config.max_depth = 3;
        config.beam_size = 8;
        config.min_score_threshold = 0.0f; // No threshold - we pick highest weight
        
        // Enhanced strategy based on query type and flexible patterns
        std::string lower_query = query;
        std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
        
        // Enhanced query pattern recognition with better parsing
        std::cout << "🔍 Query analysis: '" << query << "' -> '" << lower_query << "'\n";
        
        if (lower_query.find("what do") != std::string::npos) {
            // For "what do X" queries, prioritize behavioral and property relationships
            std::cout << "🔍 Detected 'what do' query pattern\n";
            config.relation_weights[0x00] = 1.3f; // Exact
            config.relation_weights[0x01] = 1.2f; // Temporal
            config.relation_weights[0x02] = 1.1f; // Leap
            config.use_semantic_expansion = true;
            config.use_explore_track = true;
        } else if (lower_query.find("what are") != std::string::npos) {
            // For "what are X" queries, prioritize ISA and similarity relationships
            std::cout << "🔍 Detected 'what are' query pattern\n";
            config.relation_weights[0x00] = 1.4f; // Exact
            config.relation_weights[0x01] = 1.2f; // Temporal
            config.relation_weights[0x02] = 1.1f; // Leap
            config.use_recall_track = true;
            config.use_semantic_expansion = true;
        } else if (lower_query.find("what is") != std::string::npos) {
            // For "what is X" queries, use comprehensive reasoning
            std::cout << "🔍 Detected 'what is' query pattern\n";
            config.relation_weights[0x00] = 1.3f; // Exact
            config.relation_weights[0x01] = 1.2f; // Temporal
            config.relation_weights[0x02] = 1.1f; // Leap
            config.use_bidirectional = true;
            config.use_semantic_expansion = true;
            config.use_recall_track = true;
            config.use_explore_track = true;
        } else if (lower_query.find("can") != std::string::npos) {
            // For "can" queries, prioritize capability relationships
            std::cout << "🔍 Detected 'can' query pattern\n";
            config.relation_weights[0x00] = 1.4f; // Exact
            config.relation_weights[0x01] = 1.3f; // Temporal
            config.relation_weights[0x02] = 1.2f; // Leap
            config.use_semantic_expansion = true;
            config.use_explore_track = true;
        } else if (lower_query.find("are") != std::string::npos) {
            // For "are" queries, prioritize classification relationships
            std::cout << "🔍 Detected 'are' query pattern\n";
            config.relation_weights[0x00] = 1.4f; // Exact
            config.relation_weights[0x01] = 1.2f; // Temporal
            config.relation_weights[0x02] = 1.1f; // Leap
            config.use_recall_track = true;
            config.use_semantic_expansion = true;
        } else if (lower_query.find("where") != std::string::npos) {
            // For "where" queries, prioritize spatial relationships
            std::cout << "🔍 Detected 'where' query pattern\n";
            config.relation_weights[0x00] = 1.4f; // Exact
            config.relation_weights[0x01] = 1.2f; // Temporal
            config.use_semantic_expansion = true;
        } else if (lower_query.find("how") != std::string::npos) {
            // For "how" queries, prioritize causal and behavioral relationships
            std::cout << "🔍 Detected 'how' query pattern\n";
            config.relation_weights[0x00] = 1.3f; // Exact
            config.relation_weights[0x01] = 1.2f; // Temporal
            config.relation_weights[0x02] = 1.1f; // Leap
            config.use_semantic_expansion = true;
        } else if (lower_query.find("why") != std::string::npos) {
            // For "why" queries, prioritize causal relationships
            std::cout << "🔍 Detected 'why' query pattern\n";
            config.relation_weights[0x00] = 1.4f; // Exact
            config.relation_weights[0x01] = 1.3f; // Temporal
            config.use_semantic_expansion = true;
        } else {
            // For general queries, use all strategies with balanced weights
            std::cout << "🔍 Using general query pattern\n";
            config.relation_weights[0x00] = 1.2f; // Exact
            config.relation_weights[0x01] = 1.1f; // Temporal
            config.relation_weights[0x02] = 1.0f; // Leap
            config.use_bidirectional = true;
            config.use_semantic_expansion = true;
            config.use_recall_track = true;
            config.use_explore_track = true;
        }
        
        // Emit COG_STEP for RECALL
        emit_cog_step(CogOp::RECALL, query, "generalized_search");
        
        // Use generalized path finding
        auto all_paths = find_paths_generalized(start_node, config);
        
        // Emit COG_STEP for INTEGRATE
        emit_cog_step(CogOp::INTEGRATE, query, "integrate_paths");
        
        if (all_paths.empty()) {
            std::cout << "🔍 No paths found at all\n";
            return "[Uncertain] No clear connection found";
        }
        
        // Sort by score
        std::sort(all_paths.begin(), all_paths.end(), 
                 [](const PathCandidate& a, const PathCandidate& b) {
                     return a.score > b.score;
                 });
        
        const auto& best_path = all_paths[0];
        
        // Debug output
        std::cout << "🔍 Found " << all_paths.size() << " paths, best score=" << best_path.score 
                  << ", confidence=" << best_path.confidence << "\n";
        std::cout << "🔍 tau_high_=" << tau_high_ << ", tau_mid_=" << tau_mid_ << "\n";
        
        // Emit COG_STEP for DECIDE
        emit_cog_step(CogOp::DECIDE, query, "choose_best_path");
        
        // Always pick the memory with the highest weight/score
        std::cout << "🔍 Returning highest weight memory (score=" << best_path.score << ")\n";
        return generate_output(best_path, "[Selected]");
    }
    
    std::string generate_output(const PathCandidate& path, const std::string& prefix) {
        if (path.edges.empty()) {
            return prefix + " No clear connection found";
        }
        
        // Generate contextual response based on query and path content
        std::string result = generate_contextual_response(path, prefix);
        
        // Anti-repeat guard: check for bigram repetition
        if (has_repetition(result)) {
            result = generate_variation(result);
        }
        
        // Update anti-repeat state
        update_anti_repeat_state(result);
        
        return result;
    }
    
    // Extract actual node content from the knowledge graph
    std::string extract_node_content(const uint8_t node_id[32]) {
        NodeID node;
        std::copy(node_id, node_id + 32, node.begin());
        NodeRecHeader header;
        std::vector<uint8_t> payload;
        if (store_->get_node(node, header, payload)) {
            std::string content(payload.begin(), payload.end());
            // Debug output
            std::cout << "🔍 extract_node_content: found node with content='" << content << "'\n";
            return content;
        }
        // Debug output for missing nodes
        std::cout << "🔍 extract_node_content: node not found, returning [unknown]\n";
        return "[unknown]";
    }
    
    // Generate contextual response based on query type and path content
    std::string generate_contextual_response(const PathCandidate& path, const std::string& prefix) {
        if (path.edges.empty()) {
            return prefix + " No clear connection found";
        }
        
        // Get the current query context from the reasoning engine
        std::string query_context = get_current_query_context();
        std::string lower_query = query_context;
        std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
        
        // Generate response based on query type
        std::string response;
        if (lower_query.find("what is") != std::string::npos) {
            response = generate_definition_response(path);
        } else if (lower_query.find("what are") != std::string::npos) {
            response = generate_definition_response(path);
        } else if (lower_query.find("what do") != std::string::npos) {
            response = generate_behavior_response(path);
        } else if (lower_query.find("can") != std::string::npos) {
            response = generate_capability_response(path);
        } else if (lower_query.find("are") != std::string::npos) {
            response = generate_classification_response(path);
        } else if (lower_query.find("where") != std::string::npos) {
            response = generate_location_response(path);
        } else if (lower_query.find("how") != std::string::npos) {
            response = generate_process_response(path);
        } else if (lower_query.find("why") != std::string::npos) {
            response = generate_causal_response(path);
        } else {
            response = generate_general_response(path);
        }
        
        // Format with confidence
        response = format_with_confidence(response, path.confidence);
        
        return prefix + " " + response;
    }
    
    // Generate definition responses for "what is/are" queries
    std::string generate_definition_response(const PathCandidate& path) {
        if (path.edges.empty()) return "I don't know what that is.";
        
        std::stringstream response;
        std::string subject = extract_node_content(path.edges[0].src);
        std::vector<std::string> definitions;
        
        for (const auto& edge : path.edges) {
            std::string object = extract_node_content(edge.dst);
            Rel rel = static_cast<Rel>(edge.rel);
            
            switch (rel) {
                case melvin::Rel::ISA:
                    definitions.push_back(subject + " is a " + object);
                    break;
                case melvin::Rel::HAS_PROPERTY:
                    definitions.push_back(subject + " has " + object);
                    break;
                case melvin::Rel::PART_OF:
                    definitions.push_back(subject + " is part of " + object);
                    break;
                case melvin::Rel::SIMILAR_TO:
                    definitions.push_back(subject + " is similar to " + object);
                    break;
                case melvin::Rel::DRINKS:
                    definitions.push_back(subject + " drinks " + object);
                    break;
                case melvin::Rel::CAUSES:
                    definitions.push_back(subject + " causes " + object);
                    break;
                case melvin::Rel::NEXT:
                    definitions.push_back(subject + " comes before " + object);
                    break;
                case melvin::Rel::CONTRADICTS:
                    definitions.push_back(subject + " contradicts " + object);
                    break;
                case melvin::Rel::GENERATED_FROM:
                    definitions.push_back(subject + " is generated from " + object);
                    break;
                case melvin::Rel::EFFECT:
                    definitions.push_back(subject + " has effect " + object);
                    break;
                case melvin::Rel::LOCATED_IN:
                    definitions.push_back(subject + " is located in " + object);
                    break;
                case melvin::Rel::TEMPORAL_BEFORE:
                    definitions.push_back(subject + " happens before " + object);
                    break;
                case melvin::Rel::TEMPORAL_AFTER:
                    definitions.push_back(subject + " happens after " + object);
                    break;
                default:
                    definitions.push_back(subject + " relates to " + object);
                    break;
            }
        }
        
        if (definitions.empty()) {
            return subject + " is something I know about.";
        }
        
        // Join definitions
        for (size_t i = 0; i < definitions.size(); ++i) {
            response << definitions[i];
            if (i < definitions.size() - 1) {
                response << ", and ";
            }
        }
        
        return response.str();
    }
    
    // Generate behavior responses for "what do" queries
    std::string generate_behavior_response(const PathCandidate& path) {
        if (path.edges.empty()) return "I don't know what that does.";
        
        std::stringstream response;
        std::string subject = extract_node_content(path.edges[0].src);
        std::vector<std::string> behaviors;
        
        for (const auto& edge : path.edges) {
            std::string object = extract_node_content(edge.dst);
            Rel rel = static_cast<Rel>(edge.rel);
            
            switch (rel) {
                case melvin::Rel::DRINKS:
                    behaviors.push_back(subject + " drinks " + object);
                    break;
                case melvin::Rel::HAS_PROPERTY:
                    behaviors.push_back(subject + " has " + object);
                    break;
                case melvin::Rel::CAUSES:
                    behaviors.push_back(subject + " causes " + object);
                    break;
                case melvin::Rel::ISA:
                    behaviors.push_back(subject + " is a " + object);
                    break;
                case melvin::Rel::PART_OF:
                    behaviors.push_back(subject + " is part of " + object);
                    break;
                case melvin::Rel::NEXT:
                    behaviors.push_back(subject + " comes before " + object);
                    break;
                case melvin::Rel::SIMILAR_TO:
                    behaviors.push_back(subject + " is similar to " + object);
                    break;
                case melvin::Rel::CONTRADICTS:
                    behaviors.push_back(subject + " contradicts " + object);
                    break;
                case melvin::Rel::GENERATED_FROM:
                    behaviors.push_back(subject + " is generated from " + object);
                    break;
                case melvin::Rel::EFFECT:
                    behaviors.push_back(subject + " has effect " + object);
                    break;
                case melvin::Rel::LOCATED_IN:
                    behaviors.push_back(subject + " is located in " + object);
                    break;
                case melvin::Rel::TEMPORAL_BEFORE:
                    behaviors.push_back(subject + " happens before " + object);
                    break;
                case melvin::Rel::TEMPORAL_AFTER:
                    behaviors.push_back(subject + " happens after " + object);
                    break;
                default:
                    behaviors.push_back(subject + " relates to " + object);
                    break;
            }
        }
        
        if (behaviors.empty()) {
            return subject + " has various properties and behaviors.";
        }
        
        // Join behaviors
        for (size_t i = 0; i < behaviors.size(); ++i) {
            response << behaviors[i];
            if (i < behaviors.size() - 1) {
                response << ", and ";
            }
        }
        
        return response.str();
    }
    
    // Generate capability responses for "can" queries
    std::string generate_capability_response(const PathCandidate& path) {
        if (path.edges.empty()) return "I don't know if that can do that.";
        
        std::stringstream response;
        std::string subject = extract_node_content(path.edges[0].src);
        std::vector<std::string> capabilities;
        
        for (const auto& edge : path.edges) {
            std::string object = extract_node_content(edge.dst);
            Rel rel = static_cast<Rel>(edge.rel);
            
            switch (rel) {
                case melvin::Rel::DRINKS:
                    capabilities.push_back(subject + " can drink " + object);
                    break;
                case melvin::Rel::HAS_PROPERTY:
                    capabilities.push_back(subject + " has " + object);
                    break;
                case melvin::Rel::ISA:
                    // Check if this is a negative capability (like "cannot fly")
                    if (object.find("not") != std::string::npos || object.find("cannot") != std::string::npos) {
                        capabilities.push_back(subject + " " + object);
                    } else {
                        capabilities.push_back(subject + " is a " + object);
                    }
                    break;
                case melvin::Rel::PART_OF:
                    capabilities.push_back(subject + " is part of " + object);
                    break;
                case melvin::Rel::CAUSES:
                    capabilities.push_back(subject + " causes " + object);
                    break;
                case melvin::Rel::NEXT:
                    capabilities.push_back(subject + " comes before " + object);
                    break;
                case melvin::Rel::SIMILAR_TO:
                    capabilities.push_back(subject + " is similar to " + object);
                    break;
                case melvin::Rel::CONTRADICTS:
                    capabilities.push_back(subject + " contradicts " + object);
                    break;
                case melvin::Rel::GENERATED_FROM:
                    capabilities.push_back(subject + " is generated from " + object);
                    break;
                case melvin::Rel::EFFECT:
                    capabilities.push_back(subject + " has effect " + object);
                    break;
                case melvin::Rel::LOCATED_IN:
                    capabilities.push_back(subject + " is located in " + object);
                    break;
                case melvin::Rel::TEMPORAL_BEFORE:
                    capabilities.push_back(subject + " happens before " + object);
                    break;
                case melvin::Rel::TEMPORAL_AFTER:
                    capabilities.push_back(subject + " happens after " + object);
                    break;
                default:
                    capabilities.push_back(subject + " relates to " + object);
                    break;
            }
        }
        
        if (capabilities.empty()) {
            return "I'm not sure about that capability.";
        }
        
        // Join capabilities
        for (size_t i = 0; i < capabilities.size(); ++i) {
            response << capabilities[i];
            if (i < capabilities.size() - 1) {
                response << ", and ";
            }
        }
        
        return response.str();
    }
    
    // Generate classification responses for "are" queries
    std::string generate_classification_response(const PathCandidate& path) {
        if (path.edges.empty()) return "I don't know about that classification.";
        
        std::stringstream response;
        std::string subject = extract_node_content(path.edges[0].src);
        std::vector<std::string> classifications;
        
        for (const auto& edge : path.edges) {
            std::string object = extract_node_content(edge.dst);
            Rel rel = static_cast<Rel>(edge.rel);
            
            switch (rel) {
                case melvin::Rel::ISA:
                    classifications.push_back(subject + " are " + object);
                    break;
                case melvin::Rel::HAS_PROPERTY:
                    classifications.push_back(subject + " have " + object);
                    break;
                case melvin::Rel::SIMILAR_TO:
                    classifications.push_back(subject + " are similar to " + object);
                    break;
                case melvin::Rel::PART_OF:
                    classifications.push_back(subject + " are part of " + object);
                    break;
                case melvin::Rel::DRINKS:
                    classifications.push_back(subject + " drink " + object);
                    break;
                case melvin::Rel::CAUSES:
                    classifications.push_back(subject + " cause " + object);
                    break;
                case melvin::Rel::NEXT:
                    classifications.push_back(subject + " come before " + object);
                    break;
                case melvin::Rel::CONTRADICTS:
                    classifications.push_back(subject + " contradict " + object);
                    break;
                case melvin::Rel::GENERATED_FROM:
                    classifications.push_back(subject + " are generated from " + object);
                    break;
                case melvin::Rel::EFFECT:
                    classifications.push_back(subject + " have effect " + object);
                    break;
                case melvin::Rel::LOCATED_IN:
                    classifications.push_back(subject + " are located in " + object);
                    break;
                case melvin::Rel::TEMPORAL_BEFORE:
                    classifications.push_back(subject + " happen before " + object);
                    break;
                case melvin::Rel::TEMPORAL_AFTER:
                    classifications.push_back(subject + " happen after " + object);
                    break;
                default:
                    classifications.push_back(subject + " relate to " + object);
                    break;
            }
        }
        
        if (classifications.empty()) {
            return subject + " have various characteristics.";
        }
        
        // Join classifications
        for (size_t i = 0; i < classifications.size(); ++i) {
            response << classifications[i];
            if (i < classifications.size() - 1) {
                response << ", and ";
            }
        }
        
        return response.str();
    }
    
    // Generate location responses for "where" queries
    std::string generate_location_response(const PathCandidate& path) {
        if (path.edges.empty()) return "I don't know where that is.";
        
        std::stringstream response;
        std::string subject = extract_node_content(path.edges[0].src);
        std::vector<std::string> locations;
        
        for (const auto& edge : path.edges) {
            std::string object = extract_node_content(edge.dst);
            Rel rel = static_cast<Rel>(edge.rel);
            
            switch (rel) {
                case melvin::Rel::LOCATED_IN:
                    locations.push_back(subject + " is located in " + object);
                    break;
                case melvin::Rel::PART_OF:
                    locations.push_back(subject + " is part of " + object);
                    break;
                case melvin::Rel::ISA:
                    locations.push_back(subject + " is a " + object);
                    break;
                case melvin::Rel::HAS_PROPERTY:
                    locations.push_back(subject + " has " + object);
                    break;
                case melvin::Rel::DRINKS:
                    locations.push_back(subject + " drinks " + object);
                    break;
                case melvin::Rel::CAUSES:
                    locations.push_back(subject + " causes " + object);
                    break;
                case melvin::Rel::NEXT:
                    locations.push_back(subject + " comes before " + object);
                    break;
                case melvin::Rel::SIMILAR_TO:
                    locations.push_back(subject + " is similar to " + object);
                    break;
                case melvin::Rel::CONTRADICTS:
                    locations.push_back(subject + " contradicts " + object);
                    break;
                case melvin::Rel::GENERATED_FROM:
                    locations.push_back(subject + " is generated from " + object);
                    break;
                case melvin::Rel::EFFECT:
                    locations.push_back(subject + " has effect " + object);
                    break;
                case melvin::Rel::TEMPORAL_BEFORE:
                    locations.push_back(subject + " happens before " + object);
                    break;
                case melvin::Rel::TEMPORAL_AFTER:
                    locations.push_back(subject + " happens after " + object);
                    break;
                default:
                    locations.push_back(subject + " relates to " + object);
                    break;
            }
        }
        
        if (locations.empty()) {
            return subject + " has a specific location.";
        }
        
        // Join locations
        for (size_t i = 0; i < locations.size(); ++i) {
            response << locations[i];
            if (i < locations.size() - 1) {
                response << ", and ";
            }
        }
        
        return response.str();
    }
    
    // Generate process responses for "how" queries
    std::string generate_process_response(const PathCandidate& path) {
        if (path.edges.empty()) return "I don't know how that works.";
        
        std::stringstream response;
        std::string subject = extract_node_content(path.edges[0].src);
        std::vector<std::string> processes;
        
        for (const auto& edge : path.edges) {
            std::string object = extract_node_content(edge.dst);
            Rel rel = static_cast<Rel>(edge.rel);
            
            switch (rel) {
                case melvin::Rel::CAUSES:
                    processes.push_back(subject + " causes " + object);
                    break;
                case melvin::Rel::HAS_PROPERTY:
                    processes.push_back(subject + " has " + object);
                    break;
                case melvin::Rel::ISA:
                    processes.push_back(subject + " is a " + object);
                    break;
                case melvin::Rel::PART_OF:
                    processes.push_back(subject + " is part of " + object);
                    break;
                case melvin::Rel::DRINKS:
                    processes.push_back(subject + " drinks " + object);
                    break;
                case melvin::Rel::NEXT:
                    processes.push_back(subject + " comes before " + object);
                    break;
                case melvin::Rel::SIMILAR_TO:
                    processes.push_back(subject + " is similar to " + object);
                    break;
                case melvin::Rel::CONTRADICTS:
                    processes.push_back(subject + " contradicts " + object);
                    break;
                case melvin::Rel::GENERATED_FROM:
                    processes.push_back(subject + " is generated from " + object);
                    break;
                case melvin::Rel::EFFECT:
                    processes.push_back(subject + " has effect " + object);
                    break;
                case melvin::Rel::LOCATED_IN:
                    processes.push_back(subject + " is located in " + object);
                    break;
                case melvin::Rel::TEMPORAL_BEFORE:
                    processes.push_back(subject + " happens before " + object);
                    break;
                case melvin::Rel::TEMPORAL_AFTER:
                    processes.push_back(subject + " happens after " + object);
                    break;
                default:
                    processes.push_back(subject + " relates to " + object);
                    break;
            }
        }
        
        if (processes.empty()) {
            return subject + " works through specific mechanisms.";
        }
        
        // Join processes
        for (size_t i = 0; i < processes.size(); ++i) {
            response << processes[i];
            if (i < processes.size() - 1) {
                response << ", and ";
            }
        }
        
        return response.str();
    }
    
    // Generate causal responses for "why" queries
    std::string generate_causal_response(const PathCandidate& path) {
        if (path.edges.empty()) return "I don't know why that happens.";
        
        std::stringstream response;
        std::string subject = extract_node_content(path.edges[0].src);
        std::vector<std::string> causes;
        
        for (const auto& edge : path.edges) {
            std::string object = extract_node_content(edge.dst);
            Rel rel = static_cast<Rel>(edge.rel);
            
            switch (rel) {
                case melvin::Rel::CAUSES:
                    causes.push_back(subject + " causes " + object);
                    break;
                case melvin::Rel::EFFECT:
                    causes.push_back(subject + " has the effect of " + object);
                    break;
                case melvin::Rel::HAS_PROPERTY:
                    causes.push_back(subject + " has " + object);
                    break;
                case melvin::Rel::ISA:
                    causes.push_back(subject + " is a " + object);
                    break;
                case melvin::Rel::PART_OF:
                    causes.push_back(subject + " is part of " + object);
                    break;
                case melvin::Rel::DRINKS:
                    causes.push_back(subject + " drinks " + object);
                    break;
                case melvin::Rel::NEXT:
                    causes.push_back(subject + " comes before " + object);
                    break;
                case melvin::Rel::SIMILAR_TO:
                    causes.push_back(subject + " is similar to " + object);
                    break;
                case melvin::Rel::CONTRADICTS:
                    causes.push_back(subject + " contradicts " + object);
                    break;
                case melvin::Rel::GENERATED_FROM:
                    causes.push_back(subject + " is generated from " + object);
                    break;
                case melvin::Rel::LOCATED_IN:
                    causes.push_back(subject + " is located in " + object);
                    break;
                case melvin::Rel::TEMPORAL_BEFORE:
                    causes.push_back(subject + " happens before " + object);
                    break;
                case melvin::Rel::TEMPORAL_AFTER:
                    causes.push_back(subject + " happens after " + object);
                    break;
                default:
                    causes.push_back(subject + " relates to " + object);
                    break;
            }
        }
        
        if (causes.empty()) {
            return subject + " has various causes and effects.";
        }
        
        // Join causes
        for (size_t i = 0; i < causes.size(); ++i) {
            response << causes[i];
            if (i < causes.size() - 1) {
                response << ", and ";
            }
        }
        
        return response.str();
    }
    
    // Generate general responses for other queries
    std::string generate_general_response(const PathCandidate& path) {
        if (path.edges.empty()) return "I don't have information about that.";
        
        std::stringstream response;
        std::string subject = extract_node_content(path.edges[0].src);
        std::vector<std::string> relations;
        
        for (const auto& edge : path.edges) {
            std::string object = extract_node_content(edge.dst);
            Rel rel = static_cast<Rel>(edge.rel);
            
            switch (rel) {
                case melvin::Rel::ISA:
                    relations.push_back(subject + " is a " + object);
                    break;
                case melvin::Rel::HAS_PROPERTY:
                    relations.push_back(subject + " has " + object);
                    break;
                case melvin::Rel::SIMILAR_TO:
                    relations.push_back(subject + " is similar to " + object);
                    break;
                case melvin::Rel::PART_OF:
                    relations.push_back(subject + " is part of " + object);
                    break;
                case melvin::Rel::CAUSES:
                    relations.push_back(subject + " causes " + object);
                    break;
                case melvin::Rel::DRINKS:
                    relations.push_back(subject + " drinks " + object);
                    break;
                default:
                    relations.push_back(subject + " relates to " + object);
                    break;
            }
        }
        
        if (relations.empty()) {
            return subject + " has various relationships.";
        }
        
        // Join relations
        for (size_t i = 0; i < relations.size(); ++i) {
            response << relations[i];
            if (i < relations.size() - 1) {
                response << ", and ";
            }
        }
        
        return response.str();
    }
    
    // Format response with confidence level
    std::string format_with_confidence(const std::string& response, float confidence) {
        if (confidence > 0.8f) {
            return response + ".";
        } else if (confidence > 0.5f) {
            return "I think " + response + ".";
        } else {
            return "I'm not sure, but " + response + ".";
        }
    }
    
    // Get current query context (stored during reasoning)
    std::string get_current_query_context() {
        return current_query_context_;
    }
    
    // Store current query context
    void set_current_query_context(const std::string& query) {
        current_query_context_ = query;
    }
    
    bool has_repetition(const std::string& text) {
        std::istringstream iss(text);
        std::string word1, word2;
        std::unordered_set<std::string> bigrams;
        
        if (iss >> word1) {
            while (iss >> word2) {
                std::string bigram = word1 + " " + word2;
                if (recent_bigrams_.count(bigram)) {
                    return true;
                }
                bigrams.insert(bigram);
                word1 = word2;
            }
        }
        
        return false;
    }
    
    std::string generate_variation(const std::string& original) {
        // Simple variation by rephrasing
        if (original.find("X is a Y") != std::string::npos) {
            return "[Variation] Y includes X. ";
        } else if (original.find("X causes Y") != std::string::npos) {
            return "[Variation] Y results from X. ";
        } else {
            return "[Variation] " + original;
        }
    }
    
    void update_anti_repeat_state(const std::string& text) {
        std::istringstream iss(text);
        std::string word1, word2;
        
        if (iss >> word1) {
            while (iss >> word2) {
                std::string bigram = word1 + " " + word2;
                recent_bigrams_.insert(bigram);
                word1 = word2;
            }
        }
        
        // Keep only recent bigrams (sliding window)
        if (recent_bigrams_.size() > 100) {
            recent_bigrams_.clear();
        }
    }
    
    void emit_cog_step(CogOp op, const std::string& /* inputs */, const std::string& /* outputs */) {
        // Create COG_STEP node
        CogStep step;
        step.op = static_cast<uint8_t>(op);
        step.conf = 0.8f; // Default confidence
        step.drivers[0] = drivers_.curiosity;
        step.drivers[1] = drivers_.efficiency;
        step.drivers[2] = drivers_.consistency;
        step.drivers[3] = drivers_.social;
        step.drivers[4] = drivers_.survival;
        step.num_inputs = 1;
        step.num_outputs = 1;
        step.latency_us = 1000; // 1ms default
        step.err_flags = NO_ERROR;
        
        // Store COG_STEP node
        NodeRecHeader header;
        header.type = static_cast<uint32_t>(melvin::NodeType::META_COG_STEP);
        header.flags = 0;
        header.ts_created = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        header.ts_updated = header.ts_created;
        header.payload_len = sizeof(CogStep);
        header.degree_hint = 0;
        
        std::vector<uint8_t> payload(reinterpret_cast<uint8_t*>(&step), 
                                   reinterpret_cast<uint8_t*>(&step) + sizeof(step));
        
        store_->upsert_node(header, payload);
    }
    
    std::vector<melvin::NodeID> ingest_text(const std::string& text) {
        std::vector<melvin::NodeID> nodes;
        
        // Simple tokenization
        std::istringstream iss(text);
        std::string token;
        melvin::NodeID prev_node;
        
        while (iss >> token) {
            // Normalize token
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            
            // Create symbol node
            melvin::NodeRecHeader header;
            header.type = static_cast<uint32_t>(melvin::NodeType::SYMBOL);
            header.flags = 0;
            header.payload_len = static_cast<uint32_t>(token.size());
            
            std::vector<uint8_t> payload(token.begin(), token.end());
            melvin::NodeID node_id = store_->upsert_node(header, payload);
            nodes.push_back(node_id);
            
            prev_node = node_id;
        }
        
        return nodes;
    }
};

// Forward declaration for UnifiedReasoningSystem
class UnifiedReasoningSystem;

    }

// ============================================================================
// REASONING SYSTEM COMPONENTS
// ============================================================================

// Enhanced reasoning path parser for extracting key answers
class ReasoningPathParser {
private:
    melvin::Store* store_;
    melvin::VMContext* vm_context_;
    
    // Answer extraction patterns
    struct AnswerPattern {
        std::string query_type;
        std::vector<melvin::Rel> preferred_relations;
        std::string response_template;
        float confidence_threshold;
    };
    
    std::vector<AnswerPattern> answer_patterns_;
    
    // Natural language mapping for relations
    std::unordered_map<melvin::Rel, std::string> relation_to_natural_language_;
    
    // Confidence scoring weights
    struct ConfidenceWeights {
        float path_score_weight = 0.4f;
        float relation_strength_weight = 0.3f;
        float path_length_weight = 0.1f;
        float semantic_coherence_weight = 0.2f;
    } confidence_weights_;

public:
    ReasoningPathParser(melvin::Store* store, melvin::VMContext* vm_context)
        : store_(store), vm_context_(vm_context) {
        initialize_answer_patterns();
        initialize_relation_mappings();
    }
    
    // Initialize answer extraction patterns based on query types
    void initialize_answer_patterns() {
        answer_patterns_ = {
            // "What is/are" queries - definition extraction
            {
                "what_is",
                {melvin::Rel::ISA, melvin::Rel::HAS_PROPERTY, melvin::Rel::PART_OF},
                "{subject} is {answer}",
                0.6f
            },
            
            // "Where" queries - location extraction
            {
                "where",
                {melvin::Rel::LOCATED_IN, melvin::Rel::PART_OF},
                "{subject} is located in {answer}",
                0.7f
            },
            
            // "How" queries - process extraction
            {
                "how",
                {melvin::Rel::CAUSES, melvin::Rel::EFFECT, melvin::Rel::HAS_PROPERTY},
                "{subject} works by {answer}",
                0.5f
            },
            
            // "Why" queries - causal extraction
            {
                "why",
                {melvin::Rel::CAUSES, melvin::Rel::EFFECT},
                "{subject} because {answer}",
                0.6f
            },
            
            // "Can" queries - capability extraction
            {
                "can",
                {melvin::Rel::HAS_PROPERTY, melvin::Rel::ISA},
                "{subject} can {answer}",
                0.5f
            },
            
            // "Are" queries - classification extraction
            {
                "are",
                {melvin::Rel::ISA, melvin::Rel::SIMILAR_TO},
                "{subject} are {answer}",
                0.6f
            }
        };
    }
    
    // Initialize natural language mappings for relations
    void initialize_relation_mappings() {
        relation_to_natural_language_ = {
            {melvin::Rel::ISA, "is a"},
            {melvin::Rel::PART_OF, "is part of"},
            {melvin::Rel::HAS_PROPERTY, "has"},
            {melvin::Rel::SIMILAR_TO, "is similar to"},
            {melvin::Rel::LOCATED_IN, "is located in"},
            {melvin::Rel::CAUSES, "causes"},
            {melvin::Rel::EFFECT, "has effect"},
            {melvin::Rel::DRINKS, "drinks"},
            {melvin::Rel::NEXT, "comes before"},
            {melvin::Rel::CONTRADICTS, "contradicts"},
            {melvin::Rel::GENERATED_FROM, "is generated from"},
            {melvin::Rel::TEMPORAL_BEFORE, "happens before"},
            {melvin::Rel::TEMPORAL_AFTER, "happens after"}
        };
    }
    
    // Parse reasoning path to extract key answer
    std::string parse_reasoning_path(const std::vector<melvin::Path>& paths, const std::string& query) {
        std::cout << "🔍 PARSING REASONING PATH FOR ANSWER EXTRACTION\n";
        std::cout << "==============================================\n\n";
        
        if (paths.empty()) {
            return "[No reasoning paths found]";
        }
        
        // Identify query type
        std::string query_type = identify_query_type(query);
        std::cout << "📝 Query type: " << query_type << "\n";
        
        // Extract answers from each path
        std::vector<ExtractedAnswer> extracted_answers;
        for (size_t i = 0; i < paths.size(); ++i) {
            auto answer = extract_answer_from_path(paths[i], query_type, i);
            if (!answer.content.empty()) {
                extracted_answers.push_back(answer);
            }
        }
        
        if (extracted_answers.empty()) {
            return "[No clear answer found in reasoning paths]";
        }
        
        // Select best answer
        auto best_answer = select_best_answer(extracted_answers, query_type);
        
        // Format response
        std::string formatted_response = format_response(best_answer, query_type);
        
        std::cout << "✅ Answer extraction complete\n";
        std::cout << "🎯 Best answer: " << best_answer.content << "\n";
        std::cout << "📊 Confidence: " << best_answer.confidence << "\n\n";
        
        return formatted_response;
    }
    
    // Extract answer from a single reasoning path
    struct ExtractedAnswer {
        std::string content;
        float confidence;
        size_t path_index;
        std::vector<melvin::Rel> used_relations;
        std::string reasoning_chain;
    };
    
    ExtractedAnswer extract_answer_from_path(const melvin::Path& path, const std::string& query_type, size_t path_index) {
        ExtractedAnswer answer;
        answer.path_index = path_index;
        answer.confidence = path.confidence;
        
        if (path.edges.empty()) {
            return answer;
        }
        
        // Find the answer pattern for this query type
        AnswerPattern* pattern = nullptr;
        for (auto& p : answer_patterns_) {
            if (p.query_type == query_type) {
                pattern = &p;
                break;
            }
        }
        
        if (!pattern) {
            // Use general pattern
            answer.content = extract_general_answer(path);
            return answer;
        }
        
        // Extract answer based on preferred relations
        std::vector<std::string> answer_components;
        std::stringstream reasoning_chain;
        
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                melvin::Rel relation = static_cast<melvin::Rel>(edge.rel);
                
                // Check if this relation is preferred for this query type
                bool is_preferred = std::find(pattern->preferred_relations.begin(), 
                                            pattern->preferred_relations.end(), 
                                            relation) != pattern->preferred_relations.end();
                
                if (is_preferred) {
                    melvin::NodeID src_id, dst_id;
                    std::copy(edge.src, edge.src + 32, src_id.begin());
                    std::copy(edge.dst, edge.dst + 32, dst_id.begin());
                    std::string subject = extract_node_content(src_id);
                    std::string object = extract_node_content(dst_id);
                    
                    // Map relation to natural language
                    std::string relation_text = relation_to_natural_language_[relation];
                    
                    // Build answer component
                    std::string component = subject + " " + relation_text + " " + object;
                    answer_components.push_back(component);
                    
                    // Build reasoning chain
                    if (!reasoning_chain.str().empty()) {
                        reasoning_chain << " → ";
                    }
                    reasoning_chain << component;
                    
                    answer.used_relations.push_back(relation);
                }
            }
        }
        
        // Combine answer components
        if (!answer_components.empty()) {
            for (size_t i = 0; i < answer_components.size(); ++i) {
                answer.content += answer_components[i];
                if (i < answer_components.size() - 1) {
                    answer.content += ", and ";
                }
            }
        } else {
            // Fallback to general extraction
            answer.content = extract_general_answer(path);
        }
        
        answer.reasoning_chain = reasoning_chain.str();
        
        // Calculate enhanced confidence
        answer.confidence = calculate_enhanced_confidence(path, answer, *pattern);
        
        return answer;
    }
    
    // Extract general answer when no specific pattern matches
    std::string extract_general_answer(const melvin::Path& path) {
        if (path.edges.empty()) {
            return "No specific answer found";
        }
        
        // Get the final destination node in the path
        auto last_edge = path.edges.back();
        melvin::EdgeRec edge;
        if (store_->get_edge(last_edge, edge)) {
            melvin::NodeID dst_id;
            std::copy(edge.dst, edge.dst + 32, dst_id.begin());
            return extract_node_content(dst_id);
        }
        
        return "No specific answer found";
    }
    
    // Calculate enhanced confidence score
    float calculate_enhanced_confidence(const melvin::Path& path, const ExtractedAnswer& answer, const AnswerPattern& pattern) {
        float base_confidence = path.confidence;
        
        // Relation strength bonus
        float relation_strength = 0.0f;
        for (const auto& rel : answer.used_relations) {
            if (std::find(pattern.preferred_relations.begin(), pattern.preferred_relations.end(), rel) != pattern.preferred_relations.end()) {
                relation_strength += 0.2f;
            }
        }
        relation_strength = std::min(relation_strength, 1.0f);
        
        // Path length penalty (shorter paths are generally better)
        float length_penalty = std::max(0.0f, 1.0f - (path.edges.size() - 1) * 0.1f);
        
        // Semantic coherence bonus
        float coherence_bonus = answer.used_relations.size() > 0 ? 0.1f : 0.0f;
        
        // Calculate final confidence
        float enhanced_confidence = base_confidence * confidence_weights_.path_score_weight +
                                  relation_strength * confidence_weights_.relation_strength_weight +
                                  length_penalty * confidence_weights_.path_length_weight +
                                  coherence_bonus * confidence_weights_.semantic_coherence_weight;
        
        return std::clamp(enhanced_confidence, 0.0f, 1.0f);
    }
    
    // Select the best answer from multiple extracted answers
    ExtractedAnswer select_best_answer(const std::vector<ExtractedAnswer>& answers, const std::string& query_type) {
        if (answers.empty()) {
            return ExtractedAnswer{};
        }
        
        if (answers.size() == 1) {
            return answers[0];
        }
        
        // Find the answer pattern for confidence threshold
        AnswerPattern* pattern = nullptr;
        for (auto& p : answer_patterns_) {
            if (p.query_type == query_type) {
                pattern = &p;
                break;
            }
        }
        
        float threshold = pattern ? pattern->confidence_threshold : 0.5f;
        
        // Filter answers above threshold
        std::vector<ExtractedAnswer> qualified_answers;
        for (const auto& answer : answers) {
            if (answer.confidence >= threshold) {
                qualified_answers.push_back(answer);
            }
        }
        
        if (qualified_answers.empty()) {
            // If no answers meet threshold, return highest confidence
            auto best = std::max_element(answers.begin(), answers.end(),
                [](const ExtractedAnswer& a, const ExtractedAnswer& b) {
                    return a.confidence < b.confidence;
                });
            return *best;
        }
        
        // Select best qualified answer
        auto best = std::max_element(qualified_answers.begin(), qualified_answers.end(),
            [](const ExtractedAnswer& a, const ExtractedAnswer& b) {
                // Prioritize confidence, then path index (earlier paths are better)
                if (std::abs(a.confidence - b.confidence) < 0.01f) {
                    return a.path_index > b.path_index;
                }
                return a.confidence < b.confidence;
            });
        
        return *best;
    }
    
    // Format the final response appropriately
    std::string format_response(const ExtractedAnswer& answer, const std::string& query_type) {
        if (answer.content.empty()) {
            return "[No answer could be extracted]";
        }
        
        std::stringstream response;
        
        // Add confidence indicator
        if (answer.confidence >= 0.8f) {
            response << "[High Confidence] ";
        } else if (answer.confidence >= 0.6f) {
            response << "[Medium Confidence] ";
        } else if (answer.confidence >= 0.4f) {
            response << "[Low Confidence] ";
        } else {
            response << "[Uncertain] ";
        }
        
        // Add the answer content
        response << answer.content;
        
        // Add reasoning chain for debugging (optional)
        if (!answer.reasoning_chain.empty()) {
            response << " (Reasoning: " << answer.reasoning_chain << ")";
        }
        
        return response.str();
    }
    
    // Extract content from a node ID
    std::string extract_node_content(const melvin::NodeID& node_id) {
        melvin::NodeRecHeader header;
        std::vector<uint8_t> node_data;
        
        if (store_->get_node(node_id, header, node_data)) {
            return std::string(node_data.begin(), node_data.end());
        }
        
        return "[Unknown Node]";
    }
    
    // Identify the type of query for appropriate answer extraction
    std::string identify_query_type(const std::string& query) {
        std::string lower_query = query;
        std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
        
        if (lower_query.find("what is") != std::string::npos || 
            lower_query.find("what are") != std::string::npos) {
            return "what_is";
        } else if (lower_query.find("where") != std::string::npos) {
            return "where";
        } else if (lower_query.find("how") != std::string::npos) {
            return "how";
        } else if (lower_query.find("why") != std::string::npos) {
            return "why";
        } else if (lower_query.find("can") != std::string::npos) {
            return "can";
        } else if (lower_query.find("are") != std::string::npos) {
            return "are";
        } else {
            return "general";
        }
    }
};

// Natural Language Mapper for Melvin's internal concepts
class NaturalLanguageMapper {
private:
    melvin::Store* store_;
    melvin::VMContext* vm_context_;
    
    // Concept to natural language mappings
    std::unordered_map<std::string, std::string> concept_mappings_;
    std::unordered_map<melvin::Rel, std::string> relation_mappings_;
    std::unordered_map<melvin::NodeType, std::string> node_type_mappings_;
    
    // Response templates for different query types
    std::unordered_map<std::string, std::vector<std::string>> response_templates_;
    
    // Confidence indicators
    std::vector<std::string> high_confidence_prefixes_;
    std::vector<std::string> medium_confidence_prefixes_;
    std::vector<std::string> low_confidence_prefixes_;
    
    // Natural language patterns
    struct LanguagePattern {
        std::string pattern;
        std::string replacement;
        float confidence_boost;
    };
    
    std::vector<LanguagePattern> language_patterns_;

public:
    NaturalLanguageMapper(melvin::Store* store, melvin::VMContext* vm_context)
        : store_(store), vm_context_(vm_context) {
        initialize_mappings();
        initialize_templates();
        initialize_patterns();
    }
    
    // Initialize concept mappings
    void initialize_mappings() {
        // Relation to natural language mappings
        relation_mappings_ = {
            {melvin::Rel::ISA, "is a"},
            {melvin::Rel::PART_OF, "is part of"},
            {melvin::Rel::HAS_PROPERTY, "has"},
            {melvin::Rel::SIMILAR_TO, "is similar to"},
            {melvin::Rel::LOCATED_IN, "is located in"},
            {melvin::Rel::CAUSES, "causes"},
            {melvin::Rel::EFFECT, "has effect"},
            {melvin::Rel::DRINKS, "drinks"},
            {melvin::Rel::NEXT, "comes before"},
            {melvin::Rel::CONTRADICTS, "contradicts"},
            {melvin::Rel::GENERATED_FROM, "is generated from"},
            {melvin::Rel::TEMPORAL_BEFORE, "happens before"},
            {melvin::Rel::TEMPORAL_AFTER, "happens after"}
        };
        
        // Node type mappings
        node_type_mappings_ = {
            {melvin::NodeType::SYMBOL, "symbol"},
            {melvin::NodeType::CONCEPT, "concept"},
            {melvin::NodeType::PATH, "path"},
            {melvin::NodeType::META_COG_STEP, "cognitive step"},
            {melvin::NodeType::ABSTRACTION, "abstraction"}
        };
        
        // Common concept mappings
        concept_mappings_ = {
            {"cat", "feline animal"},
            {"dog", "canine animal"},
            {"water", "liquid substance"},
            {"fire", "combustion process"},
            {"tree", "woody plant"},
            {"car", "motorized vehicle"},
            {"house", "dwelling structure"},
            {"book", "written work"},
            {"computer", "electronic device"},
            {"food", "nourishment"},
            {"music", "auditory art"},
            {"color", "visual property"},
            {"size", "dimensional property"},
            {"shape", "geometric form"},
            {"temperature", "thermal property"}
        };
    }
    
    // Initialize response templates
    void initialize_templates() {
        // High confidence prefixes
        high_confidence_prefixes_ = {
            "Based on my knowledge,",
            "I can tell you that",
            "Certainly,",
            "Definitely,"
        };
        
        // Medium confidence prefixes
        medium_confidence_prefixes_ = {
            "I believe that",
            "It seems that",
            "From what I understand,",
            "As far as I know,"
        };
        
        // Low confidence prefixes
        low_confidence_prefixes_ = {
            "I'm not entirely sure, but",
            "It's possible that",
            "I think that",
            "Perhaps"
        };
        
        // Response templates for different query types
        response_templates_ = {
            {"what_is", {
                "{subject} is {answer}",
                "{subject} can be described as {answer}",
                "The answer is that {subject} is {answer}"
            }},
            {"where", {
                "{subject} is located in {answer}",
                "You can find {subject} in {answer}",
                "{subject} is situated in {answer}"
            }},
            {"how", {
                "{subject} works by {answer}",
                "The process involves {answer}",
                "{subject} functions through {answer}"
            }},
            {"why", {
                "{subject} because {answer}",
                "The reason is that {answer}",
                "This happens because {answer}"
            }},
            {"can", {
                "{subject} can {answer}",
                "Yes, {subject} is able to {answer}",
                "{subject} has the capability to {answer}"
            }},
            {"are", {
                "{subject} are {answer}",
                "Yes, {subject} are {answer}",
                "That's correct, {subject} are {answer}"
            }}
        };
    }
    
    // Initialize language patterns
    void initialize_patterns() {
        language_patterns_ = {
            {"is a is a", "is a", 0.1f},
            {"has has", "has", 0.1f},
            {"part of part of", "part of", 0.1f},
            {"similar to similar to", "similar to", 0.1f},
            {"located in located in", "located in", 0.1f}
        };
    }
    
    // Map internal concepts to natural language
    std::string map_concepts_to_natural_language(const std::vector<melvin::Path>& paths, const std::string& query) {
        std::cout << "🗣️ MAPPING INTERNAL CONCEPTS TO NATURAL LANGUAGE\n";
        std::cout << "===============================================\n\n";
        
        if (paths.empty()) {
            return "[No concepts to map]";
        }
        
        std::stringstream natural_language;
        
        // Identify query type for appropriate mapping
        std::string query_type = identify_query_type(query);
        std::cout << "📝 Query type: " << query_type << "\n";
        
        // Process each path
        for (size_t i = 0; i < std::min(size_t(3), paths.size()); ++i) {
            const auto& path = paths[i];
            
            natural_language << "Path " << (i + 1) << " (Score: " << path.score << "):\n";
            
            if (path.edges.empty()) {
                natural_language << "  No connections found\n\n";
                continue;
            }
            
            // Extract and map the reasoning chain
            std::string mapped_chain = map_reasoning_chain(path, query_type);
            natural_language << "  " << mapped_chain << "\n\n";
        }
        
        return natural_language.str();
    }
    
    // Map a single reasoning chain to natural language
    std::string map_reasoning_chain(const melvin::Path& path, const std::string& query_type) {
        if (path.edges.empty()) {
            return "No reasoning chain found";
        }
        
        std::stringstream chain;
        melvin::NodeID first_src;
        // Get the first edge from the path
        melvin::EdgeRec first_edge;
        if (store_->get_edge(path.edges[0], first_edge)) {
            std::copy(first_edge.src, first_edge.src + 32, first_src.begin());
        }
        std::string subject = extract_node_content(first_src);
        
        // Map the starting concept
        subject = map_concept_to_natural_language(subject);
        
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                melvin::NodeID dst_id;
                std::copy(edge.dst, edge.dst + 32, dst_id.begin());
                std::string object = extract_node_content(dst_id);
                melvin::Rel relation = static_cast<melvin::Rel>(edge.rel);
                
                // Map the object concept
                object = map_concept_to_natural_language(object);
                
                // Map the relation
                std::string relation_text = relation_mappings_[relation];
                
                // Build the natural language statement
                chain << subject << " " << relation_text << " " << object;
                
                // Move along the chain
                subject = object;
                break; // For now, just use the first edge
            }
        }
        
        return chain.str();
    }
    
    // Map a single concept to natural language
    std::string map_concept_to_natural_language(const std::string& concept_name) {
        // Check if we have a direct mapping
        auto it = concept_mappings_.find(concept_name);
        if (it != concept_mappings_.end()) {
            return it->second;
        }
        
        // Apply language patterns for improvement
        std::string mapped_concept = concept_name;
        for (const auto& pattern : language_patterns_) {
            size_t pos = 0;
            while ((pos = mapped_concept.find(pattern.pattern, pos)) != std::string::npos) {
                mapped_concept.replace(pos, pattern.pattern.length(), pattern.replacement);
                pos += pattern.replacement.length();
            }
        }
        
        return mapped_concept;
    }
    
    // Generate natural language response from reasoning results
    std::string generate_natural_language_response(const std::vector<melvin::Path>& paths, const std::string& query) {
        std::cout << "📝 GENERATING NATURAL LANGUAGE RESPONSE\n";
        std::cout << "======================================\n\n";
        
        if (paths.empty()) {
            return "I don't have enough information to answer that question.";
        }
        
        // Get the best path
        auto best_path = paths[0]; // Assuming paths are already sorted by score
        
        // Identify query type
        std::string query_type = identify_query_type(query);
        
        // Extract key information from the best path
        melvin::NodeID first_src;
        melvin::EdgeRec first_edge;
        if (store_->get_edge(best_path.edges[0], first_edge)) {
            std::copy(first_edge.src, first_edge.src + 32, first_src.begin());
        }
        std::string subject = extract_node_content(first_src);
        std::string answer = extract_answer_from_path(best_path, query_type);
        
        // Map concepts to natural language
        subject = map_concept_to_natural_language(subject);
        answer = map_concept_to_natural_language(answer);
        
        // Select appropriate template
        std::string template_str = select_response_template(query_type, best_path.confidence);
        
        // Fill in the template
        std::string response = fill_template(template_str, subject, answer, best_path.confidence);
        
        std::cout << "✅ Natural language response generated\n";
        std::cout << "📋 Response: " << response << "\n\n";
        
        return response;
    }
    
    // Extract answer from a path based on query type
    std::string extract_answer_from_path(const melvin::Path& path, const std::string& query_type) {
        if (path.edges.empty()) {
            return "unknown";
        }
        
        // Get the last node in the path as the answer
        auto last_edge = path.edges.back();
        melvin::EdgeRec edge;
        if (store_->get_edge(last_edge, edge)) {
            melvin::NodeID dst_id;
            std::copy(edge.dst, edge.dst + 32, dst_id.begin());
            return extract_node_content(dst_id);
        }
        
        return "unknown";
    }
    
    // Select appropriate response template based on query type and confidence
    std::string select_response_template(const std::string& query_type, float confidence) {
        auto it = response_templates_.find(query_type);
        if (it == response_templates_.end()) {
            return "{subject} is {answer}";
        }
        
        const auto& templates = it->second;
        
        // Select template based on confidence
        if (confidence >= 0.8f) {
            return templates[0]; // Most direct template
        } else if (confidence >= 0.6f) {
            return templates.size() > 1 ? templates[1] : templates[0];
        } else {
            return templates.size() > 2 ? templates[2] : templates[0];
        }
    }
    
    // Fill in the response template
    std::string fill_template(const std::string& template_str, const std::string& subject, const std::string& answer, float confidence) {
        std::string response = template_str;
        
        // Replace placeholders
        size_t pos = 0;
        while ((pos = response.find("{subject}", pos)) != std::string::npos) {
            response.replace(pos, 9, subject);
            pos += subject.length();
        }
        
        pos = 0;
        while ((pos = response.find("{answer}", pos)) != std::string::npos) {
            response.replace(pos, 8, answer);
            pos += answer.length();
        }
        
        // Add confidence prefix
        std::string prefix = get_confidence_prefix(confidence);
        response = prefix + " " + response;
        
        return response;
    }
    
    // Get confidence prefix based on confidence level
    std::string get_confidence_prefix(float confidence) {
        if (confidence >= 0.8f) {
            return high_confidence_prefixes_[0];
        } else if (confidence >= 0.6f) {
            return medium_confidence_prefixes_[0];
        } else {
            return low_confidence_prefixes_[0];
        }
    }
    
    // Identify query type
    std::string identify_query_type(const std::string& query) {
        std::string lower_query = query;
        std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
        
        if (lower_query.find("what is") != std::string::npos || 
            lower_query.find("what are") != std::string::npos) {
            return "what_is";
        } else if (lower_query.find("where") != std::string::npos) {
            return "where";
        } else if (lower_query.find("how") != std::string::npos) {
            return "how";
        } else if (lower_query.find("why") != std::string::npos) {
            return "why";
        } else if (lower_query.find("can") != std::string::npos) {
            return "can";
        } else if (lower_query.find("are") != std::string::npos) {
            return "are";
        } else {
            return "general";
        }
    }
    
    // Extract content from a node ID
    std::string extract_node_content(const melvin::NodeID& node_id) {
        melvin::NodeRecHeader header;
        std::vector<uint8_t> node_data;
        
        if (store_->get_node(node_id, header, node_data)) {
            return std::string(node_data.begin(), node_data.end());
        }
        
        return "[Unknown Node]";
    }
    
    // Add new concept mapping
    void add_concept_mapping(const std::string& concept_name, const std::string& natural_language) {
        concept_mappings_[concept_name] = natural_language;
    }
    
    // Get concept mapping
    std::string get_concept_mapping(const std::string& concept_name) {
        auto it = concept_mappings_.find(concept_name);
        return it != concept_mappings_.end() ? it->second : concept_name;
    }
};

// Answer Selection System for Melvin's reasoning results
class AnswerSelector {
private:
    melvin::Store* store_;
    melvin::VMContext* vm_context_;
    
    // Answer candidate structure
    struct AnswerCandidate {
        std::string content;
        float confidence;
        float path_score;
        size_t path_index;
        std::vector<melvin::Rel> used_relations;
        std::string reasoning_chain;
        std::string query_type;
        float semantic_coherence;
        float relation_strength;
        float path_length_score;
        float novelty_score;
        float consistency_score;
    };
    
    // Selection criteria weights
    struct SelectionWeights {
        float confidence_weight = 0.25f;
        float path_score_weight = 0.20f;
        float semantic_coherence_weight = 0.15f;
        float relation_strength_weight = 0.15f;
        float path_length_weight = 0.10f;
        float novelty_weight = 0.10f;
        float consistency_weight = 0.05f;
    } selection_weights_;
    
    // Query type specific preferences
    std::unordered_map<std::string, std::vector<melvin::Rel>> query_type_preferences_;
    std::unordered_map<std::string, float> query_type_confidence_thresholds_;
    
    // Answer quality metrics
    struct QualityMetrics {
        float min_confidence = 0.3f;
        float min_path_score = 0.2f;
        float max_path_length = 5.0f;
        float min_semantic_coherence = 0.4f;
        float min_relation_strength = 0.3f;
    } quality_metrics_;

public:
    AnswerSelector(melvin::Store* store, melvin::VMContext* vm_context)
        : store_(store), vm_context_(vm_context) {
        initialize_query_preferences();
        initialize_quality_metrics();
    }
    
    // Initialize query type preferences
    void initialize_query_preferences() {
        query_type_preferences_ = {
            {"what_is", {static_cast<melvin::Rel>(0x00), static_cast<melvin::Rel>(0x01)}}, // Exact and Temporal
            {"where", {static_cast<melvin::Rel>(0x00), static_cast<melvin::Rel>(0x01)}}, // Exact and Temporal
            {"how", {static_cast<melvin::Rel>(0x00), static_cast<melvin::Rel>(0x01)}}, // Exact and Temporal
            {"why", {static_cast<melvin::Rel>(0x00), static_cast<melvin::Rel>(0x01)}}, // Exact and Temporal
            {"can", {static_cast<melvin::Rel>(0x00), static_cast<melvin::Rel>(0x01)}}, // Exact and Temporal
            {"are", {static_cast<melvin::Rel>(0x00), static_cast<melvin::Rel>(0x01)}} // Exact and Temporal
        };
        
        query_type_confidence_thresholds_ = {
            {"what_is", 0.6f},
            {"where", 0.7f},
            {"how", 0.5f},
            {"why", 0.6f},
            {"can", 0.5f},
            {"are", 0.6f}
        };
    }
    
    // Initialize quality metrics
    void initialize_quality_metrics() {
        // Quality metrics are already set in the struct
        // These can be adjusted based on system performance
    }
    
    // Select the best answer from reasoning results
    std::string select_best_answer(const std::vector<melvin::Path>& paths, const std::string& query) {
        std::cout << "🎯 SELECTING BEST ANSWER FROM REASONING RESULTS\n";
        std::cout << "==============================================\n\n";
        
        if (paths.empty()) {
            return "[No reasoning results to select from]";
        }
        
        // Identify query type
        std::string query_type = identify_query_type(query);
        std::cout << "📝 Query type: " << query_type << "\n";
        
        // Extract answer candidates from paths
        std::vector<AnswerCandidate> candidates = extract_answer_candidates(paths, query_type);
        
        if (candidates.empty()) {
            return "[No valid answer candidates found]";
        }
        
        std::cout << "📊 Found " << candidates.size() << " answer candidates\n";
        
        // Filter candidates by quality metrics
        std::vector<AnswerCandidate> qualified_candidates = filter_by_quality(candidates, query_type);
        
        if (qualified_candidates.empty()) {
            std::cout << "⚠️ No candidates met quality thresholds, using best available\n";
            qualified_candidates = candidates;
        }
        
        std::cout << "✅ " << qualified_candidates.size() << " candidates passed quality filter\n";
        
        // Score and rank candidates
        std::vector<AnswerCandidate> scored_candidates = score_and_rank_candidates(qualified_candidates, query_type);
        
        // Select the best candidate
        AnswerCandidate best_candidate = scored_candidates[0];
        
        std::cout << "🏆 Selected answer from path " << best_candidate.path_index << "\n";
        std::cout << "📈 Final score: " << calculate_final_score(best_candidate) << "\n";
        std::cout << "🎯 Confidence: " << best_candidate.confidence << "\n";
        std::cout << "🔗 Relations used: " << best_candidate.used_relations.size() << "\n\n";
        
        return best_candidate.content;
    }
    
    // Extract answer candidates from reasoning paths
    std::vector<AnswerCandidate> extract_answer_candidates(const std::vector<melvin::Path>& paths, const std::string& query_type) {
        std::vector<AnswerCandidate> candidates;
        
        for (size_t i = 0; i < paths.size(); ++i) {
            const auto& path = paths[i];
            
            if (path.edges.empty()) {
                continue;
            }
            
            AnswerCandidate candidate;
            candidate.path_index = i;
            candidate.path_score = path.score;
            candidate.confidence = path.confidence;
            candidate.query_type = query_type;
            
            // Extract answer content
            candidate.content = extract_answer_content(path, query_type);
            
            if (candidate.content.empty()) {
                continue;
            }
            
            // Extract reasoning chain
            candidate.reasoning_chain = extract_reasoning_chain(path);
            
            // Extract used relations
            candidate.used_relations = extract_used_relations(path);
            
            // Calculate quality metrics
            candidate.semantic_coherence = calculate_semantic_coherence(path, query_type);
            candidate.relation_strength = calculate_relation_strength(path, query_type);
            candidate.path_length_score = calculate_path_length_score(path);
            candidate.novelty_score = calculate_novelty_score(path);
            candidate.consistency_score = calculate_consistency_score(path);
            
            candidates.push_back(candidate);
        }
        
        return candidates;
    }
    
    // Extract answer content from a path
    std::string extract_answer_content(const melvin::Path& path, const std::string& query_type) {
        if (path.edges.empty()) {
            return "";
        }
        
        // Get preferred relations for this query type
        auto it = query_type_preferences_.find(query_type);
        if (it == query_type_preferences_.end()) {
            // Use general extraction
            return extract_general_answer_content(path);
        }
        
        const auto& preferred_relations = it->second;
        
        // Look for answers using preferred relations
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                melvin::Rel relation = static_cast<melvin::Rel>(edge.rel);
                
                if (std::find(preferred_relations.begin(), preferred_relations.end(), relation) != preferred_relations.end()) {
                    melvin::NodeID dst_id;
                    std::copy(edge.dst, edge.dst + 32, dst_id.begin());
                    std::string object = extract_node_content(dst_id);
                    return object;
                }
            }
        }
        
        // Fallback to general extraction
        return extract_general_answer_content(path);
    }
    
    // Extract general answer content
    std::string extract_general_answer_content(const melvin::Path& path) {
        if (path.edges.empty()) {
            return "";
        }
        
        // Use the last node in the path as the answer
        auto last_edge = path.edges.back();
        melvin::EdgeRec edge;
        if (store_->get_edge(last_edge, edge)) {
            melvin::NodeID dst_id;
            std::copy(edge.dst, edge.dst + 32, dst_id.begin());
            return extract_node_content(dst_id);
        }
        
        return "";
    }
    
    // Extract reasoning chain
    std::string extract_reasoning_chain(const melvin::Path& path) {
        if (path.edges.empty()) {
            return "";
        }
        
        std::stringstream chain;
        melvin::NodeID first_src;
        // Get the first edge from the path
        melvin::EdgeRec first_edge;
        if (store_->get_edge(path.edges[0], first_edge)) {
            std::copy(first_edge.src, first_edge.src + 32, first_src.begin());
        }
        std::string subject = extract_node_content(first_src);
        
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                melvin::NodeID dst_id;
                std::copy(edge.dst, edge.dst + 32, dst_id.begin());
                std::string object = extract_node_content(dst_id);
                melvin::Rel relation = static_cast<melvin::Rel>(edge.rel);
                
                // Use Melvin's own fragments instead of hardcoded relation text
                std::string relation_text = " "; // Just space, let nodes speak for themselves
                
                if (!chain.str().empty()) {
                    chain << " → ";
                }
                chain << subject << " " << relation_text << " " << object;
                
                subject = object;
            }
        }
        
        return chain.str();
    }
    
    // Extract used relations
    std::vector<melvin::Rel> extract_used_relations(const melvin::Path& path) {
        std::vector<melvin::Rel> relations;
        
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                relations.push_back(static_cast<melvin::Rel>(edge.rel));
            }
        }
        
        return relations;
    }
    
    // Calculate semantic coherence score
    float calculate_semantic_coherence(const melvin::Path& path, const std::string& query_type) {
        if (path.edges.empty()) {
            return 0.0f;
        }
        
        // Get preferred relations for this query type
        auto it = query_type_preferences_.find(query_type);
        if (it == query_type_preferences_.end()) {
            return 0.5f; // Default coherence
        }
        
        const auto& preferred_relations = it->second;
        
        // Count how many edges use preferred relations
        int preferred_count = 0;
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                melvin::Rel relation = static_cast<melvin::Rel>(edge.rel);
                if (std::find(preferred_relations.begin(), preferred_relations.end(), relation) != preferred_relations.end()) {
                    preferred_count++;
                }
            }
        }
        
        return static_cast<float>(preferred_count) / path.edges.size();
    }
    
    // Calculate relation strength score
    float calculate_relation_strength(const melvin::Path& path, const std::string& query_type) {
        if (path.edges.empty()) {
            return 0.0f;
        }
        
        float total_strength = 0.0f;
        int edge_count = 0;
        
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                total_strength += edge.w; // Use edge weight as strength indicator
                edge_count++;
            }
        }
        
        return edge_count > 0 ? total_strength / edge_count : 0.0f;
    }
    
    // Calculate path length score (shorter paths are generally better)
    float calculate_path_length_score(const melvin::Path& path) {
        if (path.edges.empty()) {
            return 0.0f;
        }
        
        // Shorter paths get higher scores
        float length = static_cast<float>(path.edges.size());
        return std::max(0.0f, 1.0f - (length - 1) * 0.2f);
    }
    
    // Calculate novelty score
    float calculate_novelty_score(const melvin::Path& path) {
        if (path.edges.empty()) {
            return 0.0f;
        }
        
        // For now, use a simple heuristic based on path score
        // Higher scores might indicate more novel connections
        return std::min(1.0f, path.score * 1.2f);
    }
    
    // Calculate consistency score
    float calculate_consistency_score(const melvin::Path& path) {
        if (path.edges.empty()) {
            return 0.0f;
        }
        
        // Use confidence as a proxy for consistency
        return path.confidence;
    }
    
    // Filter candidates by quality metrics
    std::vector<AnswerCandidate> filter_by_quality(const std::vector<AnswerCandidate>& candidates, const std::string& query_type) {
        std::vector<AnswerCandidate> qualified;
        
        // Get confidence threshold for this query type
        float threshold = query_type_confidence_thresholds_[query_type];
        
        for (const auto& candidate : candidates) {
            bool passes_filter = true;
            
            // Check confidence threshold
            if (candidate.confidence < threshold) {
                passes_filter = false;
            }
            
            // Check path score
            if (candidate.path_score < quality_metrics_.min_path_score) {
                passes_filter = false;
            }
            
            // Check path length
            if (candidate.used_relations.size() > quality_metrics_.max_path_length) {
                passes_filter = false;
            }
            
            // Check semantic coherence
            if (candidate.semantic_coherence < quality_metrics_.min_semantic_coherence) {
                passes_filter = false;
            }
            
            // Check relation strength
            if (candidate.relation_strength < quality_metrics_.min_relation_strength) {
                passes_filter = false;
            }
            
            if (passes_filter) {
                qualified.push_back(candidate);
            }
        }
        
        return qualified;
    }
    
    // Score and rank candidates
    std::vector<AnswerCandidate> score_and_rank_candidates(const std::vector<AnswerCandidate>& candidates, const std::string& query_type) {
        std::vector<AnswerCandidate> scored_candidates = candidates;
        
        // Calculate final scores for each candidate
        for (auto& candidate : scored_candidates) {
            float final_score = calculate_final_score(candidate);
            // Store the final score in the confidence field for ranking
            candidate.confidence = final_score;
        }
        
        // Sort by final score (descending)
        std::sort(scored_candidates.begin(), scored_candidates.end(),
            [](const AnswerCandidate& a, const AnswerCandidate& b) {
                return a.confidence > b.confidence;
            });
        
        return scored_candidates;
    }
    
    // Calculate final score for a candidate
    float calculate_final_score(const AnswerCandidate& candidate) {
        float score = 0.0f;
        
        score += candidate.confidence * selection_weights_.confidence_weight;
        score += candidate.path_score * selection_weights_.path_score_weight;
        score += candidate.semantic_coherence * selection_weights_.semantic_coherence_weight;
        score += candidate.relation_strength * selection_weights_.relation_strength_weight;
        score += candidate.path_length_score * selection_weights_.path_length_weight;
        score += candidate.novelty_score * selection_weights_.novelty_weight;
        score += candidate.consistency_score * selection_weights_.consistency_weight;
        
        return std::clamp(score, 0.0f, 1.0f);
    }
    
    // Identify query type
    std::string identify_query_type(const std::string& query) {
        std::string lower_query = query;
        std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
        
        if (lower_query.find("what is") != std::string::npos || 
            lower_query.find("what are") != std::string::npos) {
            return "what_is";
        } else if (lower_query.find("where") != std::string::npos) {
            return "where";
        } else if (lower_query.find("how") != std::string::npos) {
            return "how";
        } else if (lower_query.find("why") != std::string::npos) {
            return "why";
        } else if (lower_query.find("can") != std::string::npos) {
            return "can";
        } else if (lower_query.find("are") != std::string::npos) {
            return "are";
        } else {
            return "general";
        }
    }
    
    // Render path using Melvin's own node fragments (no hardcoded text)
    std::string render_path_from_memory(const std::vector<melvin::NodeID>& path_nodes, const std::vector<uint8_t>& relations) {
        if (path_nodes.empty()) {
            return "";
        }
        
        std::stringstream output;
        
        for (size_t i = 0; i < path_nodes.size(); ++i) {
            // Get Melvin's own stored fragment for this node
            std::string fragment = extract_node_content(path_nodes[i]);
            output << fragment;
            
            // Add spacing/uncertainty based on relation to next node
            if (i < path_nodes.size() - 1 && i < relations.size()) {
                uint8_t rel_code = relations[i] & 0x03;
                
                switch (rel_code) {
                    case 0x00: // Exact - collapse duplicates, just space
                        output << " ";
                        break;
                    case 0x01: // Temporal - sequence in order, space
                        output << " ";
                        break;
                    case 0x02: // Leap - mark uncertainty
                        output << " ??? ";
                        break;
                    case 0x03: // Reserved - default spacing
                        output << " ";
                        break;
                    default:
                        output << " ";
                        break;
                }
            }
        }
        
        return output.str();
    }
    
    // Traverse reasoning chain and collect node fragments
    std::vector<melvin::NodeID> collect_path_nodes(const melvin::Path& path) {
        std::vector<melvin::NodeID> nodes;
        
        if (path.edges.empty()) {
            return nodes;
        }
        
        // Get source node from first edge
        melvin::EdgeRec first_edge;
        if (store_->get_edge(path.edges[0], first_edge)) {
            melvin::NodeID src_id;
            std::copy(first_edge.src, first_edge.src + 32, src_id.begin());
            nodes.push_back(src_id);
        }
        
        // Add destination nodes from edges
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                melvin::NodeID dst_id;
                std::copy(edge.dst, edge.dst + 32, dst_id.begin());
                nodes.push_back(dst_id);
            }
        }
        
        return nodes;
    }
    
    // Extract relation codes from path edges
    std::vector<uint8_t> collect_path_relations(const melvin::Path& path) {
        std::vector<uint8_t> relations;
        
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                relations.push_back(static_cast<uint8_t>(edge.rel));
            }
        }
        
        return relations;
    }
    
    // Render complete reasoning chain using Melvin's memory
    std::string render_reasoning_chain(const melvin::Path& path) {
        auto nodes = collect_path_nodes(path);
        auto relations = collect_path_relations(path);
        
        return render_path_from_memory(nodes, relations);
    }
    
    // Extract content from a node ID
    std::string extract_node_content(const melvin::NodeID& node_id) {
        melvin::NodeRecHeader header;
        std::vector<uint8_t> node_data;
        
        if (store_->get_node(node_id, header, node_data)) {
            return std::string(node_data.begin(), node_data.end());
        }
        
        return "[Unknown Node]";
    }
};

// Response Formatter for Melvin's reasoning results
class ResponseFormatter {
private:
    melvin::Store* store_;
    melvin::VMContext* vm_context_;
    
    // Response formatting options
    struct FormattingOptions {
        bool include_confidence = true;
        bool include_reasoning_chain = false;
        bool include_alternatives = false;
        bool use_natural_language = true;
        bool add_qualifiers = true;
        int max_alternatives = 3;
        float confidence_threshold = 0.3f;
    } formatting_options_;
    
    // Response styles
    enum class ResponseStyle {
        CONVERSATIONAL,
        FORMAL,
        TECHNICAL,
        BRIEF,
        DETAILED
    } response_style_;
    
    // Pure node-based system - Melvin creates and connects dynamically

public:
    ResponseFormatter(melvin::Store* store, melvin::VMContext* vm_context)
        : store_(store), vm_context_(vm_context) {
        initialize_question_type_nodes();
        response_style_ = ResponseStyle::CONVERSATIONAL;
    }
    
    // Let Melvin create and connect nodes dynamically - no predefined question types
    void initialize_question_type_nodes() {
        // Melvin will create nodes and connections as needed through interaction
        // No hardcoded question types or patterns
        std::cout << "✅ Melvin ready to create and connect nodes dynamically\n";
    }
    
    // Format response dynamically using node connections
    std::string format_response_appropriately(const std::string& answer, const std::string& query, float confidence) {
        std::cout << "📝 DYNAMIC RESPONSE FORMATTING\n";
        std::cout << "==============================\n\n";
        
        if (answer.empty()) {
            return generate_dynamic_no_answer_response(query, confidence);
        }
        
        // Let Melvin create and connect nodes dynamically - no query type identification
        std::string response = generate_dynamic_response(answer, query, "", confidence);
        
        // Check if response was blocked due to low confidence
        if (response.empty()) {
            std::cout << "🚫 Response blocked due to low confidence\n";
            return generate_dynamic_no_answer_response(query, confidence);
        }
        
        // Apply response style
        response = apply_response_style(response);
        
        std::cout << "✅ Dynamic response generated successfully\n";
        std::cout << "📋 Final response: " << response << "\n\n";
        
        return response;
    }
    
    // Generate dynamic no answer response using relative confidence
    std::string generate_dynamic_no_answer_response(const std::string& query, float confidence) {
        // Let Melvin create nodes and connections dynamically
        // Use relative confidence based on available connections
        
        // Calculate relative confidence based on what connections exist
        float relative_confidence = calculate_relative_confidence(query, "");
        
        std::stringstream response;
        
        // Dynamic uncertainty based on relative connection strength
        if (relative_confidence < 0.3f) {
            response << "I'm not sure about that. ";
        } else if (relative_confidence < 0.6f) {
            response << "I think ";
        }
        
        response << "I don't have enough information to answer that.";
        
        return response.str();
    }
    
    // Generate dynamic response using relative confidence and context
    std::string generate_dynamic_response(const std::string& answer, const std::string& query, const std::string& query_type, float confidence) {
        // Let Melvin create nodes and connections dynamically
        // Confidence is now relative to other connections and context-aware
        
        // Calculate relative confidence based on node connections and context
        float relative_confidence = calculate_relative_confidence(query, answer);
        
        // Let Melvin's dynamic confidence system handle low confidence naturally
        // No hard blocking - let the strongest path determine the response
        
        std::stringstream response;
        
        // Let Melvin's dynamic confidence system determine the response style
        // No hard-coded thresholds - follow the strongest relative path
        
        // Return the answer - confidence is now context-aware and relative
        response << answer;
        
        return response.str();
    }
    
    // Dynamic confidence handling - no rigid qualifiers
    std::string add_dynamic_confidence(const std::string& response, float confidence) {
        // Confidence is already handled in generate_dynamic_response
        return response;
    }
    
    // Apply response style
    std::string apply_response_style(const std::string& response) {
        switch (response_style_) {
            case ResponseStyle::CONVERSATIONAL:
                return make_conversational(response);
            case ResponseStyle::FORMAL:
                return make_formal(response);
            case ResponseStyle::TECHNICAL:
                return make_technical(response);
            case ResponseStyle::BRIEF:
                return make_brief(response);
            case ResponseStyle::DETAILED:
                return make_detailed(response);
            default:
                return response;
        }
    }
    
    // Make response conversational
    std::string make_conversational(const std::string& response) {
        // Add conversational elements
        std::string conversational = response;
        
        // Replace formal phrases with conversational ones
        size_t pos = 0;
        while ((pos = conversational.find("Based on my knowledge,", pos)) != std::string::npos) {
            conversational.replace(pos, 22, "Well,");
            pos += 5;
        }
        
        return conversational;
    }
    
    // Make response formal
    std::string make_formal(const std::string& response) {
        // Add formal elements
        std::string formal = response;
        
        // Replace informal phrases with formal ones
        size_t pos = 0;
        while ((pos = formal.find("Well,", pos)) != std::string::npos) {
            formal.replace(pos, 5, "Based on my knowledge,");
            pos += 22;
        }
        
        return formal;
    }
    
    // Make response technical
    std::string make_technical(const std::string& response) {
        // Add technical elements
        std::string technical = response;
        
        // Add technical qualifiers
        if (technical.find("is") != std::string::npos) {
            technical += " (from a technical perspective)";
        }
        
        return technical;
    }
    
    // Make response brief
    std::string make_brief(const std::string& response) {
        // Remove unnecessary words and phrases for brief style
        std::string brief = response;
        
        // Remove common confidence prefixes
        std::vector<std::string> prefixes = {"I believe ", "It seems ", "I think "};
        for (const auto& prefix : prefixes) {
            size_t pos = 0;
            while ((pos = brief.find(prefix, pos)) != std::string::npos) {
                brief.erase(pos, prefix.length());
                pos = 0; // Reset position after erasure
            }
        }
        
        return brief;
    }
    
    // Make response detailed
    std::string make_detailed(const std::string& response) {
        // Add more detail and explanation
        std::string detailed = response;
        
        // Add explanatory phrases
        if (detailed.find("is") != std::string::npos) {
            detailed += " This is based on the available information.";
        }
        
        return detailed;
    }
    
    // Get dynamic confidence prefix based on confidence level
    std::string get_dynamic_confidence_prefix(float confidence) {
        if (confidence >= 0.8f) {
            return "";
        } else if (confidence >= 0.6f) {
            return "I believe ";
        } else if (confidence >= 0.4f) {
            return "It seems ";
        } else {
            return "I think ";
        }
    }
    
    // Calculate relative confidence based on node connections and context
    float calculate_relative_confidence(const std::string& query, const std::string& answer) {
        // Create nodes for query and answer if they don't exist
        auto query_nodes = create_or_find_nodes(query);
        auto answer_nodes = create_or_find_nodes(answer);
        
        if (query_nodes.empty()) {
            return 0.0f; // No connections possible
        }
        
        // Find all connections from query nodes
        std::vector<float> connection_strengths;
        for (const auto& query_node : query_nodes) {
            auto connections = get_node_connections(query_node);
            for (const auto& connection : connections) {
                connection_strengths.push_back(connection.strength);
            }
        }
        
        if (connection_strengths.empty()) {
            return 0.0f; // No connections found
        }
        
        // Find the strongest connection to answer nodes
        float max_connection = 0.0f;
        for (const auto& query_node : query_nodes) {
            for (const auto& answer_node : answer_nodes) {
                float connection_strength = get_connection_strength(query_node, answer_node);
                max_connection = std::max(max_connection, connection_strength);
            }
        }
        
        // Calculate relative confidence
        // If max_connection is significantly higher than others, it's high confidence
        // If it's similar to others, it's medium confidence
        // If it's lower than others, it's low confidence
        
        std::sort(connection_strengths.begin(), connection_strengths.end(), std::greater<float>());
        
        if (connection_strengths.size() == 1) {
            return max_connection; // Only one connection
        }
        
        // Calculate relative strength
        float second_best = connection_strengths[1];
        float relative_strength = max_connection / (max_connection + second_best + 0.1f);
        
        // Boost confidence if answer nodes are in the input context
        if (is_in_input_context(answer_nodes, query)) {
            relative_strength = std::min(1.0f, relative_strength * 1.5f);
        }
        
        return relative_strength;
    }
    
    // Create or find nodes for text
    std::vector<melvin::NodeID> create_or_find_nodes(const std::string& text) {
        std::vector<melvin::NodeID> nodes;
        
        // Split text into words and create/find nodes for each
        std::istringstream iss(text);
        std::string word;
        while (iss >> word) {
            // Convert to lowercase
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            
            // Create or find node for this word
            melvin::NodeRecHeader header;
            header.type = static_cast<uint32_t>(melvin::NodeType::SYMBOL);
            header.flags = 0;
            header.payload_len = static_cast<uint32_t>(word.size());
            
            std::vector<uint8_t> payload(word.begin(), word.end());
            melvin::NodeID node_id = store_->upsert_node(header, payload);
            
            if (!melvin::is_zero_id(node_id)) {
                nodes.push_back(node_id);
            }
        }
        
        return nodes;
    }
    
    // Get connections from a node
    struct Connection {
        melvin::NodeID target;
        float strength;
        melvin::Rel relation;
    };
    
    std::vector<Connection> get_node_connections(const melvin::NodeID& node) {
        std::vector<Connection> connections;
        
        // Get outgoing edges with safety checks
        melvin::AdjView view;
        if (store_ && store_->get_out_edges(node, melvin::RelMask{}, view)) {
            // Safety check for valid view
            if (view.edges && view.count > 0) {
                for (size_t i = 0; i < view.count; ++i) {
                    // Additional safety check for valid edge
                    if (view.edges[i].dst) {
                        Connection conn;
                        std::copy(view.edges[i].dst, view.edges[i].dst + 32, conn.target.begin());
                        conn.strength = view.edges[i].w_ctx; // Use context weight as strength
                        conn.relation = static_cast<melvin::Rel>(view.edges[i].rel);
                        connections.push_back(conn);
                    }
                }
            }
        }
        
        return connections;
    }
    
    // Get connection strength between two nodes
    float get_connection_strength(const melvin::NodeID& src, const melvin::NodeID& dst) {
        // Check for direct connection with safety checks
        melvin::AdjView view;
        if (store_ && store_->get_out_edges(src, melvin::RelMask{}, view)) {
            // Safety check for valid view
            if (view.edges && view.count > 0) {
                for (size_t i = 0; i < view.count; ++i) {
                    // Additional safety check for valid edge
                    if (view.edges[i].dst) {
                        // Compare NodeIDs properly
                        bool match = true;
                        for (size_t j = 0; j < 32; ++j) {
                            if (view.edges[i].dst[j] != dst[j]) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            return view.edges[i].w_ctx; // Return context weight
                        }
                    }
                }
            }
        }
        
        return 0.0f; // No direct connection
    }
    
    // Check if nodes are in input context
    bool is_in_input_context(const std::vector<melvin::NodeID>& nodes, const std::string& input) {
        // Simple check - if any of the node content appears in the input
        for (const auto& node : nodes) {
            melvin::NodeRecHeader header;
            std::vector<uint8_t> payload;
            if (store_->get_node(node, header, payload)) {
                std::string content(payload.begin(), payload.end());
                std::transform(content.begin(), content.end(), content.begin(), ::tolower);
                
                std::string lower_input = input;
                std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
                
                if (lower_input.find(content) != std::string::npos) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    // Set formatting options
    void set_formatting_options(const FormattingOptions& options) {
        formatting_options_ = options;
    }
    
    // Set response style
    void set_response_style(ResponseStyle style) {
        response_style_ = style;
    }
    
    // Get current formatting options
    FormattingOptions get_formatting_options() const {
        return formatting_options_;
    }
    
    // Get current response style
    ResponseStyle get_response_style() const {
        return response_style_;
    }
};

// Unified Reasoning System that integrates all components
class UnifiedReasoningSystem {
private:
    melvin::Store* store_;
    melvin::VMContext* vm_context_;
    
    // Component instances
    std::unique_ptr<ReasoningPathParser> path_parser_;
    std::unique_ptr<NaturalLanguageMapper> language_mapper_;
    std::unique_ptr<AnswerSelector> answer_selector_;
    std::unique_ptr<ResponseFormatter> response_formatter_;
    
    // System configuration
    struct SystemConfig {
        bool enable_debug_output = true;
        bool enable_confidence_scoring = true;
        bool enable_alternative_answers = false;
        bool enable_reasoning_chains = false;
        int max_reasoning_depth = 4;
        int beam_size = 30;
        float min_confidence_threshold = 0.3f;
    } config_;

public:
    UnifiedReasoningSystem(melvin::Store* store, melvin::VMContext* vm_context)
        : store_(store), vm_context_(vm_context) {
        initialize_components();
    }
    
    // Initialize all components
    void initialize_components() {
        path_parser_ = std::make_unique<ReasoningPathParser>(store_, vm_context_);
        language_mapper_ = std::make_unique<NaturalLanguageMapper>(store_, vm_context_);
        answer_selector_ = std::make_unique<AnswerSelector>(store_, vm_context_);
        response_formatter_ = std::make_unique<ResponseFormatter>(store_, vm_context_);
    }
    
    // Main processing function that integrates all components
    std::string process_query(const std::string& query) {
        std::cout << "🧠 UNIFIED REASONING SYSTEM PROCESSING\n";
        std::cout << "=====================================\n\n";
        std::cout << "📝 Query: " << query << "\n\n";
        
        try {
            // Step 1: Generate reasoning paths
            std::cout << "🔍 STEP 1: GENERATING REASONING PATHS\n";
            std::cout << "====================================\n";
            
            auto paths = generate_reasoning_paths(query);
            if (paths.empty()) {
                return handle_no_paths_found(query);
            }
            
            std::cout << "✅ Generated " << paths.size() << " reasoning paths\n\n";
            
            // Step 2: Parse reasoning paths to extract answers
            std::cout << "🔍 STEP 2: PARSING REASONING PATHS\n";
            std::cout << "==================================\n";
            
            std::string extracted_answer = path_parser_->parse_reasoning_path(paths, query);
            std::cout << "✅ Answer extracted: " << extracted_answer << "\n\n";
            
            // Step 3: Map internal concepts to natural language
            std::cout << "🔍 STEP 3: MAPPING CONCEPTS TO NATURAL LANGUAGE\n";
            std::cout << "==============================================\n";
            
            std::string natural_language = language_mapper_->map_concepts_to_natural_language(paths, query);
            
            if (config_.enable_debug_output) {
                std::cout << natural_language;
            }
            
            // Step 4: Select the best answer
            std::cout << "🔍 STEP 4: SELECTING BEST ANSWER\n";
            std::cout << "===============================\n";
            
            std::string best_answer = answer_selector_->select_best_answer(paths, query);
            std::cout << "✅ Best answer selected: " << best_answer << "\n\n";
            
            // Step 5: Format the response appropriately
            std::cout << "🔍 STEP 5: FORMATTING RESPONSE\n";
            std::cout << "==============================\n";
            
            std::string formatted_response = format_final_response(best_answer, query, paths);
            std::cout << "✅ Response formatted successfully\n\n";
            
            std::cout << "🎉 PROCESSING COMPLETE\n";
            std::cout << "=====================\n";
            std::cout << "📊 Final response: " << formatted_response << "\n\n";
            
            return formatted_response;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error processing query: " << e.what() << "\n\n";
            return "[Error] Unable to process query: " + std::string(e.what());
        }
    }
    
    // Generate reasoning paths using beam search
    std::vector<melvin::Path> generate_reasoning_paths(const std::string& query) {
        // Clean and tokenize query
        std::string clean_query = query;
        std::transform(clean_query.begin(), clean_query.end(), clean_query.begin(), ::tolower);
        
        std::istringstream iss(clean_query);
        std::vector<std::string> query_words;
        std::string word;
        while (iss >> word) {
            // Remove punctuation
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            if (!word.empty()) {
                query_words.push_back(word);
            }
        }
        
        // Find starting node - prioritize meaningful words over function words
        melvin::NodeID start_node;
        std::string start_word;
        
        // Function words to avoid as starting points
        std::unordered_set<std::string> function_words = {
            "what", "are", "is", "do", "does", "can", "will", "would", "could", "should",
            "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by"
        };
        
        // First pass: look for meaningful words (not function words)
        for (const auto& word : query_words) {
            if (function_words.find(word) == function_words.end()) {
                std::vector<uint8_t> payload(word.begin(), word.end());
                melvin::NodeID node = {};  // Simplified for now
                
                melvin::NodeRecHeader header;
                std::vector<uint8_t> node_data;
                if (store_->get_node(node, header, node_data)) {
                    start_node = node;
                    start_word = word;
                    break;
                }
            }
        }
        
        // Second pass: if no meaningful word found, try any word
        if (start_node.empty()) {
            for (const auto& word : query_words) {
                std::vector<uint8_t> payload(word.begin(), word.end());
                melvin::NodeID node = {};  // Simplified for now
                
                melvin::NodeRecHeader header;
                std::vector<uint8_t> node_data;
                if (store_->get_node(node, header, node_data)) {
                    start_node = node;
                    start_word = word;
                    break;
                }
            }
        }
        
        if (start_node.empty()) {
            std::cout << "❌ No starting node found for query\n";
            return {};
        }
        
        std::cout << "🔍 Starting from node: " << start_word << "\n";
        
        // Perform beam search with three broad relationship categories
        melvin::RelMask mask;
        mask.set(static_cast<melvin::Rel>(0x00)); // EXACT - collapse duplicates
        mask.set(static_cast<melvin::Rel>(0x01)); // TEMPORAL - sequence in order
        mask.set(static_cast<melvin::Rel>(0x02)); // LEAP - mark uncertainty
        
        vm_context_->init_beam(config_.beam_size, config_.max_reasoning_depth);
        auto paths = vm_context_->beam_search(start_node, mask, config_.max_reasoning_depth, config_.beam_size);
        
        return paths;
    }
    
    // Format the final response
    std::string format_final_response(const std::string& answer, const std::string& query, const std::vector<melvin::Path>& paths) {
        if (paths.empty()) {
            return response_formatter_->generate_dynamic_no_answer_response(query, 0.0f);
        }
        
        // Calculate average confidence
        float total_confidence = 0.0f;
        for (const auto& path : paths) {
            total_confidence += path.confidence;
        }
        float avg_confidence = total_confidence / paths.size();
        
        // Format response based on configuration
        if (config_.enable_alternative_answers && paths.size() > 1) {
            std::vector<std::string> alternatives;
            std::vector<float> confidences;
            
            for (size_t i = 0; i < std::min(size_t(3), paths.size()); ++i) {
                alternatives.push_back(answer);
                confidences.push_back(paths[i].confidence);
            }
            
            // For now, just use the basic formatter
            return response_formatter_->format_response_appropriately(answer, query, avg_confidence);
        } else if (config_.enable_reasoning_chains) {
            std::string reasoning_chain = generate_reasoning_chain(paths[0]);
            // For now, just use the basic formatter
            return response_formatter_->format_response_appropriately(answer, query, avg_confidence);
        } else {
            return response_formatter_->format_response_appropriately(answer, query, avg_confidence);
        }
    }
    
    // Generate reasoning chain from path
    std::string generate_reasoning_chain(const melvin::Path& path) {
        if (path.edges.empty()) {
            return "";
        }
        
        std::stringstream chain;
        melvin::NodeID first_src;
        // Get the first edge from the path
        melvin::EdgeRec first_edge;
        if (store_->get_edge(path.edges[0], first_edge)) {
            std::copy(first_edge.src, first_edge.src + 32, first_src.begin());
        }
        std::string subject = extract_node_content(first_src);
        
        for (const auto& edge_id : path.edges) {
            melvin::EdgeRec edge;
            if (store_->get_edge(edge_id, edge)) {
                melvin::NodeID dst_id;
                std::copy(edge.dst, edge.dst + 32, dst_id.begin());
                std::string object = extract_node_content(dst_id);
                melvin::Rel relation = static_cast<melvin::Rel>(edge.rel);
                
                // Use Melvin's own fragments instead of hardcoded relation text
                std::string relation_text = " "; // Just space, let nodes speak for themselves
                
                if (!chain.str().empty()) {
                    chain << " → ";
                }
                chain << subject << " " << relation_text << " " << object;
                
                subject = object;
            }
        }
        
        return chain.str();
    }
    
    // Handle case when no paths are found
    std::string handle_no_paths_found(const std::string& query) {
        std::cout << "❌ No reasoning paths found for query\n";
        return response_formatter_->generate_dynamic_no_answer_response(query, 0.0f);
    }
    
    // Extract content from a node ID
    std::string extract_node_content(const melvin::NodeID& node_id) {
        melvin::NodeRecHeader header;
        std::vector<uint8_t> node_data;
        
        if (store_->get_node(node_id, header, node_data)) {
            return std::string(node_data.begin(), node_data.end());
        }
        
        return "[Unknown Node]";
    }
    
    // Render path using Melvin's own node fragments (no hardcoded text)
    std::string render_path_from_memory(const std::vector<melvin::NodeID>& path_nodes, const std::vector<uint8_t>& relations) {
        if (path_nodes.empty()) {
            return "";
        }
        
        std::stringstream output;
        
        for (size_t i = 0; i < path_nodes.size(); ++i) {
            // Get Melvin's own stored fragment for this node
            std::string fragment = extract_node_content(path_nodes[i]);
            output << fragment;
            
            // Add spacing/uncertainty based on relation to next node
            if (i < path_nodes.size() - 1 && i < relations.size()) {
                uint8_t rel_code = relations[i] & 0x03;
                
                switch (rel_code) {
                    case 0x00: // Exact - collapse duplicates, just space
                        output << " ";
                        break;
                    case 0x01: // Temporal - sequence in order, space
                        output << " ";
                        break;
                    case 0x02: // Leap - mark uncertainty
                        output << " ??? ";
                        break;
                    case 0x03: // Reserved - default spacing
                        output << " ";
                        break;
                    default:
                        output << " ";
                        break;
                }
            }
        }
        
        return output.str();
    }
    
    // Configuration methods
    void set_debug_output(bool enable) {
        config_.enable_debug_output = enable;
    }
    
    void set_confidence_scoring(bool enable) {
        config_.enable_confidence_scoring = enable;
    }
    
    void set_alternative_answers(bool enable) {
        config_.enable_alternative_answers = enable;
    }
    
    void set_reasoning_chains(bool enable) {
        config_.enable_reasoning_chains = enable;
    }
    
    void set_max_reasoning_depth(int depth) {
        config_.max_reasoning_depth = depth;
    }
    
    void set_beam_size(int size) {
        config_.beam_size = size;
    }
    
    void set_min_confidence_threshold(float threshold) {
        config_.min_confidence_threshold = threshold;
    }
};

class Melvin {
private:
    std::unique_ptr<melvin::Store> store_;
    std::unique_ptr<melvin::VM> vm_;
    std::unique_ptr<melvin::VMContext> vm_context_;
    std::unique_ptr<UnifiedReasoningSystem> reasoning_system_;
    std::unique_ptr<melvin::ConsolidationSystem> consolidation_system_;
    std::unique_ptr<melvin::HealthMetrics> health_metrics_;
    melvin::Drivers drivers_;
    uint64_t session_seed_;
    
    // Configuration with suggested defaults
    float tau_mid_ = 0.01f;      // τ_mid (very low for best guessing)
    float tau_high_ = 0.05f;     // τ_high (very low for best guessing)
    float beta_ctx_ = 0.10f;     // β_ctx (fast decay)
    float beta_core_ = 0.01f;    // β_core (slow decay)
    uint32_t k_support_ = 6;     // k_support
    uint32_t delta_t_ = 2;       // Δt (stability window)
    float epsilon_ = 0.02f;       // ε (GC threshold)
    uint32_t T_sleeps_ = 3;      // T (GC age threshold)
    
    // Beam search defaults
    size_t beam_k_ = 32;         // beam_k
    size_t max_depth_ = 3;       // max_depth
    float coverage_bonus_ = 1.1f; // coverage_bonus
    float continuity_bonus_ = 1.1f; // continuity_bonus
    
public:
    Melvin(const std::string& store_dir = "melvin_memory") 
        : session_seed_(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count()) {
        
        // Initialize store with persistent storage by default
        store_ = melvin::open_store(std::string(store_dir));
        if (!store_) {
            throw std::runtime_error("Failed to open store");
        }
        
        // Initialize VM
        vm_ = create_vm(store_.get(), session_seed_);
        if (!vm_) {
            throw std::runtime_error("Failed to create VM");
        }
        
        // Initialize VMContext for reasoning
        vm_context_ = std::make_unique<melvin::VMContext>(store_.get(), session_seed_);
        
        // Set initial drivers
        drivers_ = melvin::Drivers(0.7f, 0.5f, 0.6f, 0.4f, 0.8f);
        vm_->set_drivers(drivers_);
        
        // Initialize unified reasoning system
        reasoning_system_ = std::make_unique<UnifiedReasoningSystem>(store_.get(), vm_context_.get());
        
        // Initialize consolidation system
        consolidation_system_ = std::make_unique<melvin::ConsolidationSystem>(store_.get());
        consolidation_system_->set_decay_params(beta_ctx_, beta_core_);
        consolidation_system_->set_thresholds(tau_mid_, tau_high_);
        consolidation_system_->set_gc_params(epsilon_, T_sleeps_);
        
        // Initialize health metrics
        health_metrics_ = std::make_unique<melvin::HealthMetrics>(store_.get());
        
        // Configure thresholds
        store_->set_thresholds(tau_mid_, tau_high_);
        store_->set_decay_params(beta_ctx_, beta_core_);
    }
    
    ~Melvin() = default;
    
    // Core reasoning interface
    std::string reason(const std::string& query) {
        // Use the new unified reasoning system
        return reasoning_system_->process_query(query);
    }
    
    // Learning interface - enhanced to create more nodes and connections
    void learn(const std::string& text) {
        auto nodes = ingest_text(text);
        reinforce_path(nodes);
        
        // Create semantic relationships
        create_semantic_relationships(nodes, text);
        
        // Create additional conceptual connections for stronger reasoning paths
        create_conceptual_connections(nodes, text);
        
        // Create reverse relationships for bidirectional reasoning
        create_reverse_relationships(nodes, text);
    }
    
    // Maintenance interface
    void decay_pass() {
        consolidation_system_->decay_pass();
    }
    
    void run_nightly_consolidation() {
        consolidation_system_->run_consolidation();
        
        // Update health metrics with consolidation results
        health_metrics_->update_consolidation_metrics(consolidation_system_->get_stats());
    }
    
    void compact() {
        consolidation_system_->compact_and_snapshot();
    }
    
    // Health monitoring
    float get_health_score() const {
        return health_metrics_->calculate_health_score();
    }
    
    void export_metrics(const std::string& filename) const {
        health_metrics_->export_to_csv(filename);
    }
    
    // Statistics
    size_t node_count() const { return store_->node_count(); }
    size_t edge_count() const { return store_->edge_count(); }
    size_t path_count() const { return store_->path_count(); }
    size_t vm_steps() const { return vm_->steps_executed(); }
    size_t memory_usage() const { return vm_->memory_usage(); }
    
    // Configuration
    void set_drivers(const melvin::Drivers& drivers) {
        drivers_ = drivers;
        vm_->set_drivers(drivers_);
        
        // Update reasoning engine with new drivers
        // reasoning_engine_ is now replaced by reasoning_system_
    }
    
    void set_thresholds(float tau_mid, float tau_high) {
        tau_mid_ = tau_mid;
        tau_high_ = tau_high;
        store_->set_thresholds(tau_mid_, tau_high_);
    }
    
    void set_decay_params(float beta_ctx, float beta_core) {
        beta_ctx_ = beta_ctx;
        beta_core_ = beta_core;
        store_->set_decay_params(beta_ctx_, beta_core_);
    }
    
    // Test data setup for acceptance tests
    void setup_test_data() {
        // Add test knowledge: cats ISA mammal, mammals DRINK water
        add_knowledge("cats", "mammal", static_cast<melvin::Rel>(0x00)); // Exact relation
        add_knowledge("mammals", "water", static_cast<melvin::Rel>(0x00)); // Exact relation
        add_knowledge("dogs", "water", static_cast<melvin::Rel>(0x00)); // Exact relation
        
        // Reinforce these edges
        reinforce_test_edges();
    }
    
private:
    std::vector<melvin::NodeID> ingest_text(const std::string& text) {
        std::vector<melvin::NodeID> nodes;
        
        // Simple tokenization
        std::istringstream iss(text);
        std::string token;
        melvin::NodeID prev_node;
        
        while (iss >> token) {
            // Normalize token
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            
            // Create symbol node
            melvin::NodeRecHeader header;
            header.type = static_cast<uint32_t>(melvin::NodeType::SYMBOL);
            header.flags = 0;
            header.payload_len = static_cast<uint32_t>(token.size());
            
            std::vector<uint8_t> payload(token.begin(), token.end());
            melvin::NodeID node_id = store_->upsert_node(header, payload);
            nodes.push_back(node_id);
            
            // Create temporal edge if we have a previous node
            if (!melvin::is_zero_id(prev_node)) {
                melvin::EdgeRec edge;
                std::copy(prev_node.begin(), prev_node.end(), edge.src);
                edge.rel = 0x01; // Temporal relation (01)
                std::copy(node_id.begin(), node_id.end(), edge.dst);
                // Apply size-relative scaling to initial weights
                float size_scaling = 1.0f; // TODO: implement size scaling;
                float base_weight = 0.5f * size_scaling;
                
                edge.w = std::clamp(base_weight, 0.0f, 1.0f);
                edge.w_core = std::clamp(base_weight * 0.6f, 0.0f, 1.0f); // 0.3f scaled
                edge.w_ctx = std::clamp(base_weight * 1.4f, 0.0f, 1.0f); // 0.7f scaled
                edge.count = 1;
                edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                
                store_->upsert_edge(edge);
            }
            
            prev_node = node_id;
        }
        
        return nodes;
    }
    
    void reinforce_path(const std::vector<melvin::NodeID>& nodes) {
        if (nodes.size() < 2) return;
        
        // Create and reinforce edges between consecutive nodes
        for (size_t i = 1; i < nodes.size(); ++i) {
            melvin::EdgeID edge_id = {};  // Simplified for now
            
            if (store_->edge_exists(edge_id)) {
                // Reinforce existing edge
                melvin::EdgeRec edge;
                if (store_->get_edge(edge_id, edge)) {
                    // Apply size-relative scaling to reinforcement increments
                    float size_scaling = 1.0f; // TODO: implement size scaling;
                    float scaled_alpha_ctx = 0.1f * size_scaling;
                    float scaled_alpha_core = 0.05f * size_scaling;
                    
                    edge.w_ctx = std::clamp(edge.w_ctx + scaled_alpha_ctx, 0.0f, 1.0f);
                    edge.w_core = std::clamp(edge.w_core + scaled_alpha_core, 0.0f, 1.0f);
                    edge.count++;
                    edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    
                    store_->upsert_edge(edge);
                }
            } else {
                // Create new edge if it doesn't exist
                melvin::EdgeRec edge;
                std::copy(nodes[i-1].begin(), nodes[i-1].end(), edge.src);
                edge.rel = static_cast<uint32_t>(0x01); // TEMPORAL - sequence in order
                std::copy(nodes[i].begin(), nodes[i].end(), edge.dst);
                edge.layer = 0;
                
                // Apply size-relative scaling to initial weights
                float size_scaling = 1.0f; // TODO: implement size scaling;
                float base_weight = 0.6f * size_scaling;
                
                edge.w = std::clamp(base_weight, 0.0f, 1.0f);
                edge.w_core = std::clamp(base_weight * 0.667f, 0.0f, 1.0f);
                edge.w_ctx = std::clamp(base_weight * 1.333f, 0.0f, 1.0f);
                edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                edge.count = 1;
                edge.flags = 0;
                edge.pad = 0;
                
                store_->upsert_edge(edge);
            }
        }
    }
    
    std::vector<uint8_t> create_reasoning_bytecode(const std::vector<melvin::NodeID>& query_nodes) {
        std::vector<uint8_t> bytecode;
        
        if (query_nodes.empty()) return bytecode;
        
        // Simple reasoning: WALK from first node
        bytecode.push_back(static_cast<uint8_t>(melvin::Opcode::WALK));
        
        // Push start node
        for (uint8_t byte : query_nodes[0]) {
            bytecode.push_back(byte);
        }
        
        // Create relation mask (allow NEXT, ISA, PART_OF)
        melvin::RelMask mask;
        mask.set(melvin::Rel::NEXT);
        mask.set(melvin::Rel::ISA);
        mask.set(melvin::Rel::PART_OF);
        
        // Push mask (16 bytes)
        uint64_t mask_low = mask.mask_low;
        uint64_t mask_high = mask.mask_high;
        
        for (int i = 7; i >= 0; --i) {
            bytecode.push_back(static_cast<uint8_t>((mask_low >> (i * 8)) & 0xFF));
        }
        for (int i = 7; i >= 0; --i) {
            bytecode.push_back(static_cast<uint8_t>((mask_high >> (i * 8)) & 0xFF));
        }
        
        // Push depth (4 bytes)
        uint32_t depth = 2;
        for (int i = 3; i >= 0; --i) {
            bytecode.push_back(static_cast<uint8_t>((depth >> (i * 8)) & 0xFF));
        }
        
        // Push beam size (4 bytes)
        uint32_t beam_size = 8;
        for (int i = 3; i >= 0; --i) {
            bytecode.push_back(static_cast<uint8_t>((beam_size >> (i * 8)) & 0xFF));
        }
        
        // EMIT result
        bytecode.push_back(static_cast<uint8_t>(melvin::Opcode::EMIT));
        
        // Push first node again
        for (uint8_t byte : query_nodes[0]) {
            bytecode.push_back(byte);
        }
        
        // Push view kind (TEXT_VIEW)
        bytecode.push_back(static_cast<uint8_t>(melvin::ViewKind::TEXT_VIEW));
        
        // HALT
        bytecode.push_back(static_cast<uint8_t>(melvin::Opcode::HALT));
        
        return bytecode;
    }
    
    void add_knowledge(const std::string& src_text, const std::string& dst_text, melvin::Rel relation) {
        // Create source node
        auto src_nodes = ingest_text(src_text);
        if (src_nodes.empty()) {
            std::cout << "🔍 add_knowledge: failed to create source node for '" << src_text << "'\n";
            return;
        }
        
        // Create destination node
        auto dst_nodes = ingest_text(dst_text);
        if (dst_nodes.empty()) {
            std::cout << "🔍 add_knowledge: failed to create destination node for '" << dst_text << "'\n";
            return;
        }
        
        // Verify nodes exist in store
        if (!store_->node_exists(src_nodes[0])) {
            std::cout << "🔍 add_knowledge: source node doesn't exist in store\n";
            return;
        }
        if (!store_->node_exists(dst_nodes[0])) {
            std::cout << "🔍 add_knowledge: destination node doesn't exist in store\n";
            return;
        }
        
        // Create edge
        melvin::EdgeRec edge;
        std::copy(src_nodes[0].begin(), src_nodes[0].end(), edge.src);
        edge.rel = static_cast<uint32_t>(relation);
        std::copy(dst_nodes[0].begin(), dst_nodes[0].end(), edge.dst);
        edge.layer = 0; // explicit
        
        // Apply size-relative scaling to initial weights
        float size_scaling = 1.0f; // TODO: implement size scaling;
        float base_weight = 0.8f * size_scaling;
        
        edge.w = std::clamp(base_weight, 0.0f, 1.0f);
        edge.w_core = std::clamp(base_weight * 0.875f, 0.0f, 1.0f); // 0.7f scaled
        edge.w_ctx = std::clamp(base_weight * 1.125f, 0.0f, 1.0f); // 0.9f scaled
        edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        edge.count = 5; // reinforced
        edge.flags = 0;
        edge.pad = 0;
        
        melvin::EdgeID edge_id = store_->upsert_edge(edge);
        std::cout << "🔍 add_knowledge: created edge from '" << src_text << "' to '" << dst_text << "' with relation " << static_cast<int>(relation) << "\n";
    }
    
    void reinforce_test_edges() {
        // Reinforce the test edges to make them strong
        auto cats_nodes = ingest_text("cats");
        auto mammal_nodes = ingest_text("mammal");
        auto water_nodes = ingest_text("water");
        
        if (!cats_nodes.empty() && !mammal_nodes.empty()) {
            melvin::EdgeID edge_id = {};  // Simplified for nowcats_nodes[0], static_cast<melvin::Rel>(0x00), mammal_nodes[0], 0);
            if (store_->edge_exists(edge_id)) {
                melvin::EdgeRec edge;
                if (store_->get_edge(edge_id, edge)) {
                // Apply size-relative scaling to reinforcement weights
                float size_scaling = 1.0f; // TODO: implement size scaling;
                edge.w_ctx = std::clamp(0.95f * size_scaling, 0.0f, 1.0f);
                edge.w_core = std::clamp(0.85f * size_scaling, 0.0f, 1.0f);
                    edge.count = 10;
                    edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    store_->upsert_edge(edge);
                }
            }
        }
    }
    
    void create_semantic_relationships(const std::vector<melvin::NodeID>& nodes, const std::string& text) {
        if (nodes.size() < 2) return;
        
        // SIMPLIFIED DYNAMIC RELATIONSHIP LEARNING
        // Create similarity relationships between adjacent concepts
        for (size_t i = 0; i < nodes.size() - 1; ++i) {
            // Get the text content of nodes
            melvin::NodeRecHeader header1, header2;
            std::vector<uint8_t> payload1, payload2;
            
            if (store_->get_node(nodes[i], header1, payload1) && 
                store_->get_node(nodes[i+1], header2, payload2)) {
                
                std::string word1(payload1.begin(), payload1.end());
                std::string word2(payload2.begin(), payload2.end());
                
                // Create leap relationship for adjacent words (mark uncertainty)
                create_semantic_edge(word1, word2, static_cast<melvin::Rel>(0x02)); // LEAP - mark uncertainty
            }
        }
        
        // Simple pattern-based relationship creation
        std::string lower_text = text;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
        
        // Look for "is" relationships (X is Y)
        size_t is_pos = lower_text.find(" is ");
        if (is_pos != std::string::npos) {
            std::string before_is = text.substr(0, is_pos);
            std::string after_is = text.substr(is_pos + 4);
            
            // Extract the last word before "is" and first word after
            std::istringstream before_stream(before_is);
            std::string last_word_before;
            std::string word;
            while (before_stream >> word) {
                last_word_before = word;
            }
            
            std::istringstream after_stream(after_is);
            std::string first_word_after;
            if (after_stream >> first_word_after) {
                // Create ISA relationship
                create_semantic_edge(last_word_before, first_word_after, melvin::Rel::ISA);
            }
        }
        
        // Look for "are" relationships (X are Y)
        size_t are_pos = lower_text.find(" are ");
        if (are_pos != std::string::npos) {
            std::string before_are = text.substr(0, are_pos);
            std::string after_are = text.substr(are_pos + 5);
            
            // Extract the last word before "are" and first word after
            std::istringstream before_stream(before_are);
            std::string last_word_before;
            std::string word;
            while (before_stream >> word) {
                last_word_before = word;
            }
            
            std::istringstream after_stream(after_are);
            std::string first_word_after;
            if (after_stream >> first_word_after) {
                // Create ISA relationship
                create_semantic_edge(last_word_before, first_word_after, melvin::Rel::ISA);
            }
        }
        
        // Look for "has" relationships (X has Y)
        size_t has_pos = lower_text.find(" has ");
        if (has_pos != std::string::npos) {
            std::string before_has = text.substr(0, has_pos);
            std::string after_has = text.substr(has_pos + 5);
            
            std::istringstream before_stream(before_has);
            std::string last_word_before;
            std::string word;
            while (before_stream >> word) {
                last_word_before = word;
            }
            
            std::istringstream after_stream(after_has);
            std::string first_word_after;
            if (after_stream >> first_word_after) {
                // Create HAS_PROPERTY relationship
                create_semantic_edge(last_word_before, first_word_after, melvin::Rel::HAS_PROPERTY);
            }
        }
    }
    
    void create_flexible_isa_relationships(const std::string& text, const std::string& lower_text) {
        // Pattern 1: "X is Y" / "X is a Y" / "X is an Y"
        std::vector<std::string> is_patterns = {
            " is ", " is a ", " is an ", " is the ", " is one "
        };
        
        for (const auto& pattern : is_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_isa_from_parts(before, after);
            }
        }
        
        // Pattern 2: "X are Y" / "X are a Y" / "X are the Y"
        std::vector<std::string> are_patterns = {
            " are ", " are a ", " are an ", " are the "
        };
        
        for (const auto& pattern : are_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_isa_from_parts(before, after);
            }
        }
        
        // Pattern 3: "X belongs to Y" / "X belongs in Y"
        std::vector<std::string> belongs_patterns = {
            " belongs to ", " belongs in ", " falls under ", " falls into "
        };
        
        for (const auto& pattern : belongs_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_isa_from_parts(before, after);
            }
        }
        
        // Pattern 4: "X is a type of Y" / "X is a kind of Y"
        std::vector<std::string> type_patterns = {
            " is a type of ", " is a kind of ", " is a form of ", " is a variety of "
        };
        
        for (const auto& pattern : type_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_isa_from_parts(before, after);
            }
        }
    }
    
    // Helper function to create ISA relationships from text parts
    void create_isa_from_parts(const std::string& before, const std::string& after) {
        // Extract the last word before the pattern
        std::istringstream before_stream(before);
            std::string last_word_before;
            std::string word;
            while (before_stream >> word) {
                last_word_before = word;
            }
            
        // Extract the first meaningful word after the pattern
        std::istringstream after_stream(after);
            std::string first_word_after;
            if (after_stream >> first_word_after) {
            // Clean the word (remove punctuation)
            std::string clean_word;
            for (char c : first_word_after) {
                if (std::isalpha(c)) {
                    clean_word += c;
                }
            }
            
            if (!clean_word.empty() && !last_word_before.empty()) {
                create_semantic_edge(last_word_before, clean_word, melvin::Rel::ISA);
            }
        }
    }
    
    // Enhanced property relationship detection
    void create_flexible_property_relationships(const std::string& text, const std::string& lower_text) {
        // Pattern 1: "X has Y" / "X have Y"
        std::vector<std::string> has_patterns = {
            " has ", " have ", " possess ", " contains ", " features "
        };
        
        for (const auto& pattern : has_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_property_from_parts(before, after);
            }
        }
        
        // Pattern 2: "X is characterized by Y" / "X is known for Y"
        std::vector<std::string> char_patterns = {
            " is characterized by ", " is known for ", " is famous for ", " is notable for "
        };
        
        for (const auto& pattern : char_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_property_from_parts(before, after);
            }
        }
    }
    
    // Helper function to create property relationships
    void create_property_from_parts(const std::string& before, const std::string& after) {
        std::istringstream before_stream(before);
            std::string last_word_before;
            std::string word;
            while (before_stream >> word) {
                last_word_before = word;
            }
            
        std::istringstream after_stream(after);
            std::string first_word_after;
            if (after_stream >> first_word_after) {
            std::string clean_word;
            for (char c : first_word_after) {
                if (std::isalpha(c)) {
                    clean_word += c;
                }
            }
            
            if (!clean_word.empty() && !last_word_before.empty()) {
                create_semantic_edge(last_word_before, clean_word, melvin::Rel::HAS_PROPERTY);
            }
        }
    }
    
    // Enhanced behavioral relationship detection
    void create_flexible_behavioral_relationships(const std::string& text, const std::string& lower_text) {
        // Pattern 1: "X drinks Y" / "X eat Y" / "X consume Y"
        std::vector<std::string> consume_patterns = {
            " drinks ", " eat ", " eats ", " consume ", " consumes ", " feed on "
        };
        
        for (const auto& pattern : consume_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_behavioral_from_parts(before, after, melvin::Rel::DRINKS);
            }
        }
        
        // Pattern 2: "X lives in Y" / "X inhabits Y"
        std::vector<std::string> live_patterns = {
            " lives in ", " inhabits ", " dwells in ", " resides in "
        };
        
        for (const auto& pattern : live_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_behavioral_from_parts(before, after, melvin::Rel::LOCATED_IN);
            }
        }
        
        // Pattern 3: "X can Y" / "X is able to Y"
        std::vector<std::string> ability_patterns = {
            " can ", " is able to ", " is capable of ", " has the ability to "
        };
        
        for (const auto& pattern : ability_patterns) {
            size_t pos = lower_text.find(pattern);
            if (pos != std::string::npos) {
                std::string before = text.substr(0, pos);
                std::string after = text.substr(pos + pattern.length());
                create_behavioral_from_parts(before, after, melvin::Rel::HAS_PROPERTY);
            }
        }
    }
    
    // Helper function to create behavioral relationships
    void create_behavioral_from_parts(const std::string& before, const std::string& after, melvin::Rel relation) {
        std::istringstream before_stream(before);
            std::string last_word_before;
            std::string word;
            while (before_stream >> word) {
                last_word_before = word;
            }
            
        std::istringstream after_stream(after);
            std::string first_word_after;
            if (after_stream >> first_word_after) {
            std::string clean_word;
            for (char c : first_word_after) {
                if (std::isalpha(c)) {
                    clean_word += c;
                }
            }
            
            if (!clean_word.empty() && !last_word_before.empty()) {
                create_semantic_edge(last_word_before, clean_word, relation);
            }
            }
        }
        
        // Create similarity relationships between adjacent concepts
    void create_adjacent_similarity_relationships(const std::vector<melvin::NodeID>& nodes) {
        for (size_t i = 0; i < nodes.size() - 1; ++i) {
            // Get the text content of nodes
            melvin::NodeRecHeader header1, header2;
            std::vector<uint8_t> payload1, payload2;
            
            if (store_->get_node(nodes[i], header1, payload1) && 
                store_->get_node(nodes[i+1], header2, payload2)) {
                
                std::string word1(payload1.begin(), payload1.end());
                std::string word2(payload2.begin(), payload2.end());
                
                // Create leap relationship for adjacent words (mark uncertainty)
                create_semantic_edge(word1, word2, static_cast<melvin::Rel>(0x02)); // LEAP - mark uncertainty
            }
        }
    }
    
    void create_semantic_edge(const std::string& src_text, const std::string& dst_text, melvin::Rel relation) {
        // Create source node
        auto src_nodes = ingest_text(src_text);
        if (src_nodes.empty()) return;
        
        // Create destination node
        auto dst_nodes = ingest_text(dst_text);
        if (dst_nodes.empty()) return;
        
        // Create edge
        melvin::EdgeRec edge;
        std::copy(src_nodes[0].begin(), src_nodes[0].end(), edge.src);
        edge.rel = static_cast<uint32_t>(relation);
        std::copy(dst_nodes[0].begin(), dst_nodes[0].end(), edge.dst);
        edge.layer = 0; // explicit
        // Apply size-relative scaling to initial weights
        float size_scaling = 1.0f; // TODO: implement size scaling;
        float base_weight = 0.6f * size_scaling;
        
        edge.w = std::clamp(base_weight, 0.0f, 1.0f);
        edge.w_core = std::clamp(base_weight * 0.667f, 0.0f, 1.0f); // 0.4f scaled
        edge.w_ctx = std::clamp(base_weight * 1.333f, 0.0f, 1.0f); // 0.8f scaled
        edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        edge.count = 1;
        edge.flags = 0;
        edge.pad = 0;
        
        store_->upsert_edge(edge);
    }
    
    // Create additional conceptual connections for stronger reasoning paths
    void create_conceptual_connections(const std::vector<melvin::NodeID>& nodes, const std::string& text) {
        if (nodes.size() < 2) return;
        
        // Create leap connections between non-adjacent nodes (mark uncertainty)
        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i + 2; j < nodes.size(); ++j) {
                // Create leap connections for non-adjacent nodes
                create_semantic_edge(nodes[i], nodes[j], static_cast<melvin::Rel>(0x02)); // LEAP - mark uncertainty
            }
        }
        
        // Create exact connections for duplicate words (collapse duplicates)
        std::unordered_map<std::string, std::vector<melvin::NodeID>> word_groups;
        for (const auto& node : nodes) {
            melvin::NodeRecHeader header;
            std::vector<uint8_t> payload;
            if (store_->get_node(node, header, payload)) {
                std::string word(payload.begin(), payload.end());
                word_groups[word].push_back(node);
            }
        }
        
        // Connect duplicate words with exact relationships
        for (const auto& [word, node_list] : word_groups) {
            if (node_list.size() > 1) {
                for (size_t i = 0; i < node_list.size(); ++i) {
                    for (size_t j = i + 1; j < node_list.size(); ++j) {
                        create_semantic_edge(node_list[i], node_list[j], static_cast<melvin::Rel>(0x00)); // EXACT - collapse duplicates
                    }
                }
            }
        }
    }
    
    // Create reverse relationships for bidirectional reasoning
    void create_reverse_relationships(const std::vector<melvin::NodeID>& nodes, const std::string& text) {
        if (nodes.size() < 2) return;
        
        std::string lower_text = text;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
        
        // Create reverse temporal relationships (backward sequence)
        for (size_t i = 1; i < nodes.size(); ++i) {
            create_semantic_edge(nodes[i], nodes[i-1], static_cast<melvin::Rel>(0x01)); // TEMPORAL - backward sequence
        }
        
        // Create reverse leap relationships (uncertainty in both directions)
        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i + 2; j < nodes.size(); ++j) {
                create_semantic_edge(nodes[j], nodes[i], static_cast<melvin::Rel>(0x02)); // LEAP - mark uncertainty
            }
        }
        
        // Create reverse relationships for specific patterns
        if (lower_text.find(" is ") != std::string::npos || lower_text.find(" are ") != std::string::npos) {
            // For "X is Y", also create "Y includes X" using exact relationship
            if (nodes.size() >= 2) {
                create_semantic_edge(nodes[1], nodes[0], static_cast<melvin::Rel>(0x00)); // EXACT - collapse duplicates
            }
        }
        
        if (lower_text.find(" has ") != std::string::npos) {
            // For "X has Y", also create "Y belongs to X" using temporal relationship
            if (nodes.size() >= 2) {
                create_semantic_edge(nodes[1], nodes[0], static_cast<melvin::Rel>(0x01)); // TEMPORAL - sequence
            }
        }
    }
    
    // Helper function to create semantic edge from node IDs
    void create_semantic_edge(const melvin::NodeID& src, const melvin::NodeID& dst, melvin::Rel relation) {
        melvin::EdgeRec edge;
        std::copy(src.begin(), src.end(), edge.src);
        edge.rel = static_cast<uint32_t>(relation);
        std::copy(dst.begin(), dst.end(), edge.dst);
        edge.layer = 0;
        
        float size_scaling = 1.0f;
        float base_weight = 0.5f * size_scaling; // Slightly lower weight for conceptual connections
        
        edge.w = std::clamp(base_weight, 0.0f, 1.0f);
        edge.w_core = std::clamp(base_weight * 0.667f, 0.0f, 1.0f);
        edge.w_ctx = std::clamp(base_weight * 1.333f, 0.0f, 1.0f);
        edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        edge.count = 1;
        edge.flags = 0;
        edge.pad = 0;
        
        store_->upsert_edge(edge);
    }
    
    // Inference rules for creating new relationships
    class InferenceEngine {
    private:
        melvin::Store* store_;
        melvin::VMContext* vm_context_;
        
    public:
        InferenceEngine(melvin::Store* store, melvin::VMContext* vm_context) 
            : store_(store), vm_context_(vm_context) {}
        
        // Apply transitive inference rules
        std::vector<melvin::EdgeID> apply_transitive_inference(const melvin::Path& path) {
            std::vector<melvin::EdgeID> new_edges;
            
            if (path.edges.size() < 2) return new_edges;
            
            // Get edge details
            std::vector<melvin::EdgeRec> edges;
            for (const melvin::EdgeID& edge_id : path.edges) {
                melvin::EdgeRec edge;
                if (store_->get_edge(edge_id, edge)) {
                    edges.push_back(edge);
                }
            }
            
            if (edges.size() < 2) return new_edges;
            
            // Rule 1: ISA transitive (A ISA B, B ISA C => A ISA C)
            if (edges[0].rel == static_cast<uint32_t>(melvin::Rel::ISA) && 
                edges[1].rel == static_cast<uint32_t>(melvin::Rel::ISA)) {
                
                melvin::NodeID src, dst;
                std::copy(edges[0].src, edges[0].src + 32, src.begin());
                std::copy(edges[1].dst, edges[1].dst + 32, dst.begin());
                
                float confidence = path.confidence * 0.8f; // Transitive inference confidence
                melvin::EdgeID new_edge = vm_context_->create_inferred_edge(src, dst, melvin::Rel::ISA, confidence);
                if (!melvin::is_zero_id(new_edge)) {
                    new_edges.push_back(new_edge);
                }
            }
            
            // Rule 2: Property inheritance (A ISA B, B HAS_PROPERTY C => A HAS_PROPERTY C)
            if (edges[0].rel == static_cast<uint32_t>(melvin::Rel::ISA) && 
                edges[1].rel == static_cast<uint32_t>(melvin::Rel::HAS_PROPERTY)) {
                
                melvin::NodeID src, dst;
                std::copy(edges[0].src, edges[0].src + 32, src.begin());
                std::copy(edges[1].dst, edges[1].dst + 32, dst.begin());
                
                float confidence = path.confidence * 0.7f; // Property inheritance confidence
                melvin::EdgeID new_edge = vm_context_->create_inferred_edge(src, dst, melvin::Rel::HAS_PROPERTY, confidence);
                if (!melvin::is_zero_id(new_edge)) {
                    new_edges.push_back(new_edge);
                }
            }
            
            // Rule 3: Action inheritance (A ISA B, B DRINKS C => A DRINKS C)
            if (edges[0].rel == static_cast<uint32_t>(melvin::Rel::ISA) && 
                edges[1].rel == static_cast<uint32_t>(melvin::Rel::DRINKS)) {
                
                melvin::NodeID src, dst;
                std::copy(edges[0].src, edges[0].src + 32, src.begin());
                std::copy(edges[1].dst, edges[1].dst + 32, dst.begin());
                
                float confidence = path.confidence * 0.6f; // Action inheritance confidence
                melvin::EdgeID new_edge = vm_context_->create_inferred_edge(src, dst, melvin::Rel::DRINKS, confidence);
                if (!melvin::is_zero_id(new_edge)) {
                    new_edges.push_back(new_edge);
                }
            }
            
            return new_edges;
        }
        
        // Apply similarity-based inference
        std::vector<melvin::EdgeID> apply_similarity_inference(const melvin::NodeID& node, const melvin::RelMask& mask) {
            std::vector<melvin::EdgeID> new_edges;
            
            // Find similar nodes
            melvin::AdjView similar_view;
            melvin::RelMask similar_mask;
            similar_mask.set(melvin::Rel::SIMILAR_TO);
            
            if (store_->get_out_edges(node, similar_mask, similar_view)) {
                for (size_t i = 0; i < similar_view.count; ++i) {
                    melvin::NodeID similar_node;
                    std::copy(similar_view.edges[i].dst, similar_view.edges[i].dst + 32, similar_node.begin());
                    
                    // Find properties of similar node
                    melvin::AdjView prop_view;
                    if (store_->get_out_edges(similar_node, mask, prop_view)) {
                        for (size_t j = 0; j < prop_view.count; ++j) {
                            melvin::NodeID prop_node;
                            std::copy(prop_view.edges[j].dst, prop_view.edges[j].dst + 32, prop_node.begin());
                            
                            // Create inferred property relationship
                            float confidence = 0.5f; // Similarity-based inference confidence
                            melvin::Rel inferred_rel = static_cast<melvin::Rel>(prop_view.edges[j].rel);
                            melvin::EdgeID new_edge = vm_context_->create_inferred_edge(node, prop_node, inferred_rel, confidence);
                            if (!melvin::is_zero_id(new_edge)) {
                                new_edges.push_back(new_edge);
                            }
                        }
                    }
                }
            }
            
            return new_edges;
        }
        
        // Apply causal inference
        std::vector<melvin::EdgeID> apply_causal_inference(const melvin::Path& path) {
            std::vector<melvin::EdgeID> new_edges;
            
            if (path.edges.size() < 2) return new_edges;
            
            // Get edge details
            std::vector<melvin::EdgeRec> edges;
            for (const melvin::EdgeID& edge_id : path.edges) {
                melvin::EdgeRec edge;
                if (store_->get_edge(edge_id, edge)) {
                    edges.push_back(edge);
                }
            }
            
            if (edges.size() < 2) return new_edges;
            
            // Rule: A CAUSES B, B CAUSES C => A CAUSES C (transitive causation)
            if (edges[0].rel == static_cast<uint32_t>(melvin::Rel::CAUSES) && 
                edges[1].rel == static_cast<uint32_t>(melvin::Rel::CAUSES)) {
                
                melvin::NodeID src, dst;
                std::copy(edges[0].src, edges[0].src + 32, src.begin());
                std::copy(edges[1].dst, edges[1].dst + 32, dst.begin());
                
                float confidence = path.confidence * 0.6f; // Causal inference confidence
                melvin::EdgeID new_edge = vm_context_->create_inferred_edge(src, dst, melvin::Rel::CAUSES, confidence);
                if (!melvin::is_zero_id(new_edge)) {
                    new_edges.push_back(new_edge);
                }
            }
            
            return new_edges;
        }
    };
    
    // Inference engine instance
    std::unique_ptr<InferenceEngine> inference_engine_;
    
    // Initialize inference engine
    void initialize_inference_engine() {
        inference_engine_ = std::make_unique<InferenceEngine>(store_.get(), vm_context_.get());
    }
    
    // Apply inference rules to discovered paths
    void apply_inference_rules(const std::vector<melvin::Path>& paths) {
        if (!inference_engine_) return;
        
        for (const auto& path : paths) {
            // Apply transitive inference
            auto transitive_edges = inference_engine_->apply_transitive_inference(path);
            for (const auto& edge_id : transitive_edges) {
                // Store inferred edges
                melvin::EdgeRec edge;
                if (store_->get_edge(edge_id, edge)) {
                    store_->upsert_edge(edge);
                }
            }
            
            // Apply causal inference
            auto causal_edges = inference_engine_->apply_causal_inference(path);
            for (const auto& edge_id : causal_edges) {
                // Store inferred edges
                melvin::EdgeRec edge;
                if (store_->get_edge(edge_id, edge)) {
                    store_->upsert_edge(edge);
                }
            }
        }
    }
};

// Forward declaration for C API
struct melvin_t;

// C API for external bindings
extern "C" {
    melvin_t* melvin_create(const char* store_dir) {
        try {
            std::string dir = store_dir ? std::string(store_dir) : std::string("");
            return reinterpret_cast<melvin_t*>(new Melvin(dir));
        } catch (...) {
            return nullptr;
        }
    }
    
    void melvin_destroy(melvin_t* melvin) {
        delete reinterpret_cast<Melvin*>(melvin);
    }
    
    const char* melvin_reason(melvin_t* melvin, const char* query) {
        if (!melvin || !query) return nullptr;
        
        try {
            auto* m = reinterpret_cast<Melvin*>(melvin);
            std::string result = m->reason(query);
            // Note: This leaks memory in a real implementation
            // Should use a proper string management strategy
            static std::string cached_result;
            cached_result = result;
            return cached_result.c_str();
        } catch (...) {
            return nullptr;
        }
    }
    
    void melvin_learn(melvin_t* melvin, const char* text) {
        if (!melvin || !text) return;
        
        try {
            auto* m = reinterpret_cast<Melvin*>(melvin);
            m->learn(text);
        } catch (...) {
            // Ignore errors
        }
    }
    
    void melvin_decay_pass(melvin_t* melvin) {
        if (!melvin) return;
        
        try {
            auto* m = reinterpret_cast<Melvin*>(melvin);
            m->decay_pass();
        } catch (...) {
            // Ignore errors
        }
    }
    
    size_t melvin_node_count(melvin_t* melvin) {
        if (!melvin) return 0;
        auto* m = reinterpret_cast<Melvin*>(melvin);
        return m->node_count();
    }
    
    size_t melvin_edge_count(melvin_t* melvin) {
        if (!melvin) return 0;
        auto* m = reinterpret_cast<Melvin*>(melvin);
        return m->edge_count();
    }
    
    size_t melvin_path_count(melvin_t* melvin) {
        if (!melvin) return 0;
        auto* m = reinterpret_cast<Melvin*>(melvin);
        return m->path_count();
    }
    
    void melvin_run_nightly_consolidation(melvin_t* melvin) {
        if (!melvin) return;
        
        try {
            auto* m = reinterpret_cast<Melvin*>(melvin);
            m->run_nightly_consolidation();
        } catch (...) {
            // Ignore errors
        }
    }
    
    float melvin_get_health_score(melvin_t* melvin) {
        if (!melvin) return 0.0f;
        auto* m = reinterpret_cast<Melvin*>(melvin);
        return m->get_health_score();
    }
    
    void melvin_export_metrics(melvin_t* melvin, const char* filename) {
        if (!melvin || !filename) return;
        
        try {
            auto* m = reinterpret_cast<Melvin*>(melvin);
            // Simple metrics export - just write basic stats
            std::ofstream file(filename);
            if (file.is_open()) {
                file << "nodes,edges,paths,health_score\n";
                file << m->node_count() << "," 
                     << m->edge_count() << "," 
                     << m->path_count() << "," 
                     << m->get_health_score() << "\n";
                file.close();
            }
        } catch (...) {
            // Ignore errors
        }
    }
    
    void melvin_setup_test_data(melvin_t* melvin) {
        if (!melvin) return;
        
        try {
            auto* m = reinterpret_cast<Melvin*>(melvin);
            // Setup some basic test data
            m->learn("cats are animals");
            m->learn("dogs are animals");
            m->learn("animals are living things");
        } catch (...) {
            // Ignore errors
        }
    }

// ============================================================================
// EVOLUTIONARY BRAIN SIMULATION FRAMEWORK
// ============================================================================


} // namespace melvin

// Genome structure for evolutionary parameters (hardened)
struct Genome {
    std::string id;
    uint64_t seed;
    
    // Ingest parameters
    struct Ingest {
        float temporal_weight_inc = 1.0f;
        float leap_init = 1.0f;
        float leap_create_prob = 0.06f;
    } ingest;
    
    // Reasoning parameters
    struct Reason {
        uint32_t beam_width = 8;
        uint32_t max_hops = 6;
        float leap_bias = 0.15f;
        float abstraction_thresh = 0.78f;
    } reason;
    
    // Storage parameters
    struct Storage {
        bool mmap = true;
        uint32_t flush_interval = 2048;
        uint32_t index_stride = 256;
    } storage;
    
    // Sensor parameters
    struct Sensors {
        float audio_cluster_thresh = 0.62f;
        float image_sim_thresh = 0.78f;
    } sensors;
    
    // Serialization
    std::string to_json() const;
    void from_json(const std::string& json_str);
    
    // Evolution operations
    Genome mutate(float mutation_rate = 0.1f) const;
    Genome crossover(const Genome& other) const;
    float distance(const Genome& other) const;
};

// Binary memory layout constants (aligned with Melvin spec)
constexpr uint8_t TAUGHT_NODE_MARKER = 0x01;    // 0001
constexpr uint8_t CONNECTION_MARKER = 0x05;     // 0101  
constexpr uint8_t THOUGHT_NODE_MARKER = 0x0E;   // 1110

// 2-bit relation codes
constexpr uint8_t REL_EXACT = 0x00;     // 00
constexpr uint8_t REL_TEMPORAL = 0x01;  // 01
constexpr uint8_t REL_LEAP = 0x02;      // 10
constexpr uint8_t REL_RESERVED = 0x03;  // 11

// Brain class combining genome and memory (hardened)
class EvolutionaryBrain {
private:
    Genome genome_;
    std::unique_ptr<Melvin> melvin_;
    std::string brain_id_;
    std::string memory_dir_;
    
    // Binary memory log (mmap'd)
    int mem_log_fd_ = -1;
    void* mem_log_ptr_ = nullptr;
    size_t mem_log_size_ = 0;
    size_t mem_log_offset_ = 0;
    
    // Performance metrics (hardened)
    struct Metrics {
        float accuracy = 0.0f;
        float multi_hop_score = 0.0f;
        float abstraction_use = 0.0f;
        float remote_recall = 0.0f;
        float ece = 0.0f;  // Expected Calibration Error (10-bin calibration)
        float self_consistency = 0.0f;  // Majority agreement over N=5 traversals
        float contradiction_rate = 0.0f;
        float efficiency_score = 0.0f;
        float fitness = 0.0f;
        
        uint64_t total_queries = 0;
        uint64_t correct_answers = 0;
        uint64_t total_latency_us = 0;
        uint64_t total_nodes_touched = 0;
        size_t memory_usage_bytes = 0;
        
        // I/O budget tracking
        uint64_t bytes_written = 0;
        uint64_t syscalls_made = 0;
        uint64_t time_ms = 0;
    } metrics_;
    
    // Data stream consumption
    uint64_t events_processed_ = 0;
    std::vector<std::string> recent_thoughts_;
    
    // Crash containment
    bool quarantined_ = false;
    std::string quarantine_reason_;
    
    // Leap cooldown (anti-spam)
    float current_leap_bias_;
    uint64_t last_leap_failure_time_ = 0;
    
    // Binary memory operations
    void write_taught_node(const std::string& content);
    void write_connection(uint8_t relation, const std::string& src, const std::string& dst, float weight);
    void write_thought_node(const std::vector<std::string>& path, float confidence);
    
    // Memory management
    void setup_mmap_log();
    void flush_mmap_log();
    void close_mmap_log();
    
public:
    EvolutionaryBrain(const Genome& genome, const std::string& brain_id, const std::string& base_dir);
    ~EvolutionaryBrain();
    
    // Core operations
    void ingest_data(const std::string& data);
    std::string answer_query(const std::string& query);
    void update_metrics(const std::string& query, const std::string& answer, bool correct, uint64_t latency_us);
    
    // Memory operations
    void reinforce_path(const std::vector<std::string>& path);
    void decay_memories();
    void consolidate_abstractions();
    
    // Thought replay and audit
    bool replay_thought_node(const std::string& thought_id);
    void audit_thought_replays();
    
    // Getters
    const Genome& get_genome() const { return genome_; }
    const Metrics& get_metrics() const { return metrics_; }
    const std::string& get_brain_id() const { return brain_id_; }
    uint64_t get_events_processed() const { return events_processed_; }
    bool is_quarantined() const { return quarantined_; }
    const std::string& get_quarantine_reason() const { return quarantine_reason_; }
    
    // Fitness calculation (with I/O budget penalty)
    float calculate_fitness() const;
    void export_thoughts(const std::string& filename) const;
    
    // Crash handling
    void quarantine(const std::string& reason);
    void respawn_from_elite(const Genome& elite_genome);
};

// Data stream generators (hardened with sensory outline)
class DataStreamGenerator {
private:
    std::mt19937_64 rng_;
    uint64_t event_counter_ = 0;
    
    // Fact templates (temporal chains)
    std::vector<std::string> animal_facts_ = {
        "cats drink water", "dogs chase cats", "birds fly", "fish swim",
        "mammals are warm-blooded", "reptiles are cold-blooded"
    };
    
    std::vector<std::string> object_facts_ = {
        "books contain knowledge", "cars transport people", "houses provide shelter",
        "computers process information", "phones enable communication"
    };
    
    std::vector<std::string> abstract_concepts_ = {
        "love is an emotion", "justice is fairness", "beauty is subjective",
        "truth is correspondence", "freedom is choice"
    };
    
    // Audio frame tokens (1-2 bytes, quantized)
    std::vector<uint8_t> audio_frame_tokens_;
    std::vector<std::string> phoneme_clusters_;
    std::vector<std::string> word_clusters_;
    
    // Image embeddings (compressed)
    std::vector<std::string> image_embeddings_;
    std::vector<std::string> image_words_;
    
public:
    DataStreamGenerator(uint64_t seed = 0) : rng_(seed) {
        initialize_audio_tokens();
        initialize_image_embeddings();
    }
    
    // Core generators
    std::string generate_fact();
    std::string generate_sequence();
    std::string generate_perceptual_code();
    std::string generate_noise();
    std::string generate_next_event();
    
    // Sensory-specific generators
    std::string generate_audio_frame_sequence();
    std::string generate_image_embedding();
    std::string generate_temporal_chain();
    std::string generate_leap_bridge();
    
    // Audio token management
    void initialize_audio_tokens();
    std::string cluster_audio_frames(const std::vector<uint8_t>& frames);
    
    // Image embedding management
    void initialize_image_embeddings();
    std::string find_similar_image(const std::string& embedding);
    
    uint64_t get_event_count() const { return event_counter_; }
};

// Probe system for evaluation (hardened)
class ProbeSystem {
private:
    struct ProbeResult {
        std::string query;
        std::string expected_answer;
        std::string actual_answer;
        bool correct;
        uint64_t latency_us;
        uint32_t path_length;
        float confidence;
        uint32_t nodes_touched;
        std::string thought_id;  // For replay auditing
    };
    
    std::vector<ProbeResult> results_;
    uint64_t probe_seed_;
    
public:
    ProbeSystem(uint64_t seed = 0) : probe_seed_(seed) {}
    
    // Probe generation (seeded for reproducibility)
    std::vector<std::pair<std::string, std::string>> generate_temporal_chain_probes();
    std::vector<std::pair<std::string, std::string>> generate_leap_bridge_probes();
    std::vector<std::pair<std::string, std::string>> generate_abstraction_probes();
    std::vector<std::pair<std::string, std::string>> generate_remote_recall_probes();
    std::vector<std::pair<std::string, std::string>> generate_consistency_probes();
    std::vector<std::pair<std::string, std::string>> generate_audio_token_probes();
    std::vector<std::pair<std::string, std::string>> generate_image_similarity_probes();
    
    // Execute probes on a brain
    std::vector<ProbeResult> execute_probes(EvolutionaryBrain& brain, uint32_t num_probes = 50);
    
    // Precise metric calculations
    float calculate_accuracy(const std::vector<ProbeResult>& results);
    float calculate_multi_hop_score(const std::vector<ProbeResult>& results);
    float calculate_abstraction_use(const std::vector<ProbeResult>& results);
    float calculate_remote_recall(const std::vector<ProbeResult>& results);
    float calculate_ece(const std::vector<ProbeResult>& results);  // 10-bin calibration
    float calculate_self_consistency(const std::vector<ProbeResult>& results);  // N=5 traversals
    float calculate_contradiction_rate(const std::vector<ProbeResult>& results);
    float calculate_efficiency_score(const std::vector<ProbeResult>& results);
    
    // Thought replay auditing
    bool audit_thought_replays(EvolutionaryBrain& brain, const std::vector<ProbeResult>& results);
};

// Evolution orchestrator (hardened)
class EvolutionOrchestrator {
private:
    static constexpr size_t POPULATION_SIZE = 100;
    static constexpr uint64_t EVALUATION_WINDOW = 50000;  // events
    static constexpr float ELITE_RATIO = 0.1f;
    static constexpr float MUTATION_RATIO = 0.7f;
    static constexpr float CROSSOVER_RATIO = 0.2f;
    
    std::vector<std::unique_ptr<EvolutionaryBrain>> population_;
    std::unique_ptr<DataStreamGenerator> data_generator_;
    std::unique_ptr<ProbeSystem> probe_system_;
    
    std::string base_dir_;
    uint64_t generation_ = 0;
    uint64_t total_events_ = 0;
    
    // Deterministic seed hierarchy
    uint64_t master_seed_;
    uint64_t gen_seed_;
    std::vector<uint64_t> brain_seeds_;
    
    // Constrained parallelism
    size_t max_workers_;
    std::atomic<size_t> active_workers_{0};
    std::queue<std::function<void()>> work_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Threading
    std::mutex population_mutex_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> should_stop_{false};
    
    // Crash containment
    std::vector<size_t> quarantined_brains_;
    std::mutex quarantine_mutex_;
    
public:
    EvolutionOrchestrator(const std::string& base_dir, uint64_t master_seed = 0);
    ~EvolutionOrchestrator();
    
    // Main evolution loop
    void run_evolution_cycle();
    void run_continuous_evolution();
    
    // Population management
    void initialize_population();
    void evaluate_population();
    void evolve_population();
    void respawn_brain(size_t index, const Genome& new_genome);
    
    // Data and evaluation (constrained parallelism)
    void feed_data_to_population();
    void run_probes_on_population();
    
    // Statistics and logging
    void export_generation_stats();
    void export_brain_genomes();
    void export_metrics();
    
    // Crash handling
    void quarantine_brain(size_t brain_index, const std::string& reason);
    void respawn_quarantined_brains();
    
    // Control
    void stop_evolution();
    uint64_t get_generation() const { return generation_; }
    uint64_t get_total_events() const { return total_events_; }
    
    // Worker management
    void worker_thread_function();
    void submit_work(std::function<void()> work);
};

// Runner for managing the simulation
namespace melvin {
class BrainSimulationRunner {
private:
    std::unique_ptr<EvolutionOrchestrator> orchestrator_;
    std::string output_dir_;
    bool running_ = false;
    
public:
    BrainSimulationRunner(const std::string& output_dir, uint64_t master_seed = 1234567890);
    ~BrainSimulationRunner();
    
    void start_simulation();
    void stop_simulation();
    void run_single_generation();
    
    bool is_running() const { return running_; }
    void export_final_results();
};
} // namespace melvin

// ============================================================================
// IMPLEMENTATION
// ============================================================================

// Genome implementation (hardened)
std::string Genome::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"id\":\"" << id << "\",";
    oss << "\"seed\":" << seed << ",";
    oss << "\"ingest\":{";
    oss << "\"temporal_weight_inc\":" << ingest.temporal_weight_inc << ",";
    oss << "\"leap_init\":" << ingest.leap_init << ",";
    oss << "\"leap_create_prob\":" << ingest.leap_create_prob;
    oss << "},";
    oss << "\"reason\":{";
    oss << "\"beam_width\":" << reason.beam_width << ",";
    oss << "\"max_hops\":" << reason.max_hops << ",";
    oss << "\"leap_bias\":" << reason.leap_bias << ",";
    oss << "\"abstraction_thresh\":" << reason.abstraction_thresh;
    oss << "},";
    oss << "\"storage\":{";
    oss << "\"mmap\":" << (storage.mmap ? "true" : "false") << ",";
    oss << "\"flush_interval\":" << storage.flush_interval << ",";
    oss << "\"index_stride\":" << storage.index_stride;
    oss << "},";
    oss << "\"sensors\":{";
    oss << "\"audio_cluster_thresh\":" << sensors.audio_cluster_thresh << ",";
    oss << "\"image_sim_thresh\":" << sensors.image_sim_thresh;
    oss << "}";
    oss << "}";
    return oss.str();
}

void Genome::from_json(const std::string& json_str) {
    // Simple JSON parsing - in production, use a proper JSON library
    // For now, just set default values
    id = "B000";
    seed = 0;
    ingest.temporal_weight_inc = 1.0f;
    ingest.leap_init = 1.0f;
    ingest.leap_create_prob = 0.06f;
    reason.beam_width = 8;
    reason.max_hops = 6;
    reason.leap_bias = 0.15f;
    reason.abstraction_thresh = 0.78f;
    storage.mmap = true;
    storage.flush_interval = 2048;
    storage.index_stride = 256;
    sensors.audio_cluster_thresh = 0.62f;
    sensors.image_sim_thresh = 0.78f;
}

Genome Genome::mutate(float mutation_rate) const {
    Genome mutated = *this;
    std::mt19937_64 rng(seed + std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    if (dist(rng) < mutation_rate) {
        std::uniform_real_distribution<float> rate_dist(0.5f, 2.0f);
        mutated.ingest.temporal_weight_inc = std::clamp(rate_dist(rng), 0.1f, 5.0f);
    }
    
    if (dist(rng) < mutation_rate) {
        std::uniform_int_distribution<uint32_t> width_dist(4, 32);
        mutated.reason.beam_width = width_dist(rng);
    }
    
    if (dist(rng) < mutation_rate) {
        std::uniform_real_distribution<float> bias_dist(0.05f, 0.4f);
        mutated.reason.leap_bias = bias_dist(rng);
    }
    
    if (dist(rng) < mutation_rate) {
        std::uniform_real_distribution<float> thresh_dist(0.5f, 0.95f);
        mutated.reason.abstraction_thresh = thresh_dist(rng);
    }
    
    if (dist(rng) < mutation_rate) {
        std::uniform_real_distribution<float> audio_dist(0.4f, 0.9f);
        mutated.sensors.audio_cluster_thresh = audio_dist(rng);
    }
    
    if (dist(rng) < mutation_rate) {
        std::uniform_real_distribution<float> image_dist(0.6f, 0.95f);
        mutated.sensors.image_sim_thresh = image_dist(rng);
    }
    
    return mutated;
}

Genome Genome::crossover(const Genome& other) const {
    Genome child;
    std::mt19937_64 rng(seed + other.seed);
    std::uniform_int_distribution<int> coin(0, 1);
    
    // Randomly select from each parent
    child.id = "B" + std::to_string(seed % 1000);
    child.seed = seed + other.seed;
    
    // Ingest parameters
    child.ingest.temporal_weight_inc = coin(rng) ? ingest.temporal_weight_inc : other.ingest.temporal_weight_inc;
    child.ingest.leap_init = coin(rng) ? ingest.leap_init : other.ingest.leap_init;
    child.ingest.leap_create_prob = coin(rng) ? ingest.leap_create_prob : other.ingest.leap_create_prob;
    
    // Reason parameters
    child.reason.beam_width = coin(rng) ? reason.beam_width : other.reason.beam_width;
    child.reason.max_hops = coin(rng) ? reason.max_hops : other.reason.max_hops;
    child.reason.leap_bias = coin(rng) ? reason.leap_bias : other.reason.leap_bias;
    child.reason.abstraction_thresh = coin(rng) ? reason.abstraction_thresh : other.reason.abstraction_thresh;
    
    // Storage parameters
    child.storage.mmap = coin(rng) ? storage.mmap : other.storage.mmap;
    child.storage.flush_interval = coin(rng) ? storage.flush_interval : other.storage.flush_interval;
    child.storage.index_stride = coin(rng) ? storage.index_stride : other.storage.index_stride;
    
    // Sensor parameters
    child.sensors.audio_cluster_thresh = coin(rng) ? sensors.audio_cluster_thresh : other.sensors.audio_cluster_thresh;
    child.sensors.image_sim_thresh = coin(rng) ? sensors.image_sim_thresh : other.sensors.image_sim_thresh;
    
    return child;
}

float Genome::distance(const Genome& other) const {
    float dist = 0.0f;
    dist += std::abs(ingest.temporal_weight_inc - other.ingest.temporal_weight_inc);
    dist += std::abs(ingest.leap_init - other.ingest.leap_init);
    dist += std::abs(ingest.leap_create_prob - other.ingest.leap_create_prob);
    dist += std::abs(static_cast<float>(reason.beam_width) - static_cast<float>(other.reason.beam_width)) / 32.0f;
    dist += std::abs(static_cast<float>(reason.max_hops) - static_cast<float>(other.reason.max_hops)) / 10.0f;
    dist += std::abs(reason.leap_bias - other.reason.leap_bias);
    dist += std::abs(reason.abstraction_thresh - other.reason.abstraction_thresh);
    dist += std::abs(sensors.audio_cluster_thresh - other.sensors.audio_cluster_thresh);
    dist += std::abs(sensors.image_sim_thresh - other.sensors.image_sim_thresh);
    return dist;
}

// EvolutionaryBrain implementation (hardened)
EvolutionaryBrain::EvolutionaryBrain(const Genome& genome, const std::string& brain_id, const std::string& base_dir)
    : genome_(genome), brain_id_(brain_id), memory_dir_(base_dir + "/" + brain_id), 
      current_leap_bias_(genome.reason.leap_bias) {
    
    // Create memory directory
    std::filesystem::create_directories(memory_dir_);
    
    // Initialize Melvin with genome parameters
    melvin_ = std::make_unique<Melvin>(memory_dir_);
    
    // Setup mmap'd binary log
    setup_mmap_log();
}

EvolutionaryBrain::~EvolutionaryBrain() {
    close_mmap_log();
}

void EvolutionaryBrain::ingest_data(const std::string& data) {
    try {
        melvin_->learn(data);
        events_processed_++;
        
        // Write to binary log using Melvin's format
        write_taught_node(data);
        
        // Store recent thoughts for later analysis
        if (recent_thoughts_.size() >= 1000) {
            recent_thoughts_.erase(recent_thoughts_.begin());
        }
        recent_thoughts_.push_back(data);
        
        // Flush periodically
        if (events_processed_ % genome_.storage.flush_interval == 0) {
            flush_mmap_log();
        }
        
    } catch (const std::exception& e) {
        quarantine("Ingest failure: " + std::string(e.what()));
    }
}

std::string EvolutionaryBrain::answer_query(const std::string& query) {
    if (quarantined_) {
        return "QUARANTINED: " + quarantine_reason_;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        std::string answer = melvin_->reason(query);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        uint64_t latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        
        // Update latency metrics
        metrics_.total_latency_us += latency_us;
        metrics_.total_queries++;
        metrics_.time_ms += latency_us / 1000;
        
        // Write thought node for replay auditing
        std::vector<std::string> path = {query, answer};
        write_thought_node(path, 0.8f);  // Placeholder confidence
        
        return answer;
        
    } catch (const std::exception& e) {
        quarantine("Query failure: " + std::string(e.what()));
        return "ERROR: " + std::string(e.what());
    }
}

void EvolutionaryBrain::update_metrics(const std::string& query, const std::string& answer, bool correct, uint64_t latency_us) {
    if (correct) {
        metrics_.correct_answers++;
    }
    
    // Update accuracy
    metrics_.accuracy = static_cast<float>(metrics_.correct_answers) / static_cast<float>(metrics_.total_queries);
    
    // Update efficiency score (with I/O budget penalty)
    if (metrics_.total_queries > 0) {
        float avg_latency_ms = static_cast<float>(metrics_.total_latency_us) / static_cast<float>(metrics_.total_queries) / 1000.0f;
        float io_penalty = static_cast<float>(metrics_.bytes_written) / static_cast<float>(metrics_.total_queries) / 1000.0f;  // KB per query
        metrics_.efficiency_score = 1.0f / (1.0f + avg_latency_ms / 100.0f + io_penalty / 10.0f);
    }
}

float EvolutionaryBrain::calculate_fitness() const {
    if (quarantined_) {
        return -1.0f;  // Quarantined brains get minimum fitness
    }
    
    return 0.3f * metrics_.accuracy +
           0.15f * metrics_.multi_hop_score +
           0.1f * metrics_.abstraction_use +
           0.1f * metrics_.remote_recall +
           0.1f * (1.0f - metrics_.ece) +
           0.1f * metrics_.self_consistency -
           0.05f * metrics_.contradiction_rate +
           0.1f * metrics_.efficiency_score;
}

void EvolutionaryBrain::export_thoughts(const std::string& filename) const {
    std::ofstream file(filename);
    if (file.is_open()) {
        for (const auto& thought : recent_thoughts_) {
            file << thought << "\n";
        }
    }
}

// Binary memory operations
void EvolutionaryBrain::write_taught_node(const std::string& content) {
    if (mem_log_ptr_ && mem_log_offset_ < mem_log_size_) {
        uint8_t* ptr = static_cast<uint8_t*>(mem_log_ptr_) + mem_log_offset_;
        *ptr++ = TAUGHT_NODE_MARKER;
        
        // Write content length and data
        uint32_t len = content.length();
        std::memcpy(ptr, &len, sizeof(len));
        ptr += sizeof(len);
        std::memcpy(ptr, content.data(), len);
        
        mem_log_offset_ += 1 + sizeof(len) + len;
        metrics_.bytes_written += 1 + sizeof(len) + len;
        metrics_.syscalls_made++;
    }
}

void EvolutionaryBrain::write_connection(uint8_t relation, const std::string& src, const std::string& dst, float weight) {
    if (mem_log_ptr_ && mem_log_offset_ < mem_log_size_) {
        uint8_t* ptr = static_cast<uint8_t*>(mem_log_ptr_) + mem_log_offset_;
        *ptr++ = CONNECTION_MARKER;
        *ptr++ = relation;
        
        // Write weight
        std::memcpy(ptr, &weight, sizeof(weight));
        ptr += sizeof(weight);
        
        // Write source and destination
        uint32_t src_len = src.length();
        std::memcpy(ptr, &src_len, sizeof(src_len));
        ptr += sizeof(src_len);
        std::memcpy(ptr, src.data(), src_len);
        ptr += src_len;
        
        uint32_t dst_len = dst.length();
        std::memcpy(ptr, &dst_len, sizeof(dst_len));
        ptr += sizeof(dst_len);
        std::memcpy(ptr, dst.data(), dst_len);
        
        mem_log_offset_ += 2 + sizeof(weight) + 2 * sizeof(uint32_t) + src_len + dst_len;
        metrics_.bytes_written += 2 + sizeof(weight) + 2 * sizeof(uint32_t) + src_len + dst_len;
        metrics_.syscalls_made++;
    }
}

void EvolutionaryBrain::write_thought_node(const std::vector<std::string>& path, float confidence) {
    if (mem_log_ptr_ && mem_log_offset_ < mem_log_size_) {
        uint8_t* ptr = static_cast<uint8_t*>(mem_log_ptr_) + mem_log_offset_;
        *ptr++ = THOUGHT_NODE_MARKER;
        
        // Write confidence
        std::memcpy(ptr, &confidence, sizeof(confidence));
        ptr += sizeof(confidence);
        
        // Write path length
        uint32_t path_len = path.size();
        std::memcpy(ptr, &path_len, sizeof(path_len));
        ptr += sizeof(path_len);
        
        // Write each path element
        for (const auto& element : path) {
            uint32_t elem_len = element.length();
            std::memcpy(ptr, &elem_len, sizeof(elem_len));
            ptr += sizeof(elem_len);
            std::memcpy(ptr, element.data(), elem_len);
            ptr += elem_len;
        }
        
        mem_log_offset_ += 1 + sizeof(confidence) + sizeof(path_len) + path_len * sizeof(uint32_t);
        for (const auto& element : path) {
            mem_log_offset_ += element.length();
        }
        metrics_.bytes_written += mem_log_offset_;
        metrics_.syscalls_made++;
    }
}

// Memory management
void EvolutionaryBrain::setup_mmap_log() {
    std::string log_path = memory_dir_ + "/mem.log";
    mem_log_fd_ = open(log_path.c_str(), O_CREAT | O_RDWR, 0644);
    if (mem_log_fd_ == -1) {
        quarantine("Failed to open memory log");
        return;
    }
    
    // Set initial size
    mem_log_size_ = 1024 * 1024;  // 1MB initial
    if (ftruncate(mem_log_fd_, mem_log_size_) == -1) {
        quarantine("Failed to truncate memory log");
        return;
    }
    
    // Map the file
    mem_log_ptr_ = mmap(nullptr, mem_log_size_, PROT_READ | PROT_WRITE, MAP_SHARED, mem_log_fd_, 0);
    if (mem_log_ptr_ == MAP_FAILED) {
        quarantine("Failed to mmap memory log");
        return;
    }
    
    mem_log_offset_ = 0;
}

void EvolutionaryBrain::flush_mmap_log() {
    if (mem_log_ptr_) {
        msync(mem_log_ptr_, mem_log_offset_, MS_SYNC);
    }
}

void EvolutionaryBrain::close_mmap_log() {
    if (mem_log_ptr_ && mem_log_ptr_ != MAP_FAILED) {
        munmap(mem_log_ptr_, mem_log_size_);
        mem_log_ptr_ = nullptr;
    }
    if (mem_log_fd_ != -1) {
        close(mem_log_fd_);
        mem_log_fd_ = -1;
    }
}

// Crash handling
void EvolutionaryBrain::quarantine(const std::string& reason) {
    quarantined_ = true;
    quarantine_reason_ = reason;
    std::cerr << "Brain " << brain_id_ << " quarantined: " << reason << std::endl;
}

void EvolutionaryBrain::respawn_from_elite(const Genome& elite_genome) {
    genome_ = elite_genome;
    quarantined_ = false;
    quarantine_reason_.clear();
    current_leap_bias_ = genome_.reason.leap_bias;
    
    // Reset metrics
    metrics_ = {};
    
    // Reinitialize Melvin
    melvin_ = std::make_unique<Melvin>(memory_dir_);
}

// DataStreamGenerator implementation
std::string DataStreamGenerator::generate_fact() {
    std::uniform_int_distribution<size_t> fact_dist(0, animal_facts_.size() + object_facts_.size() + abstract_concepts_.size() - 1);
    size_t index = fact_dist(rng_);
    
    if (index < animal_facts_.size()) {
        return animal_facts_[index];
    } else if (index < animal_facts_.size() + object_facts_.size()) {
        return object_facts_[index - animal_facts_.size()];
    } else {
        return abstract_concepts_[index - animal_facts_.size() - object_facts_.size()];
    }
}

std::string DataStreamGenerator::generate_sequence() {
    std::vector<std::string> sequence;
    std::uniform_int_distribution<int> seq_len(2, 5);
    int length = seq_len(rng_);
    
    for (int i = 0; i < length; ++i) {
        sequence.push_back(generate_fact());
    }
    
    std::ostringstream oss;
    for (size_t i = 0; i < sequence.size(); ++i) {
        if (i > 0) oss << " ";
        oss << sequence[i];
    }
    return oss.str();
}

std::string DataStreamGenerator::generate_perceptual_code() {
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
    std::ostringstream oss;
    oss << "PERCEPTUAL_CODE:";
    
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte_dist(rng_));
    }
    return oss.str();
}

std::string DataStreamGenerator::generate_noise() {
    std::uniform_int_distribution<int> noise_type(0, 2);
    int type = noise_type(rng_);
    
    switch (type) {
        case 0:
            return "random noise " + std::to_string(rng_());
        case 1:
            return "irrelevant data " + std::to_string(rng_());
        default:
            return "drift signal " + std::to_string(rng_());
    }
}

std::string DataStreamGenerator::generate_next_event() {
    event_counter_++;
    
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float rand = dist(rng_);
    
    if (rand < 0.4f) {
        return generate_fact();
    } else if (rand < 0.7f) {
        return generate_sequence();
    } else if (rand < 0.9f) {
        return generate_perceptual_code();
    } else {
    return generate_noise();
}

} // namespace melvin

// ProbeSystem implementation
std::vector<std::pair<std::string, std::string>> ProbeSystem::generate_temporal_chain_probes() {
    return {
        {"If cats are animals and animals are living things, what are cats?", "living things"},
        {"If dogs chase cats and cats are afraid of dogs, what happens when a dog approaches a cat?", "the cat becomes afraid"},
        {"If books contain knowledge and knowledge is power, what do books contain?", "power"},
        {"If cars transport people and people need to get places, what do cars help people do?", "get places"}
    };
}

std::vector<std::pair<std::string, std::string>> ProbeSystem::generate_abstraction_probes() {
    return {
        {"What do cats and dogs have in common?", "they are both animals"},
        {"What property do mammals and birds share?", "they are both living things"},
        {"How are books and computers similar?", "they both contain or process information"}
    };
}

std::vector<std::pair<std::string, std::string>> ProbeSystem::generate_remote_recall_probes() {
    return {
        {"What did you learn about cats earlier?", "cats are animals"},
        {"What was mentioned about books?", "books contain knowledge"},
        {"What did you learn about mammals?", "mammals are warm-blooded"}
    };
}

std::vector<std::pair<std::string, std::string>> ProbeSystem::generate_consistency_probes() {
    return {
        {"Are cats animals?", "yes"},
        {"Are cats not animals?", "no"},
        {"Do cats drink water?", "yes"},
        {"Do cats not drink water?", "no"}
    };
}

std::vector<ProbeSystem::ProbeResult> ProbeSystem::execute_probes(EvolutionaryBrain& brain, uint32_t num_probes) {
    std::vector<ProbeResult> results;
    
    auto multi_hop = generate_temporal_chain_probes();
    auto abstraction = generate_abstraction_probes();
    auto remote_recall = generate_remote_recall_probes();
    auto consistency = generate_consistency_probes();
    
    std::vector<std::pair<std::string, std::string>> all_probes;
    all_probes.insert(all_probes.end(), multi_hop.begin(), multi_hop.end());
    all_probes.insert(all_probes.end(), abstraction.begin(), abstraction.end());
    all_probes.insert(all_probes.end(), remote_recall.begin(), remote_recall.end());
    all_probes.insert(all_probes.end(), consistency.begin(), consistency.end());
    
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::shuffle(all_probes.begin(), all_probes.end(), rng);
    
    for (uint32_t i = 0; i < std::min(num_probes, static_cast<uint32_t>(all_probes.size())); ++i) {
        const auto& probe = all_probes[i];
        
        ProbeResult result;
        result.query = probe.first;
        result.expected_answer = probe.second;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        result.actual_answer = brain.answer_query(probe.first);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        result.latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        result.correct = (result.actual_answer.find(probe.second) != std::string::npos);
        result.confidence = 0.8f;  // Placeholder
        result.nodes_touched = 10;  // Placeholder
        
        results.push_back(result);
    }
    
    return results;
}

float ProbeSystem::calculate_accuracy(const std::vector<ProbeResult>& results) {
    if (results.empty()) return 0.0f;
    
    uint32_t correct = 0;
    for (const auto& result : results) {
        if (result.correct) correct++;
    }
    
    return static_cast<float>(correct) / static_cast<float>(results.size());
}

float ProbeSystem::calculate_multi_hop_score(const std::vector<ProbeResult>& results) {
    // Simplified implementation
    return calculate_accuracy(results) * 0.8f;
}

float ProbeSystem::calculate_abstraction_use(const std::vector<ProbeResult>& results) {
    // Simplified implementation
    return calculate_accuracy(results) * 0.6f;
}

float ProbeSystem::calculate_remote_recall(const std::vector<ProbeResult>& results) {
    // Simplified implementation
    return calculate_accuracy(results) * 0.7f;
}

float ProbeSystem::calculate_ece(const std::vector<ProbeResult>& results) {
    // Simplified implementation - lower is better
    return 0.1f;
}

float ProbeSystem::calculate_self_consistency(const std::vector<ProbeResult>& results) {
    // Simplified implementation
    return calculate_accuracy(results) * 0.9f;
}

float ProbeSystem::calculate_contradiction_rate(const std::vector<ProbeResult>& results) {
    // Simplified implementation - lower is better
    return 0.05f;
}

float ProbeSystem::calculate_efficiency_score(const std::vector<ProbeResult>& results) {
    if (results.empty()) return 0.0f;
    
    uint64_t total_latency = 0;
    for (const auto& result : results) {
        total_latency += result.latency_us;
    }
    
    float avg_latency_ms = static_cast<float>(total_latency) / static_cast<float>(results.size()) / 1000.0f;
    return 1.0f / (1.0f + avg_latency_ms / 100.0f);
}

// Audio token management
void DataStreamGenerator::initialize_audio_tokens() {
    // Initialize audio frame tokens (1-2 bytes, quantized)
    for (int i = 0; i < 256; ++i) {
        audio_frame_tokens_.push_back(static_cast<uint8_t>(i));
    }
    
    // Initialize phoneme clusters
    phoneme_clusters_ = {"a", "e", "i", "o", "u", "b", "c", "d", "f", "g", "h", "j", "k", "l", "m", "n", "p", "q", "r", "s", "t", "v", "w", "x", "y", "z"};
    
    // Initialize word clusters
    word_clusters_ = {"cat", "dog", "bird", "fish", "water", "food", "house", "car", "book", "tree"};
}

std::string DataStreamGenerator::cluster_audio_frames(const std::vector<uint8_t>& frames) {
    // Simple clustering based on frame values
    if (frames.empty()) return "silence";
    
    uint8_t avg_frame = 0;
    for (uint8_t frame : frames) {
        avg_frame += frame;
    }
    avg_frame /= frames.size();
    
    // Map to phoneme cluster
    size_t cluster_index = avg_frame % phoneme_clusters_.size();
    return phoneme_clusters_[cluster_index];
}

// Image embedding management
void DataStreamGenerator::initialize_image_embeddings() {
    // Initialize image embeddings (compressed)
    image_embeddings_ = {"img_001", "img_002", "img_003", "img_004", "img_005"};
    image_words_ = {"cat", "dog", "tree", "house", "car"};
}

std::string DataStreamGenerator::find_similar_image(const std::string& embedding) {
    // Simple similarity based on embedding hash
    size_t hash = std::hash<std::string>{}(embedding);
    size_t index = hash % image_words_.size();
    return image_words_[index];
}

// EvolutionOrchestrator implementation
EvolutionOrchestrator::EvolutionOrchestrator(const std::string& base_dir, uint64_t master_seed) 
    : base_dir_(base_dir), master_seed_(master_seed), data_generator_(std::make_unique<DataStreamGenerator>()), 
      probe_system_(std::make_unique<ProbeSystem>()), max_workers_(std::min(static_cast<size_t>(std::thread::hardware_concurrency()), POPULATION_SIZE)) {
    
    // Create output directories
    std::filesystem::create_directories(base_dir_ + "/genomes");
    std::filesystem::create_directories(base_dir_ + "/metrics");
    std::filesystem::create_directories(base_dir_ + "/thoughts");
    std::filesystem::create_directories(base_dir_ + "/brains");
}

EvolutionOrchestrator::~EvolutionOrchestrator() {
    stop_evolution();
}

void EvolutionOrchestrator::initialize_population() {
    population_.clear();
    population_.reserve(POPULATION_SIZE);
    
    for (size_t i = 0; i < POPULATION_SIZE; ++i) {
        Genome genome;
        // Add some initial variation
        genome.reason.beam_width = 8 + (i % 24);
        genome.ingest.temporal_weight_inc = 0.3f + (i % 7) * 0.1f;
        genome.reason.leap_bias = 0.05f + (i % 25) * 0.01f;
        
        std::string brain_id = "brain_" + std::to_string(i);
        population_.push_back(std::make_unique<EvolutionaryBrain>(genome, brain_id, base_dir_ + "/brains"));
    }
}

void EvolutionOrchestrator::run_evolution_cycle() {
    std::cout << "Starting evolution cycle, generation " << generation_ << std::endl;
    
    // Feed data to population
    feed_data_to_population();
    
    // Run probes and evaluate
    run_probes_on_population();
    
    // Evolve population
    evolve_population();
    
    // Export statistics
    export_generation_stats();
    export_brain_genomes();
    export_metrics();
    
    generation_++;
    std::cout << "Completed evolution cycle, generation " << generation_ << std::endl;
}

void EvolutionOrchestrator::run_continuous_evolution() {
    initialize_population();
    
    while (!should_stop_) {
        run_evolution_cycle();
        
        // Check if we should stop (e.g., after certain number of generations)
        if (generation_ >= 100) {
            break;
        }
        
        // Small delay between generations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void EvolutionOrchestrator::feed_data_to_population() {
    // Feed same data stream to all brains with slight offsets
    for (uint64_t event = 0; event < EVALUATION_WINDOW; ++event) {
        std::string data = data_generator_->generate_next_event();
        
        // Feed to all brains in parallel
        std::vector<std::future<void>> futures;
        
        for (auto& brain : population_) {
            futures.push_back(std::async(std::launch::async, [&brain, &data]() {
                brain->ingest_data(data);
            }));
        }
        
        // Wait for all brains to process
        for (auto& future : futures) {
            future.wait();
        }
        
        total_events_++;
    }
}

void EvolutionOrchestrator::run_probes_on_population() {
    std::vector<std::future<void>> futures;
    
    for (auto& brain : population_) {
        futures.push_back(std::async(std::launch::async, [&brain, this]() {
            auto results = probe_system_->execute_probes(*brain, 50);
            
            // Update brain metrics
            float accuracy = probe_system_->calculate_accuracy(results);
            float multi_hop = probe_system_->calculate_multi_hop_score(results);
            float abstraction = probe_system_->calculate_abstraction_use(results);
            float remote_recall = probe_system_->calculate_remote_recall(results);
            float ece = probe_system_->calculate_ece(results);
            float self_consistency = probe_system_->calculate_self_consistency(results);
            float contradiction_rate = probe_system_->calculate_contradiction_rate(results);
            float efficiency = probe_system_->calculate_efficiency_score(results);
            
            // Update brain's internal metrics
            brain->update_metrics("", "", accuracy > 0.5f, 0);
        }));
    }
    
    // Wait for all probes to complete
    for (auto& future : futures) {
        future.wait();
    }
}

void EvolutionOrchestrator::evolve_population() {
    // Sort population by fitness
    std::sort(population_.begin(), population_.end(), 
              [](const std::unique_ptr<EvolutionaryBrain>& a, const std::unique_ptr<EvolutionaryBrain>& b) {
                  return a->calculate_fitness() > b->calculate_fitness();
              });
    
    size_t elite_count = static_cast<size_t>(POPULATION_SIZE * ELITE_RATIO);
    size_t mutation_count = static_cast<size_t>(POPULATION_SIZE * MUTATION_RATIO);
    size_t crossover_count = static_cast<size_t>(POPULATION_SIZE * CROSSOVER_RATIO);
    
    // Keep elite
    std::vector<Genome> elite_genomes;
    for (size_t i = 0; i < elite_count; ++i) {
        elite_genomes.push_back(population_[i]->get_genome());
    }
    
    // Create new population
    std::vector<std::unique_ptr<EvolutionaryBrain>> new_population;
    new_population.reserve(POPULATION_SIZE);
    
    // Add elite
    for (size_t i = 0; i < elite_count; ++i) {
        std::string brain_id = "brain_" + std::to_string(generation_) + "_" + std::to_string(i);
        new_population.push_back(std::make_unique<EvolutionaryBrain>(elite_genomes[i], brain_id, base_dir_ + "/brains"));
    }
    
    // Add mutated offspring
    for (size_t i = 0; i < mutation_count; ++i) {
        Genome parent = elite_genomes[i % elite_genomes.size()];
        Genome mutated = parent.mutate(0.1f);
        
        std::string brain_id = "brain_" + std::to_string(generation_) + "_mut_" + std::to_string(i);
        new_population.push_back(std::make_unique<EvolutionaryBrain>(mutated, brain_id, base_dir_ + "/brains"));
    }
    
    // Add crossover offspring
    for (size_t i = 0; i < crossover_count; ++i) {
        Genome parent1 = elite_genomes[i % elite_genomes.size()];
        Genome parent2 = elite_genomes[(i + 1) % elite_genomes.size()];
        Genome child = parent1.crossover(parent2);
        
        std::string brain_id = "brain_" + std::to_string(generation_) + "_cross_" + std::to_string(i);
        new_population.push_back(std::make_unique<EvolutionaryBrain>(child, brain_id, base_dir_ + "/brains"));
    }
    
    // Replace population
    population_ = std::move(new_population);
}

void EvolutionOrchestrator::export_generation_stats() {
    std::string filename = base_dir_ + "/metrics/generation_" + std::to_string(generation_) + ".csv";
    std::ofstream file(filename);
    
    if (file.is_open()) {
        file << "brain_id,fitness,accuracy,multi_hop,abstraction,remote_recall,ece,self_consistency,contradiction_rate,efficiency\n";
        
        for (const auto& brain : population_) {
            const auto& metrics = brain->get_metrics();
            file << brain->get_brain_id() << ","
                 << brain->calculate_fitness() << ","
                 << metrics.accuracy << ","
                 << metrics.multi_hop_score << ","
                 << metrics.abstraction_use << ","
                 << metrics.remote_recall << ","
                 << metrics.ece << ","
                 << metrics.self_consistency << ","
                 << metrics.contradiction_rate << ","
                 << metrics.efficiency_score << "\n";
        }
    }
}

void EvolutionOrchestrator::export_brain_genomes() {
    for (const auto& brain : population_) {
        std::string filename = base_dir_ + "/genomes/" + brain->get_brain_id() + ".json";
        std::ofstream file(filename);
        
        if (file.is_open()) {
            file << brain->get_genome().to_json();
        }
    }
}

void EvolutionOrchestrator::export_metrics() {
    std::string filename = base_dir_ + "/metrics/overall_metrics.csv";
    std::ofstream file(filename, std::ios::app);
    
    if (file.is_open()) {
        if (generation_ == 0) {
            file << "generation,avg_fitness,best_fitness,worst_fitness,total_events\n";
        }
        
        float avg_fitness = 0.0f;
        float best_fitness = 0.0f;
        float worst_fitness = 1.0f;
        
        for (const auto& brain : population_) {
            float fitness = brain->calculate_fitness();
            avg_fitness += fitness;
            best_fitness = std::max(best_fitness, fitness);
            worst_fitness = std::min(worst_fitness, fitness);
        }
        
        avg_fitness /= population_.size();
        
        file << generation_ << "," << avg_fitness << "," << best_fitness << "," << worst_fitness << "," << total_events_ << "\n";
    }
}

void EvolutionOrchestrator::stop_evolution() {
    should_stop_ = true;
    
    // Wait for worker threads to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
}

// BrainSimulationRunner implementation
melvin::BrainSimulationRunner::BrainSimulationRunner(const std::string& output_dir, uint64_t master_seed) 
    : orchestrator_(std::make_unique<EvolutionOrchestrator>(output_dir, master_seed)), output_dir_(output_dir) {
    
    std::filesystem::create_directories(output_dir_);
}

melvin::BrainSimulationRunner::~BrainSimulationRunner() {
    stop_simulation();
}

void melvin::BrainSimulationRunner::start_simulation() {
    if (running_) return;
    
    running_ = true;
    std::cout << "Starting evolutionary brain simulation..." << std::endl;
    
    // Run evolution in a separate thread
    std::thread evolution_thread([this]() {
        orchestrator_->run_continuous_evolution();
    });
    
    evolution_thread.detach();
}

void melvin::BrainSimulationRunner::stop_simulation() {
    if (!running_) return;
    
    running_ = false;
    orchestrator_->stop_evolution();
    std::cout << "Stopped evolutionary brain simulation." << std::endl;
}

void melvin::BrainSimulationRunner::run_single_generation() {
    // Initialize population if not already done
    if (orchestrator_->get_generation() == 0) {
        orchestrator_->initialize_population();
    }
    orchestrator_->run_evolution_cycle();
}

void melvin::BrainSimulationRunner::export_final_results() {
    orchestrator_->export_generation_stats();
    orchestrator_->export_brain_genomes();
    orchestrator_->export_metrics();
    
    std::cout << "Final results exported to " << output_dir_ << std::endl;
}

