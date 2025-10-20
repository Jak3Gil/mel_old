#pragma once

#include "../core/types_v2.h"
#include "../evolution/genome.h"
#include <vector>
#include <memory>
#include <optional>

namespace melvin::v2::memory {

// ============================================================================
// WORKING MEMORY - PFC-Like Short-Term Buffer
// ============================================================================

/**
 * WorkingMemory implements a PFC-like buffer for maintaining active concepts.
 * 
 * Key features:
 * - K slots (typically 7±2, from genome)
 * - Gating mechanism (neuromodulator-controlled)
 * - Precision decay over time
 * - Binding tags (roles: "agent", "object", "goal")
 * - Refresh mechanism (rehearsal)
 * 
 * Based on:
 * - Cowan (2001): ~4 chunks capacity
 * - Baddeley & Hitch (1974): Working memory model
 * - O'Reilly & Frank (2006): Gating in PFC
 */
class WorkingMemory {
public:
    struct Config {
        size_t slot_count;           // Number of WM slots (from genome: 3-12)
        float decay_tau;             // Time constant for precision decay (seconds)
        float gating_threshold;      // Minimum salience to gate into WM
        float precision_decay_rate;  // Per-second decay (from genome)
        float refresh_rate;          // Boost from rehearsal
        float binding_strength;      // How strongly items bind to tags
        bool enable_logging;         // Debug output
        
        Config()
            : slot_count(7), decay_tau(5.0f), gating_threshold(0.5f),
              precision_decay_rate(0.95f), refresh_rate(0.2f),
              binding_strength(0.7f), enable_logging(false) {}
    };
    
    WorkingMemory(const evolution::Genome& genome);
    explicit WorkingMemory(const Config& config);
    ~WorkingMemory();
    
    // ========================================================================
    // GATING (Add/Update/Remove)
    // ========================================================================
    
    /**
     * Try to add item to working memory
     * - Requires salience > gating_threshold
     * - If slots full, may evict lowest precision item
     * - Controlled by neuromodulators (DA for gating strength)
     * 
     * Returns: true if added, false if rejected
     */
    bool try_add(NodeID item, const std::string& tag, float salience, float plasticity_rate = 1.0f);
    
    /**
     * Update existing item (refresh/rehearsal)
     * - Boosts precision
     * - Resets decay timer
     */
    bool refresh(NodeID item);
    
    /**
     * Remove item from working memory
     */
    bool remove(NodeID item);
    
    /**
     * Remove item from specific slot
     */
    bool remove_slot(size_t slot_idx);
    
    /**
     * Clear all slots
     */
    void clear();
    
    // ========================================================================
    // QUERY
    // ========================================================================
    
    /**
     * Get all active slots
     */
    std::vector<WMSlot> get_active_slots() const;
    
    /**
     * Get slots by binding tag
     */
    std::vector<WMSlot> get_slots_by_tag(const std::string& tag) const;
    
    /**
     * Check if item is in WM
     */
    bool contains(NodeID item) const;
    
    /**
     * Get slot for item (if exists)
     */
    std::optional<WMSlot> get_slot(NodeID item) const;
    
    /**
     * Get slot by index
     */
    std::optional<WMSlot> get_slot_at(size_t idx) const;
    
    /**
     * Get number of active slots
     */
    size_t active_count() const;
    
    /**
     * Check if WM is full
     */
    bool is_full() const;
    
    // ========================================================================
    // TICK (Time-Based Updates)
    // ========================================================================
    
    /**
     * Tick: apply precision decay, remove expired items
     * - Called once per cognitive cycle
     * - dt = delta time in seconds
     */
    void tick(float dt);
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    Config get_config() const;
    
    /**
     * Set gating threshold dynamically (neuromodulator-controlled)
     */
    void set_gating_threshold(float threshold);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        size_t total_gates;          // Total items gated in
        size_t total_refreshes;      // Total refresh operations
        size_t total_evictions;      // Items evicted due to capacity
        size_t total_decays;         // Items removed due to decay
        float avg_precision;         // Average precision of active items
        float avg_lifetime;          // Average time items stay in WM
        size_t tick_count;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
private:
    Config config_;
    std::vector<WMSlot> slots_;
    Stats stats_;
    size_t tick_count_;
    
    // Helpers
    int find_slot_for_item(NodeID item) const;
    int find_empty_slot() const;
    int find_lowest_precision_slot() const;
    void apply_decay(float dt);
    void prune_expired();
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Extract WM config from genome
 */
WorkingMemory::Config extract_wm_config(const evolution::Genome& genome);

/**
 * Pretty-print WM state
 */
std::string wm_state_to_string(const std::vector<WMSlot>& slots);

} // namespace melvin::v2::memory

