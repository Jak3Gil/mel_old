#pragma once

#include <cstddef>
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace melvin {
namespace adaptive {

/**
 * ⚡ Adaptive EXACT Window Configuration
 * 
 * Controls how Melvin's brain creates temporal connections based on node novelty.
 * 
 * Behavioral Rules:
 * - New/weak nodes → larger window → broader temporal context
 * - Strong/familiar nodes → smaller window → efficient connections
 * - Window size adapts between N_min and N_max
 * 
 * Philosophy:
 * New experiences should bind to more surrounding context (curiosity).
 * Familiar patterns can be compact and efficient (consolidation).
 */
struct AdaptiveWindowConfig {
    // ========================================================================
    // WINDOW SIZE BOUNDS
    // ========================================================================
    
    /**
     * Minimum window size - even the strongest nodes connect to at least this many
     * Default: 5 (always maintain some temporal context)
     */
    size_t N_min = 5;
    
    /**
     * Maximum window size - brand new nodes can connect to up to this many
     * Default: 50 (capture rich context for novel experiences)
     */
    size_t N_max = 50;
    
    // ========================================================================
    // NOVELTY & STRENGTH CALCULATION
    // ========================================================================
    
    /**
     * Novelty decay rate
     * novelty = 1 / (1 + activation_count * novelty_decay)
     * Higher values make novelty decay faster with experience
     * Default: 0.1
     */
    float novelty_decay = 0.1f;
    
    /**
     * Novelty boost multiplier
     * Amplifies window size for high-novelty nodes
     * window_size *= (1 + novelty * novelty_boost)
     * Default: 0.5 (50% boost for completely novel nodes)
     */
    float novelty_boost = 0.5f;
    
    /**
     * Strength sensitivity
     * Controls how strongly node strength affects window size
     * Higher values = stronger nodes get much smaller windows
     * Default: 1.0
     */
    float strength_sensitivity = 1.0f;
    
    // ========================================================================
    // TEMPORAL WEIGHT DECAY
    // ========================================================================
    
    /**
     * Temporal decay lambda (λ)
     * Controls how edge weight decays with temporal distance
     * weight = exp(-lambda * distance)
     * 
     * Examples:
     * - lambda = 0.05: slow decay, distant nodes still relevant
     * - lambda = 0.10: moderate decay (default)
     * - lambda = 0.20: fast decay, focus on recent context
     * 
     * Default: 0.1
     */
    float temporal_decay_lambda = 0.1f;
    
    /**
     * Minimum edge weight threshold
     * Edges below this weight won't be created
     * Default: 0.01 (1% of maximum strength)
     */
    float min_edge_weight = 0.01f;
    
    // ========================================================================
    // STRENGTH CALCULATION
    // ========================================================================
    
    /**
     * How to calculate node strength (normalized 0-1)
     * Options:
     * - 0: Average outgoing edge weight
     * - 1: Activation frequency (activations / max_activations)
     * - 2: Weighted combination of both
     * Default: 2
     */
    int strength_mode = 2;
    
    /**
     * Blend factor for combined strength mode
     * strength = edge_weight * blend + activation_freq * (1 - blend)
     * Default: 0.6 (60% edge weights, 40% activation frequency)
     */
    float strength_blend = 0.6f;
    
    // ========================================================================
    // PERFORMANCE OPTIMIZATION
    // ========================================================================
    
    /**
     * Enable adaptive windowing
     * If false, uses fixed window of N_min
     * Default: true
     */
    bool enable_adaptive = true;
    
    /**
     * Cache window sizes for nodes
     * Speeds up repeated calculations
     * Default: true
     */
    bool cache_window_sizes = true;
    
    /**
     * Recalculate window size every N activations
     * Lower = more responsive to changes
     * Higher = better performance
     * Default: 10
     */
    size_t recalc_interval = 10;
    
    // ========================================================================
    // STATISTICS & DEBUGGING
    // ========================================================================
    
    /**
     * Enable detailed statistics tracking
     * Default: false (for performance)
     */
    bool enable_stats = false;
    
    /**
     * Log window size calculations
     * Default: false
     */
    bool verbose_logging = false;
    
    // ========================================================================
    // HELPER METHODS
    // ========================================================================
    
    /**
     * Calculate novelty score for a node
     * Returns: 0.0 (well-known) to 1.0 (brand new)
     */
    float calculate_novelty(uint64_t activation_count) const {
        return 1.0f / (1.0f + activation_count * novelty_decay);
    }
    
    /**
     * Calculate adaptive window size
     * 
     * @param novelty Node novelty (0-1)
     * @param strength Node strength (0-1)
     * @return Window size between N_min and N_max
     */
    size_t calculate_window_size(float novelty, float strength) const {
        if (!enable_adaptive) {
            return N_min;
        }
        
        // Base window: inverse of strength
        // Strong nodes (strength → 1) get smaller window
        // Weak nodes (strength → 0) get larger window
        float base_factor = 1.0f - (strength * strength_sensitivity);
        base_factor = std::max(0.0f, std::min(1.0f, base_factor));
        
        // Apply novelty boost
        float novelty_factor = 1.0f + novelty * novelty_boost;
        
        // Calculate window size
        float window_range = static_cast<float>(N_max - N_min);
        float window_size_f = N_min + (window_range * base_factor * novelty_factor);
        
        // Clamp to bounds
        size_t window_size = static_cast<size_t>(window_size_f);
        window_size = std::max(N_min, std::min(N_max, window_size));
        
        return window_size;
    }
    
    /**
     * Calculate edge weight based on temporal distance
     * 
     * @param distance Temporal distance (0 = current, 1 = previous, etc.)
     * @return Weight value (0-1)
     */
    float calculate_temporal_weight(size_t distance) const {
        float weight = std::exp(-temporal_decay_lambda * distance);
        return weight < min_edge_weight ? 0.0f : weight;
    }
    
    /**
     * Validate configuration
     * Returns true if all parameters are valid
     */
    bool validate() const {
        if (N_min < 1 || N_min > N_max) return false;
        if (N_max > 10000) return false;  // Sanity check
        if (novelty_decay < 0.0f) return false;
        if (novelty_boost < 0.0f) return false;
        if (strength_sensitivity < 0.0f) return false;
        if (temporal_decay_lambda < 0.0f) return false;
        if (min_edge_weight < 0.0f || min_edge_weight > 1.0f) return false;
        if (strength_mode < 0 || strength_mode > 2) return false;
        if (strength_blend < 0.0f || strength_blend > 1.0f) return false;
        return true;
    }
    
    /**
     * Print configuration summary
     */
    void print() const {
        printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
        printf("║  ADAPTIVE WINDOW CONFIGURATION                                ║\n");
        printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
        printf("  Window Range: %zu - %zu nodes\n", N_min, N_max);
        printf("  Novelty Decay: %.3f\n", novelty_decay);
        printf("  Novelty Boost: %.3f\n", novelty_boost);
        printf("  Strength Sensitivity: %.3f\n", strength_sensitivity);
        printf("  Temporal Decay (λ): %.3f\n", temporal_decay_lambda);
        printf("  Min Edge Weight: %.3f\n", min_edge_weight);
        printf("  Adaptive Enabled: %s\n", enable_adaptive ? "Yes" : "No");
        printf("\n");
    }
};

/**
 * Statistics tracker for adaptive window system
 */
struct AdaptiveWindowStats {
    uint64_t total_windows_calculated = 0;
    uint64_t total_edges_created = 0;
    uint64_t total_edges_skipped = 0;  // Below min weight threshold
    
    // Window size distribution
    size_t min_window_seen = 1000;
    size_t max_window_seen = 0;
    double avg_window_size = 0.0;
    
    // Temporal weight statistics
    double avg_edge_weight = 0.0;
    double min_edge_weight = 1.0;
    double max_edge_weight = 0.0;
    
    void update_window_size(size_t window_size) {
        total_windows_calculated++;
        min_window_seen = std::min(min_window_seen, window_size);
        max_window_seen = std::max(max_window_seen, window_size);
        
        // Running average
        double n = static_cast<double>(total_windows_calculated);
        avg_window_size = (avg_window_size * (n - 1) + window_size) / n;
    }
    
    void update_edge_weight(float weight) {
        if (weight > 0.0f) {
            total_edges_created++;
            min_edge_weight = std::min(min_edge_weight, static_cast<double>(weight));
            max_edge_weight = std::max(max_edge_weight, static_cast<double>(weight));
            
            // Running average
            double n = static_cast<double>(total_edges_created);
            avg_edge_weight = (avg_edge_weight * (n - 1) + weight) / n;
        } else {
            total_edges_skipped++;
        }
    }
    
    void print() const {
        printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
        printf("║  ADAPTIVE WINDOW STATISTICS                                   ║\n");
        printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
        printf("  Windows Calculated: %llu\n", total_windows_calculated);
        printf("  Window Size Range: %zu - %zu (avg: %.1f)\n", 
               min_window_seen, max_window_seen, avg_window_size);
        printf("\n");
        printf("  Edges Created: %llu\n", total_edges_created);
        printf("  Edges Skipped (low weight): %llu\n", total_edges_skipped);
        printf("  Edge Weight Range: %.3f - %.3f (avg: %.3f)\n",
               min_edge_weight, max_edge_weight, avg_edge_weight);
        printf("\n");
    }
    
    void reset() {
        *this = AdaptiveWindowStats();
    }
};

}} // namespace melvin::adaptive

