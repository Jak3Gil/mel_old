#include "edge_decay.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Full Edge definition
struct Edge {
    uint64_t u, v;
    uint64_t locB;
    float weight;
    float w_core;
    float w_ctx;
    uint32_t count;
    
    // Extended fields for decay
    std::string last_use_date;  // ISO 8601
    uint32_t queries_routed = 0;
    uint32_t queries_attempted = 0;
    
    void update_frequency(uint64_t total_nodes) { (void)total_nodes; }
    
    float utility() const {
        if (queries_attempted == 0) return 0.0f;
        return static_cast<float>(queries_routed) / queries_attempted;
    }
};

namespace melvin {
namespace learning {

// Get current date (ISO 8601 format)
std::string EdgeDecay::current_date_iso() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    
    #ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
    #else
        localtime_r(&time_t_now, &tm_now);
    #endif
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm_now);
    return std::string(buffer);
}

// Compute days between two dates (simplified)
float EdgeDecay::days_between(const std::string& date1, const std::string& date2) {
    // Simplified: assume format YYYY-MM-DD
    // For production, use proper date library
    // For now, return 1.0 as placeholder
    (void)date1;
    (void)date2;
    return 1.0f;  // Placeholder
}

// Compute Ebbinghaus decay factor
float EdgeDecay::compute_decay_factor(float days, float lambda) {
    // Ebbinghaus formula: retention = exp(-λ * t)
    return std::exp(-lambda * days);
}

// Apply decay to all edges
DecayResult EdgeDecay::apply_decay(
    std::vector<::Edge>& edges,
    const std::string& current_date,
    const DecayOptions& opts
) {
    DecayResult result;
    
    // Calculate average weight before
    float sum_before = 0.0f;
    for (const auto& edge : edges) {
        sum_before += edge.weight;
    }
    result.avg_weight_before = edges.empty() ? 0.0f : sum_before / edges.size();
    
    if (opts.verbose) {
        std::cout << "\n[DECAY] Applying Ebbinghaus decay (λ=" << opts.lambda << ")...\n";
        std::cout << "  Current date: " << current_date << "\n";
        std::cout << "  Total edges: " << edges.size() << "\n";
        std::cout << "  Avg weight before: " << result.avg_weight_before << "\n";
    }
    
    // Apply decay to each edge
    std::vector<::Edge> surviving_edges;
    for (auto& edge : edges) {
        // If no last_use_date, assume it's new (don't decay)
        if (edge.last_use_date.empty()) {
            edge.last_use_date = current_date;
            surviving_edges.push_back(edge);
            continue;
        }
        
        // Compute days since last use
        float days = days_between(edge.last_use_date, current_date);
        
        // Apply decay
        float decay_factor = compute_decay_factor(days, opts.lambda);
        float new_weight = edge.weight * decay_factor;
        
        // Apply minimum threshold
        new_weight = std::max(new_weight, opts.w_min);
        
        // Prune if below minimum and pruning enabled
        if (opts.prune_low && new_weight <= opts.w_min) {
            result.edges_pruned++;
            continue;  // Skip this edge (prune it)
        }
        
        if (new_weight < edge.weight) {
            result.edges_decayed++;
        }
        
        edge.weight = new_weight;
        edge.w_core = new_weight;
        surviving_edges.push_back(edge);
    }
    
    // Replace with surviving edges
    edges = surviving_edges;
    
    // Calculate average weight after
    float sum_after = 0.0f;
    for (const auto& edge : edges) {
        sum_after += edge.weight;
    }
    result.avg_weight_after = edges.empty() ? 0.0f : sum_after / edges.size();
    
    if (opts.verbose) {
        std::cout << "  Edges decayed: " << result.edges_decayed << "\n";
        std::cout << "  Edges pruned: " << result.edges_pruned << "\n";
        std::cout << "  Avg weight after: " << result.avg_weight_after << "\n";
        std::cout << "  Retention: " << std::fixed << std::setprecision(1) 
                  << (result.avg_weight_after / result.avg_weight_before * 100.0f) << "%\n\n";
    }
    
    return result;
}

// Rehearse edges (boost/penalize based on success)
void EdgeDecay::rehearse_edges(
    std::vector<::Edge>& edges,
    const std::vector<uint64_t>& path,
    bool success,
    float alpha,
    float beta
) {
    if (path.size() < 2) {
        return;  // Need at least 2 nodes for an edge
    }
    
    std::string today = current_date_iso();
    
    // For each edge in the path
    for (size_t i = 0; i + 1 < path.size(); i++) {
        uint64_t from = path[i];
        uint64_t to = path[i + 1];
        
        // Find the edge
        for (auto& edge : edges) {
            if (edge.u == from && edge.v == to) {
                edge.queries_attempted++;
                
                if (success) {
                    // Boost weight (anti-decay)
                    edge.weight = std::min(edge.weight + alpha, 1.0f);
                    edge.queries_routed++;
                    edge.last_use_date = today;
                } else {
                    // Penalize weight
                    edge.weight = std::max(edge.weight * beta, 0.1f);
                }
                
                edge.w_core = edge.weight;
                break;
            }
        }
    }
}

} // namespace learning
} // namespace melvin

