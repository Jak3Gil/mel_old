/*
 * MELVIN UNIFIED BRAIN - REASONING ENGINE
 * Traversal logic, diffusion, and emergent dimensions
 */

#pragma once
#include "melvin_storage.h"
#include <random>
#include <queue>

namespace melvin_reasoning {

using namespace melvin_storage;

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

// Normalize connector words to canonical form
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

// Get connector node ID, create if needed
inline int get_connector_node(const std::string& connector) {
    std::string normalized = normalize_connector(connector);
    
    auto it = node_lookup.find(normalized);
    if (it != node_lookup.end()) {
        return it->second;
    }
    
    // Create canonical connector node
    return create_node(normalized, 1, 0);  // type=1 for connector
}

// ==================== TEMPLATE GAP DETECTION ====================

// Detect if query has a template structure like "X connector Y?"
struct TemplateGap {
    bool has_gap = false;
    int subject_node = -1;      // X
    int connector_node = -1;    // "are", "have", etc.
    int object_node = -1;       // Y
    std::string connector_text;
};

inline TemplateGap detect_template_gap(const std::vector<int>& query_nodes, const std::vector<std::string>& query_words) {
    TemplateGap gap;
    
    if (query_nodes.size() < 2 || query_words.size() < 2) {
        return gap;
    }
    
    // Look for pattern: "what/are X connector Y"
    for (size_t i = 0; i + 1 < query_words.size(); ++i) {
        std::string word = query_words[i];
        std::string normalized = normalize_connector(word);
        
        // Check if this is a connector
        if (normalized == "BE" || normalized == "HAS" || normalized == "CAN" || normalized == "DOES") {
            // Found connector!
            gap.has_gap = true;
            gap.connector_text = normalized;
            gap.connector_node = get_connector_node(normalized);
            
            // Subject is before connector
            if (i > 0 && query_words[i-1] != "what" && query_words[i-1] != "who") {
                auto it = node_lookup.find(query_words[i-1]);
                if (it != node_lookup.end()) {
                    gap.subject_node = it->second;
                }
            }
            
            // Object is after connector
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

// Candidate for LEAP creation
struct LeapCandidate {
    int target_node = -1;
    float support = 0.0f;           // How many examples support this
    int distinct_subjects = 0;      // How many different subjects
    float avg_weight = 0.0f;        // Average edge weight
    std::vector<int> example_subjects;
};

// Gather candidates: find all Y where "? connector Y" exists
inline std::vector<LeapCandidate> gather_leap_candidates(int connector_node) {
    std::vector<LeapCandidate> candidates;
    std::map<int, LeapCandidate> target_map;
    
    // Find all edges that connect TO this connector
    for (const auto& edge : edges) {
        if (edge.b == connector_node && edge.is_exact()) {
            int subject = edge.a;
            
            // Now find what this subject connects to AFTER the connector
            auto it = adjacency.find(connector_node);
            if (it != adjacency.end()) {
                for (int edge_id : it->second) {
                    if (edges[edge_id].is_exact()) {
                        int target = edges[edge_id].b;
                        
                        // Record this as support for target
                        auto& candidate = target_map[target];
                        candidate.target_node = target;
                        candidate.support += edge.w;
                        
                        // Track distinct subjects
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
    
    // Convert map to vector and compute averages
    for (auto& [target, candidate] : target_map) {
        if (candidate.distinct_subjects > 0) {
            candidate.avg_weight = candidate.support / candidate.distinct_subjects;
            candidates.push_back(candidate);
        }
    }
    
    // Sort by support (highest first)
    std::sort(candidates.begin(), candidates.end(), 
              [](const LeapCandidate& a, const LeapCandidate& b) {
                  return a.support > b.support;
              });
    
    return candidates;
}

// ==================== SEMANTIC SIMILARITY ====================

// Compute similarity between two nodes based on shared neighbors
inline float compute_similarity(int node_a, int node_b) {
    if (node_a == node_b) return 1.0f;
    if (node_a < 0 || node_a >= (int)nodes.size()) return 0.0f;
    if (node_b < 0 || node_b >= (int)nodes.size()) return 0.0f;
    
    // Get neighbors of each node
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
    
    // Count shared neighbors (Jaccard similarity)
    std::vector<int> intersection;
    std::set_intersection(neighbors_a.begin(), neighbors_a.end(),
                         neighbors_b.begin(), neighbors_b.end(),
                         std::back_inserter(intersection));
    
    if (neighbors_a.empty() && neighbors_b.empty()) return 0.0f;
    
    int shared = intersection.size();
    int total = neighbors_a.size() + neighbors_b.size() - shared;
    
    return total > 0 ? (float)shared / total : 0.0f;
}

// Find shared intermediary nodes between two nodes
inline std::vector<int> find_shared_intermediaries(int node_a, int node_b) {
    std::vector<int> intermediaries;
    
    auto it_a = adjacency.find(node_a);
    auto it_b = adjacency.find(node_b);
    
    if (it_a == adjacency.end() || it_b == adjacency.end()) {
        return intermediaries;
    }
    
    // Get neighbors
    std::set<int> neighbors_a, neighbors_b;
    for (int edge_id : it_a->second) {
        neighbors_a.insert(edges[edge_id].b);
    }
    for (int edge_id : it_b->second) {
        neighbors_b.insert(edges[edge_id].b);
    }
    
    // Find intersection
    std::set_intersection(neighbors_a.begin(), neighbors_a.end(),
                         neighbors_b.begin(), neighbors_b.end(),
                         std::back_inserter(intermediaries));
    
    return intermediaries;
}

// ==================== CONTEXT FIELD ====================

inline std::vector<float> context_field;  // Activation potentials for all nodes

inline void init_context_field() {
    context_field.resize(nodes.size(), 0.0f);
}

inline void activate_node(int node_id, float activation = 1.0f) {
    if (context_field.empty()) init_context_field();
    if (node_id >= 0 && node_id < (int)context_field.size()) {
        context_field[node_id] = activation;
    }
}

// ==================== LEAP LOGIC CONSTANTS ====================

// Leap creation thresholds
const float LEAP_THRESHOLD = 0.4f;          // Minimum score to create leap
const float TH_SUPPORT = 3.0f;              // Minimum crowd support (# of examples)
const int TH_DIVERSITY = 2;                 // Minimum distinct subjects
const float TH_MARGIN = 1.0f;               // Best must beat second by this margin

// Promotion/decay thresholds
const float PROMOTE_THRESHOLD = 0.7f;       // Weight to promote LEAP to EXACT
const int MIN_SUCCESSES = 3;                // Minimum successes before promotion
const float MIN_WEIGHT = 0.2f;              // Below this, LEAP gets pruned
const double DECAY_TIME = 3600.0 * 24 * 7; // 1 week in seconds

// Path scoring weights
const float KAPPA = 0.5f;   // Bonus for EXACT edges
const float MU = 0.7f;      // Penalty for LEAP edges
const float SIGMA = 0.2f;   // Coherence bonus

// Compute leap score: similarity × activation × (1 - edge_exists)
inline float compute_leap_score(int node_a, int node_b) {
    if (context_field.empty()) {
        // If no context field, use similarity only
        return compute_similarity(node_a, node_b);
    }
    if (node_a < 0 || node_a >= (int)context_field.size()) return 0.0f;
    if (node_b < 0 || node_b >= (int)context_field.size()) return 0.0f;
    
    // Check if EXACT edge already exists
    auto it = adjacency.find(node_a);
    if (it != adjacency.end()) {
        for (int edge_id : it->second) {
            if (edges[edge_id].b == node_b && edges[edge_id].is_exact()) {
                return 0.0f;  // EXACT exists, no need for leap
            }
        }
    }
    
    // Compute components
    float similarity = compute_similarity(node_a, node_b);
    float activation = context_field[node_a] + context_field[node_b];
    float not_exists = 1.0f;  // Since we already checked it doesn't exist above
    
    return similarity * activation * not_exists;
}

// Create a LEAP connection using crowd-support criteria
inline int create_leap_with_support(int subject_node, int connector_node, uint32_t rel = Rel::TEMPORAL) {
    // Gather candidates from crowd support
    auto candidates = gather_leap_candidates(connector_node);
    
    if (candidates.empty()) {
        return -1;
    }
    
    // Get best and second-best
    const LeapCandidate& best = candidates[0];
    const LeapCandidate& second = candidates.size() > 1 ? candidates[1] : LeapCandidate();
    
    // Check crowd-support criteria
    bool has_support = best.support >= TH_SUPPORT;
    bool has_diversity = best.distinct_subjects >= TH_DIVERSITY;
    bool has_margin = best.support >= second.support + TH_MARGIN;
    
    if (has_support && has_diversity && has_margin) {
        // Create LEAP with confidence based on support
        float leap_weight = std::min(0.8f, best.support / 10.0f);
        int edge_id = connect(subject_node, best.target_node, rel, leap_weight, EdgeType::LEAP);
        
        if (edge_id >= 0) {
            edges[edge_id].leap_score = best.support;
            
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
    
    std::cout << "   ⚠️  [LEAP REJECTED] Insufficient support "
              << "(support:" << best.support << "/" << TH_SUPPORT
              << ", diversity:" << best.distinct_subjects << "/" << TH_DIVERSITY
              << ", margin:" << (best.support - second.support) << "/" << TH_MARGIN << ")\n";
    
    return -1;
}

// Create a LEAP connection if score is high enough (original fallback method)
inline int create_leap_if_needed(int node_a, int node_b, uint32_t rel = Rel::TEMPORAL) {
    float score = compute_leap_score(node_a, node_b);
    
    if (score > LEAP_THRESHOLD) {
        int edge_id = connect(node_a, node_b, rel, score, EdgeType::LEAP);
        if (edge_id >= 0) {
            edges[edge_id].leap_score = score;
        }
        return edge_id;
    }
    
    return -1;
}

// ==================== PATH SCORING ====================

// Score a reasoning path (prefer EXACT, penalize LEAP)
inline double score_path(const std::vector<int>& path) {
    if (path.size() < 2) return 0.0;
    
    double score = 0.0;
    int n_exact = 0, n_leap = 0;
    double total_weight = 0.0;
    
    // Score edges in path
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        auto it = adjacency.find(path[i]);
        if (it != adjacency.end()) {
            for (int edge_id : it->second) {
                if (edges[edge_id].b == path[i+1]) {
                    const auto& edge = edges[edge_id];
                    
                    // Log weight component
                    score += std::log(1.0 + edge.w);
                    total_weight += edge.w;
                    
                    // Track edge types
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
    
    // Compute final score with bonuses/penalties
    score += KAPPA * n_exact;      // Bonus for EXACT edges
    score -= MU * n_leap;          // Penalty for LEAP edges
    
    // Coherence bonus (path doesn't loop)
    std::set<int> unique_nodes(path.begin(), path.end());
    double coherence = (double)unique_nodes.size() / path.size();
    score += SIGMA * coherence;
    
    return score;
}

// Find EXACT chain between nodes
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
        
        // Only follow EXACT edges
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
        // Get outgoing edges from current node
        auto it = adjacency.find(current);
        if (it == adjacency.end() || it->second.empty()) break;
        
        // Find best edge (prefer EXACT over LEAP)
        int best_edge = -1;
        float best_weight = 0.0f;
        bool found_exact = false;
        
        for (int edge_id : it->second) {
            if (edge_id >= 0 && edge_id < (int)edges.size()) {
                float weight = edges[edge_id].w;
                bool is_exact = edges[edge_id].is_exact();
                
                // Strongly prefer EXACT edges
                if (prefer_exact && is_exact && !found_exact) {
                    // First EXACT edge found - reset comparison
                    best_weight = weight;
                    best_edge = edge_id;
                    found_exact = true;
                } else if (prefer_exact && found_exact && is_exact) {
                    // Compare among EXACT edges
                    if (weight > best_weight) {
                        best_weight = weight;
                        best_edge = edge_id;
                    }
                } else if (!prefer_exact || !found_exact) {
                    // No EXACT found yet, consider all edges
                    if (weight > best_weight) {
                        best_weight = weight;
                        best_edge = edge_id;
                        if (is_exact) found_exact = true;
                    }
                }
            }
        }
        
        if (best_edge < 0) break;
        
        // Follow edge
        int next_node = edges[best_edge].b;
        path.push_back(next_node);
        current = next_node;
        
        // Touch the node
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
        
        // Build probability distribution
        std::vector<float> probs;
        std::vector<int> next_nodes;
        float total = 0.0f;
        
        for (int edge_id : it->second) {
            if (edge_id >= 0 && edge_id < (int)edges.size()) {
                float weight = edges[edge_id].w;
                
                // Boost cross-modal
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
        
        // Sample
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
        
        // Diffuse along edges
        for (const auto& edge : edges) {
            if (edge.a >= 0 && edge.a < (int)context_field.size() &&
                edge.b >= 0 && edge.b < (int)context_field.size()) {
                
                float flow = edge.w * (context_field[edge.a] - context_field[edge.b]);
                
                // Modulate by modality match
                if (nodes[edge.a].sensory_type != nodes[edge.b].sensory_type) {
                    flow *= 0.5f;  // Slower cross-modal diffusion
                }
                
                delta[edge.b] += tau * flow;
                delta[edge.a] -= tau * flow;
            }
        }
        
        // Apply diffusion
        for (size_t i = 0; i < context_field.size(); ++i) {
            context_field[i] += delta[i];
            context_field[i] *= 0.95f;  // Decay
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

