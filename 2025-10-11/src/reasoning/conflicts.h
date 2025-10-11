/*
 * MELVIN CONFLICT HANDLING - LEAP vs EXACT Arbitration
 * 
 * Provides:
 * - Conflict detection between EXACT and LEAP edges
 * - Penalty system for contradictory LEAPs
 * - Abstention when confidence is low
 * - Per-(X,C) histogram tracking
 */

#pragma once
#include "../../include/melvin_storage.h"
#include "../util/telemetry.h"
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

namespace melvin_conflicts {

using namespace melvin_storage;

// ==================== CONFIGURATION ====================

// Penalty applied to LEAPs that conflict with new EXACT edges
constexpr float PENALTY = 1.0f;

// LEAP must be THIS strong to override a conflicting EXACT
constexpr float OVERRIDE_THRESHOLD = 8.0f;

// Abstain if best answer doesn't beat second by this margin
constexpr float ABSTAIN_MARGIN = 1.0f;

// ==================== CONFLICT TRACKING ====================

// Per-(X,C) histogram: tracks Y values and their total weight
// Key: (subject_node, connector_node)
// Value: map of (target_node -> accumulated_weight)
static std::map<std::pair<int,int>, std::map<int, float>> conflict_histograms;

// Record an EXACT edge into conflict histogram
inline void record_exact(int subject, int connector, int object, float weight = 1.0f) {
    auto key = std::make_pair(subject, connector);
    conflict_histograms[key][object] += weight;
}

// Get the dominant EXACT target for (subject, connector)
inline int get_dominant_exact(int subject, int connector, float* out_weight = nullptr) {
    auto key = std::make_pair(subject, connector);
    auto hist_it = conflict_histograms.find(key);
    
    if (hist_it == conflict_histograms.end()) {
        return -1;
    }
    
    int best_target = -1;
    float best_weight = 0.0f;
    
    for (const auto& [target, weight] : hist_it->second) {
        if (weight > best_weight) {
            best_weight = weight;
            best_target = target;
        }
    }
    
    if (out_weight) *out_weight = best_weight;
    return best_target;
}

// Check if a LEAP conflicts with established EXACT knowledge
inline bool has_conflict(int subject, int connector, int leap_target, int* conflicting_target = nullptr) {
    float dominant_weight = 0.0f;
    int dominant = get_dominant_exact(subject, connector, &dominant_weight);
    
    if (dominant >= 0 && dominant != leap_target && dominant_weight >= ABSTAIN_MARGIN) {
        if (conflicting_target) *conflicting_target = dominant;
        return true;
    }
    
    return false;
}

// Penalize LEAPs that conflict with newly added EXACT edges
inline void penalize_conflicts(int subject, int connector, int new_exact_target) {
    // Find all LEAP edges from subject
    auto adj_it = adjacency.find(subject);
    if (adj_it == adjacency.end()) return;
    
    for (int edge_id : adj_it->second) {
        if (edge_id < 0 || edge_id >= (int)edges.size()) continue;
        
        auto& edge = edges[edge_id];
        
        // Only penalize LEAP edges that conflict
        if (edge.is_leap() && edge.b != new_exact_target) {
            // Check if this LEAP goes through the same connector
            // For now, we penalize all LEAPs from this subject that don't match
            edge.w = std::max(0.0f, edge.w - PENALTY);
            
            if (edge.w < 0.1f) {
                // Log decay
                std::stringstream ss_w;
                ss_w << std::fixed << std::setprecision(2) << edge.w;
                
                LOG_LEAP_DECAY(
                    nodes[subject].data,
                    nodes[edge.b].data,
                    "conflict_with_exact"
                );
            }
        }
    }
}

// Build conflict histograms from existing edges
inline void build_conflict_histograms() {
    conflict_histograms.clear();
    
    for (const auto& edge : edges) {
        if (edge.is_exact()) {
            // Try to infer connector (middle node in 3-hop patterns)
            // For direct edges, we'll track them without connector
            record_exact(edge.a, -1, edge.b, edge.w);
        }
    }
}

// ==================== ABSTENTION ====================

struct Answer {
    int node = -1;
    float confidence = 0.0f;
    bool is_leap = false;
    float support = 0.0f;
    
    bool operator<(const Answer& other) const {
        return confidence < other.confidence;
    }
};

// Should we abstain from answering?
inline bool should_abstain(const std::vector<Answer>& answers, std::string* reason = nullptr) {
    if (answers.empty()) {
        if (reason) *reason = "no_answers";
        return true;
    }
    
    // Get top 2 answers
    std::vector<Answer> sorted = answers;
    std::sort(sorted.rbegin(), sorted.rend());
    
    const Answer& best = sorted[0];
    const Answer& second = sorted.size() > 1 ? sorted[1] : Answer();
    
    // Check margin
    if (best.confidence < second.confidence + ABSTAIN_MARGIN) {
        if (reason) {
            std::stringstream ss;
            ss << "ambiguous_margin_" << std::fixed << std::setprecision(2) 
               << (best.confidence - second.confidence);
            *reason = ss.str();
        }
        return true;
    }
    
    // Check if LEAP has conflict
    if (best.is_leap && best.support < OVERRIDE_THRESHOLD) {
        // Would need more sophisticated conflict check here
        // For now, low-confidence LEAPs might be abstained
        if (best.confidence < 0.5f) {
            if (reason) *reason = "low_confidence_leap";
            return true;
        }
    }
    
    return false;
}

// ==================== STATISTICS ====================

inline void print_conflict_stats() {
    std::cout << "\n📊 Conflict Statistics:\n";
    std::cout << "   Total (subject,connector) patterns: " << conflict_histograms.size() << "\n";
    
    int multi_target = 0;
    for (const auto& [key, targets] : conflict_histograms) {
        if (targets.size() > 1) multi_target++;
    }
    
    std::cout << "   Patterns with multiple targets: " << multi_target << "\n";
    
    if (multi_target > 0) {
        std::cout << "\n   Ambiguous patterns:\n";
        int shown = 0;
        for (const auto& [key, targets] : conflict_histograms) {
            if (targets.size() > 1 && shown < 5) {
                auto [subj, conn] = key;
                std::cout << "      (" << (subj >= 0 && subj < (int)nodes.size() ? nodes[subj].data : "?")
                         << ", connector) -> ";
                
                bool first = true;
                for (const auto& [target, weight] : targets) {
                    if (!first) std::cout << ", ";
                    std::cout << nodes[target].data << "(" << std::fixed << std::setprecision(1) << weight << ")";
                    first = false;
                }
                std::cout << "\n";
                shown++;
            }
        }
    }
}

} // namespace melvin_conflicts

