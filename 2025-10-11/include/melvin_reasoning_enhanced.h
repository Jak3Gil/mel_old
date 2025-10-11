/*
 * MELVIN UNIFIED BRAIN - ENHANCED REASONING ENGINE
 * WITH TELEMETRY, CONFIG, AND CONFLICT HANDLING
 * 
 * Integrates:
 * - Telemetry for event logging
 * - Configurable thresholds
 * - Conflict detection and abstention
 */

#pragma once
#include "melvin_storage.h"
#include "../src/util/telemetry.h"
#include "../src/util/config.h"
#include "../src/reasoning/conflicts.h"
#include <random>
#include <queue>
#include <sstream>
#include <iomanip>

namespace melvin_reasoning {

using namespace melvin_storage;
using namespace melvin_telemetry;
using namespace melvin_config;
using namespace melvin_conflicts;

// ==================== RELATION TYPES ====================

enum Rel : uint32_t {
    TEMPORAL = 1,      // Sequential ordering
    ISA = 3,           // "X is Y"
    HAS = 4,           // "X has Y"
    CAN = 5,           // "X can Y"
    CONSUMES = 6,      // "X eats Y"
    CROSS_MODAL = 8    // Links different modalities
};

// ==================== CONNECTOR NORMALIZATION ====================

inline std::string normalize_connector(const std::string& word) {
    if (word == "is" || word == "are" || word == "was" || word == "were" || word == "be") {
        return "BE";
    }
    if (word == "has" || word == "have" || word == "had") {
        return "HAS";
    }
    if (word == "can" || word == "could" || word == "able") {
        return "CAN";
    }
    if (word == "does" || word == "do" || word == "did") {
        return "DOES";
    }
    return word;
}

inline int get_connector_node(const std::string& connector) {
    std::string normalized = normalize_connector(connector);
    
    auto it = node_lookup.find(normalized);
    if (it != node_lookup.end()) {
        return it->second;
    }
    
    return create_node(normalized, 1, 0);
}

// ==================== TEMPLATE GAP DETECTION ====================

struct TemplateGap {
    bool has_gap = false;
    int subject_node = -1;
    int connector_node = -1;
    int object_node = -1;
    std::string connector_text;
};

inline TemplateGap detect_template_gap(const std::vector<int>& query_nodes, const std::vector<std::string>& query_words) {
    TemplateGap gap;
    
    if (query_nodes.size() < 2 || query_words.size() < 2) {
        return gap;
    }
    
    for (size_t i = 0; i + 1 < query_words.size(); ++i) {
        std::string word = query_words[i];
        std::string normalized = normalize_connector(word);
        
        if (normalized == "BE" || normalized == "HAS" || normalized == "CAN" || normalized == "DOES") {
            gap.has_gap = true;
            gap.connector_text = normalized;
            gap.connector_node = get_connector_node(normalized);
            
            if (i > 0 && query_words[i-1] != "what" && query_words[i-1] != "who") {
                auto it = node_lookup.find(query_words[i-1]);
                if (it != node_lookup.end()) {
                    gap.subject_node = it->second;
                }
            }
            
            if (i + 1 < query_words.size()) {
                auto it = node_lookup.find(query_words[i+1]);
                if (it != node_lookup.end()) {
                    gap.object_node = it->second;
                }
            }
            
            break;
        }
    }
    
    return gap;
}

// ==================== CROWD SUPPORT ANALYSIS ====================

struct LeapCandidate {
    int target_node = -1;
    float support = 0.0f;
    int distinct_subjects = 0;
    float avg_weight = 0.0f;
    std::vector<int> example_subjects;
};

inline std::vector<LeapCandidate> gather_leap_candidates(int connector_node) {
    std::vector<LeapCandidate> candidates;
    std::map<int, LeapCandidate> target_map;
    
    for (const auto& edge : edges) {
        if (edge.b == connector_node && edge.is_exact()) {
            int subject = edge.a;
            
            auto it = adjacency.find(connector_node);
            if (it != adjacency.end()) {
                for (int edge_id : it->second) {
                    if (edges[edge_id].is_exact()) {
                        int target = edges[edge_id].b;
                        
                        auto& candidate = target_map[target];
                        candidate.target_node = target;
                        candidate.support += edge.w;
                        
                        bool already_counted = false;
                        for (int existing_subj : candidate.example_subjects) {
                            if (existing_subj == subject) {
                                already_counted = true;
                                break;
                            }
                        }
                        
                        if (!already_counted) {
                            candidate.example_subjects.push_back(subject);
                            candidate.distinct_subjects = candidate.example_subjects.size();
                        }
                    }
                }
            }
        }
    }
    
    for (auto& [target, candidate] : target_map) {
        if (candidate.distinct_subjects > 0) {
            candidate.avg_weight = candidate.support / candidate.distinct_subjects;
            candidates.push_back(candidate);
        }
    }
    
    std::sort(candidates.begin(), candidates.end(), 
              [](const LeapCandidate& a, const LeapCandidate& b) {
                  return a.support > b.support;
              });
    
    return candidates;
}

// ==================== SEMANTIC SIMILARITY ====================

inline float compute_similarity(int node_a, int node_b) {
    if (node_a == node_b) return 1.0f;
    if (node_a < 0 || node_a >= (int)nodes.size()) return 0.0f;
    if (node_b < 0 || node_b >= (int)nodes.size()) return 0.0f;
    
    std::set<int> neighbors_a, neighbors_b;
    
    auto it_a = adjacency.find(node_a);
    if (it_a != adjacency.end()) {
        for (int edge_id : it_a->second) {
            neighbors_a.insert(edges[edge_id].b);
        }
    }
    
    auto it_b = adjacency.find(node_b);
    if (it_b != adjacency.end()) {
        for (int edge_id : it_b->second) {
            neighbors_b.insert(edges[edge_id].b);
        }
    }
    
    std::vector<int> intersection;
    std::set_intersection(neighbors_a.begin(), neighbors_a.end(),
                         neighbors_b.begin(), neighbors_b.end(),
                         std::back_inserter(intersection));
    
    if (neighbors_a.empty() && neighbors_b.empty()) return 0.0f;
    
    int shared = intersection.size();
    int total = neighbors_a.size() + neighbors_b.size() - shared;
    
    return total > 0 ? (float)shared / total : 0.0f;
}

inline std::vector<int> find_shared_intermediaries(int node_a, int node_b) {
    std::vector<int> intermediaries;
    
    auto it_a = adjacency.find(node_a);
    auto it_b = adjacency.find(node_b);
    
    if (it_a == adjacency.end() || it_b == adjacency.end()) {
        return intermediaries;
    }
    
    std::set<int> neighbors_a, neighbors_b;
    for (int edge_id : it_a->second) {
        neighbors_a.insert(edges[edge_id].b);
    }
    for (int edge_id : it_b->second) {
        neighbors_b.insert(edges[edge_id].b);
    }
    
    std::set_intersection(neighbors_a.begin(), neighbors_a.end(),
                         neighbors_b.begin(), neighbors_b.end(),
                         std::back_inserter(intermediaries));
    
    return intermediaries;
}

// ==================== CONTEXT FIELD ====================

inline std::vector<float> context_field;

inline void init_context_field() {
    context_field.resize(nodes.size(), 0.0f);
}

inline void activate_node(int node_id, float activation = 1.0f) {
    if (context_field.empty()) init_context_field();
    if (node_id >= 0 && node_id < (int)context_field.size()) {
        context_field[node_id] = activation;
    }
}

// ==================== ENHANCED LEAP LOGIC WITH CONFIG ====================

inline float compute_leap_score(int node_a, int node_b) {
    if (context_field.empty()) {
        return compute_similarity(node_a, node_b);
    }
    if (node_a < 0 || node_a >= (int)context_field.size()) return 0.0f;
    if (node_b < 0 || node_b >= (int)context_field.size()) return 0.0f;
    
    // Check if EXACT edge already exists
    auto it = adjacency.find(node_a);
    if (it != adjacency.end()) {
        for (int edge_id : it->second) {
            if (edges[edge_id].b == node_b && edges[edge_id].is_exact()) {
                return 0.0f;
            }
        }
    }
    
    float similarity = compute_similarity(node_a, node_b);
    float activation = context_field[node_a] + context_field[node_b];
    float not_exists = 1.0f;
    
    return similarity * activation * not_exists;
}

// Create LEAP with crowd support + telemetry + config
inline int create_leap_with_support(int subject_node, int connector_node, uint32_t rel = Rel::TEMPORAL) {
    SCOPED_TIMER("create_leap_with_support");
    
    auto candidates = gather_leap_candidates(connector_node);
    
    if (candidates.empty()) {
        return -1;
    }
    
    const LeapCandidate& best = candidates[0];
    const LeapCandidate& second = candidates.size() > 1 ? candidates[1] : LeapCandidate();
    
    // Use configurable thresholds
    auto cfg = get_config();
    bool has_support = best.support >= cfg.th_support;
    bool has_diversity = best.distinct_subjects >= (int)cfg.th_diversity;
    bool has_margin = best.support >= second.support + cfg.th_margin;
    
    if (has_support && has_diversity && has_margin) {
        // Check for conflicts
        int conflicting = -1;
        if (has_conflict(subject_node, connector_node, best.target_node, &conflicting)) {
            std::stringstream ss_support, ss_diversity, ss_margin;
            ss_support << best.support;
            ss_diversity << best.distinct_subjects;
            ss_margin << (best.support - second.support);
            
            LOG_LEAP_REJECT(
                nodes[subject_node].data,
                nodes[connector_node].data,
                "conflict_with_exact",
                ss_support.str(),
                ss_diversity.str(),
                ss_margin.str()
            );
            
            std::cout << "   ⚠️  [LEAP REJECTED] Conflicts with EXACT: "
                      << nodes[subject_node].data << " -> " << nodes[conflicting].data << "\n";
            return -1;
        }
        
        // Create LEAP
        float leap_weight = std::min(0.8f, best.support / 10.0f);
        int edge_id = connect(subject_node, best.target_node, rel, leap_weight, EdgeType::LEAP);
        
        if (edge_id >= 0) {
            edges[edge_id].leap_score = best.support;
            
            // Log telemetry
            std::stringstream ss_support, ss_diversity;
            ss_support << std::fixed << std::setprecision(2) << best.support;
            ss_diversity << best.distinct_subjects;
            
            LOG_LEAP_CREATE(
                nodes[subject_node].data,
                nodes[connector_node].data,
                nodes[best.target_node].data,
                ss_support.str(),
                ss_diversity.str()
            );
            
            std::cout << "   🔮 [LEAP CREATED] " << nodes[subject_node].data 
                     << " --LEAP--> " << nodes[best.target_node].data
                     << " (support: " << best.support 
                     << ", subjects: " << best.distinct_subjects 
                     << ", examples: [";
            
            for (size_t i = 0; i < std::min((size_t)3, best.example_subjects.size()); ++i) {
                std::cout << nodes[best.example_subjects[i]].data;
                if (i + 1 < std::min((size_t)3, best.example_subjects.size())) std::cout << ", ";
            }
            std::cout << "])\n";
        }
        
        return edge_id;
    }
    
    // Log rejection
    std::stringstream ss_support, ss_diversity, ss_margin;
    ss_support << best.support << "/" << cfg.th_support;
    ss_diversity << best.distinct_subjects << "/" << (int)cfg.th_diversity;
    ss_margin << (best.support - second.support) << "/" << cfg.th_margin;
    
    LOG_LEAP_REJECT(
        nodes[subject_node].data,
        nodes[connector_node].data,
        "insufficient_support",
        ss_support.str(),
        ss_diversity.str(),
        ss_margin.str()
    );
    
    std::cout << "   ⚠️  [LEAP REJECTED] Insufficient support "
              << "(support:" << best.support << "/" << cfg.th_support
              << ", diversity:" << best.distinct_subjects << "/" << (int)cfg.th_diversity
              << ", margin:" << (best.support - second.support) << "/" << cfg.th_margin << ")\n";
    
    return -1;
}

inline int create_leap_if_needed(int node_a, int node_b, uint32_t rel = Rel::TEMPORAL) {
    auto cfg = get_config();
    float score = compute_leap_score(node_a, node_b);
    
    if (score > cfg.leap_threshold) {
        int edge_id = connect(node_a, node_b, rel, score, EdgeType::LEAP);
        if (edge_id >= 0) {
            edges[edge_id].leap_score = score;
        }
        return edge_id;
    }
    
    return -1;
}

// ==================== PATH SCORING WITH CONFIG ====================

inline double score_path(const std::vector<int>& path) {
    if (path.size() < 2) return 0.0;
    
    auto cfg = get_config();
    double score = 0.0;
    int n_exact = 0, n_leap = 0;
    double total_weight = 0.0;
    
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        auto it = adjacency.find(path[i]);
        if (it != adjacency.end()) {
            for (int edge_id : it->second) {
                if (edges[edge_id].b == path[i+1]) {
                    const auto& edge = edges[edge_id];
                    
                    score += std::log(1.0 + edge.w);
                    total_weight += edge.w;
                    
                    if (edge.is_exact()) {
                        n_exact++;
                    } else if (edge.is_leap()) {
                        n_leap++;
                    }
                    
                    break;
                }
            }
        }
    }
    
    score += cfg.kappa * n_exact;
    score -= cfg.mu * n_leap;
    
    std::set<int> unique_nodes(path.begin(), path.end());
    double coherence = (double)unique_nodes.size() / path.size();
    score += cfg.sigma * coherence;
    
    return score;
}

inline std::vector<int> find_exact_chain(int start, int target, int max_depth = 5) {
    std::vector<int> path;
    std::queue<std::pair<int, std::vector<int>>> queue;
    std::set<int> visited;
    
    queue.push({start, {start}});
    visited.insert(start);
    
    while (!queue.empty() && path.empty()) {
        auto [current, current_path] = queue.front();
        queue.pop();
        
        if (current == target) {
            path = current_path;
            break;
        }
        
        if ((int)current_path.size() >= max_depth) continue;
        
        auto it = adjacency.find(current);
        if (it != adjacency.end()) {
            for (int edge_id : it->second) {
                if (edges[edge_id].is_exact() && visited.find(edges[edge_id].b) == visited.end()) {
                    std::vector<int> new_path = current_path;
                    new_path.push_back(edges[edge_id].b);
                    queue.push({edges[edge_id].b, new_path});
                    visited.insert(edges[edge_id].b);
                }
            }
        }
    }
    
    return path;
}

// ==================== TRAVERSAL ====================

inline std::vector<int> traverse(int start, int max_steps = 10, bool prefer_exact = true) {
    std::vector<int> path = {start};
    if (start < 0 || start >= (int)nodes.size()) return path;
    
    int current = start;
    
    for (int step = 0; step < max_steps; ++step) {
        auto it = adjacency.find(current);
        if (it == adjacency.end() || it->second.empty()) break;
        
        int best_edge = -1;
        float best_weight = 0.0f;
        bool found_exact = false;
        
        for (int edge_id : it->second) {
            if (edge_id >= 0 && edge_id < (int)edges.size()) {
                float weight = edges[edge_id].w;
                bool is_exact = edges[edge_id].is_exact();
                
                if (prefer_exact && is_exact && !found_exact) {
                    best_weight = weight;
                    best_edge = edge_id;
                    found_exact = true;
                } else if (prefer_exact && found_exact && is_exact) {
                    if (weight > best_weight) {
                        best_weight = weight;
                        best_edge = edge_id;
                    }
                } else if (!prefer_exact || !found_exact) {
                    if (weight > best_weight) {
                        best_weight = weight;
                        best_edge = edge_id;
                        if (is_exact) found_exact = true;
                    }
                }
            }
        }
        
        if (best_edge < 0) break;
        
        int next_node = edges[best_edge].b;
        path.push_back(next_node);
        current = next_node;
        
        touch_node(next_node);
    }
    
    return path;
}

inline std::vector<int> traverse_probabilistic(int start, int max_steps = 10, float temperature = 1.0f) {
    std::vector<int> path = {start};
    if (start < 0 || start >= (int)nodes.size()) return path;
    
    int current = start;
    std::mt19937 rng(std::random_device{}());
    
    for (int step = 0; step < max_steps; ++step) {
        auto it = adjacency.find(current);
        if (it == adjacency.end() || it->second.empty()) break;
        
        std::vector<float> probs;
        std::vector<int> next_nodes;
        float total = 0.0f;
        
        for (int edge_id : it->second) {
            if (edge_id >= 0 && edge_id < (int)edges.size()) {
                float weight = edges[edge_id].w;
                
                if (edges[edge_id].is_cross_modal) {
                    weight += edges[edge_id].cross_modal_bonus;
                }
                
                float prob = std::pow(weight, 1.0f / temperature);
                probs.push_back(prob);
                next_nodes.push_back(edges[edge_id].b);
                total += prob;
            }
        }
        
        if (total == 0.0f) break;
        
        std::uniform_real_distribution<float> dist(0.0f, total);
        float r = dist(rng);
        float cumulative = 0.0f;
        
        int next_node = -1;
        for (size_t i = 0; i < probs.size(); ++i) {
            cumulative += probs[i];
            if (r <= cumulative) {
                next_node = next_nodes[i];
                break;
            }
        }
        
        if (next_node < 0) break;
        
        path.push_back(next_node);
        current = next_node;
        touch_node(next_node);
    }
    
    return path;
}

inline void diffuse_context(int steps = 5, float tau = 0.1f) {
    if (context_field.empty()) init_context_field();
    
    for (int step = 0; step < steps; ++step) {
        std::vector<float> delta(context_field.size(), 0.0f);
        
        for (const auto& edge : edges) {
            if (edge.a >= 0 && edge.a < (int)context_field.size() &&
                edge.b >= 0 && edge.b < (int)context_field.size()) {
                
                float flow = edge.w * (context_field[edge.a] - context_field[edge.b]);
                
                if (nodes[edge.a].sensory_type != nodes[edge.b].sensory_type) {
                    flow *= 0.5f;
                }
                
                delta[edge.b] += tau * flow;
                delta[edge.a] -= tau * flow;
            }
        }
        
        for (size_t i = 0; i < context_field.size(); ++i) {
            context_field[i] += delta[i];
            context_field[i] *= 0.95f;
        }
    }
}

inline std::vector<int> get_top_activated(int k = 10) {
    std::vector<std::pair<float, int>> scored;
    for (size_t i = 0; i < context_field.size(); ++i) {
        scored.push_back({context_field[i], (int)i});
    }
    
    std::sort(scored.begin(), scored.end(), std::greater<>());
    
    std::vector<int> result;
    for (int i = 0; i < k && i < (int)scored.size(); ++i) {
        result.push_back(scored[i].second);
    }
    
    return result;
}

} // namespace melvin_reasoning

