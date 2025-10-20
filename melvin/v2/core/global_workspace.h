#pragma once

#include "types_v2.h"
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>

namespace melvin::v2 {

// ============================================================================
// GLOBAL WORKSPACE - Central "Conscious" Blackboard (GWT)
// ============================================================================

/**
 * GlobalWorkspace implements Global Workspace Theory (Baars, 1988).
 * 
 * It's a central blackboard where:
 * - Modules POST thoughts (percepts, goals, hypotheses, actions)
 * - Only high-salience thoughts remain (capacity-limited like consciousness)
 * - All modules can READ the current conscious content
 * - Provides thread-safe snapshot mechanism
 * 
 * Key properties:
 * - Capacity-limited (top-K most salient thoughts)
 * - Automatic salience decay
 * - Thread-safe concurrent access
 * - Goal management
 */
class GlobalWorkspace {
public:
    struct Config {
        size_t max_thoughts;           // Maximum thoughts in workspace (default: 20)
        float salience_decay_rate;     // Decay per second (default: 0.95)
        float min_salience;            // Threshold for removal (default: 0.1)
        bool enable_logging;           // Log all posts
        
        Config() 
            : max_thoughts(20), salience_decay_rate(0.95f), 
              min_salience(0.1f), enable_logging(false) {}
    };
    
    // Snapshot for lock-free reading
    struct Snapshot {
        std::vector<Thought> thoughts;
        std::string current_goal;
        float goal_priority;
        std::vector<WMSlot> wm_slots;           // From working memory
        NeuromodState neuromod;                 // Current neuromodulator state
        Timestamp captured_at;
        size_t tick_count;
        
        Snapshot() : goal_priority(0.0f), captured_at(0), tick_count(0) {}
    };
    
    explicit GlobalWorkspace(const Config& config = Config());
    ~GlobalWorkspace();
    
    // ========================================================================
    // THOUGHT MANAGEMENT
    // ========================================================================
    
    /**
     * Post a thought to the workspace
     * - If thought.salience > min, added to workspace
     * - If workspace full, lowest-salience thought evicted
     * - Thread-safe
     */
    void post(const Thought& thought);
    
    /**
     * Post multiple thoughts (atomic batch)
     */
    void post_batch(const std::vector<Thought>& thoughts);
    
    /**
     * Query thoughts by type
     * - Returns all thoughts matching type with salience >= min_salience
     * - Thread-safe (returns copy)
     */
    std::vector<Thought> query(const std::string& type, float min_salience = 0.0f) const;
    
    /**
     * Get all thoughts (sorted by salience descending)
     */
    std::vector<Thought> get_all_thoughts() const;
    
    /**
     * Get most salient thought
     */
    Thought get_most_salient() const;
    
    /**
     * Clear all thoughts
     */
    void clear();
    
    // ========================================================================
    // GOAL MANAGEMENT
    // ========================================================================
    
    /**
     * Set current goal
     * - Goal influences attention (top-down bias)
     * - Higher priority = stronger influence
     */
    void set_goal(const std::string& goal, float priority = 1.0f);
    
    /**
     * Get current goal
     */
    std::string get_current_goal() const;
    
    /**
     * Get goal priority
     */
    float get_goal_priority() const;
    
    /**
     * Clear goal
     */
    void clear_goal();
    
    // ========================================================================
    // SNAPSHOT (Lock-Free Reading)
    // ========================================================================
    
    /**
     * Get snapshot of current workspace state
     * - Lock-free for readers
     * - Includes all current thoughts, goal, WM, neuromod state
     */
    Snapshot get_snapshot() const;
    
    /**
     * Update snapshot with external state (WM, neuromod)
     * - Called by UnifiedLoop after each tick
     */
    void update_snapshot_external(const std::vector<WMSlot>& wm_slots,
                                  const NeuromodState& neuromod);
    
    // ========================================================================
    // TICK (Time-Based Updates)
    // ========================================================================
    
    /**
     * Tick: decay salience, remove low-salience thoughts
     * - Called once per cognitive cycle
     * - dt = delta time in seconds
     */
    void tick(float dt);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        size_t total_posts;
        size_t total_evictions;
        size_t current_thought_count;
        float avg_salience;
        size_t tick_count;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    Config get_config() const;
    
private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================
    
    Config config_;
    
    // Thread-safe thought storage
    mutable std::mutex thoughts_mutex_;
    std::vector<Thought> thoughts_;
    uint64_t next_thought_id_;
    
    // Goal state
    mutable std::mutex goal_mutex_;
    std::string current_goal_;
    float goal_priority_;
    
    // Snapshot state (updated by update_snapshot_external)
    mutable std::mutex snapshot_mutex_;
    Snapshot current_snapshot_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
    size_t tick_count_;
    
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================
    
    // Sort thoughts by salience (descending)
    void sort_thoughts();
    
    // Evict lowest-salience thought
    void evict_lowest();
    
    // Apply decay to all thoughts
    void apply_decay(float factor);
    
    // Remove thoughts below threshold
    void prune_low_salience();
    
    // Generate unique thought ID
    uint64_t generate_thought_id();
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Create a percept thought
Thought make_percept_thought(NodeID object_id, float salience);

// Create a goal thought
Thought make_goal_thought(const std::string& goal_desc, float salience);

// Create a hypothesis thought
Thought make_hypothesis_thought(const std::vector<NodeID>& concept_refs, float salience);

// Create an action thought
Thought make_action_thought(const std::string& action, float salience);

// Pretty-print thought
std::string thought_to_string(const Thought& thought);

// Pretty-print workspace snapshot
std::string snapshot_to_string(const GlobalWorkspace::Snapshot& snapshot);

} // namespace melvin::v2

