#pragma once

#include "melvin_vision.h"
#include "melvin_graph.h"
#include <vector>
#include <string>
#include <deque>

namespace melvin {
namespace uca {

// ============================================================================
// BIOLOGICAL ANALOG: FEF + Superior Colliculus (Attention Control)
// ============================================================================

/**
 * Attention control mode
 */
enum AttentionMode {
    BOTTOM_UP,        // Stimulus-driven (SC, saliency)
    TOP_DOWN,         // Goal-driven (FEF, task-relevant)
    BALANCED,         // Combination (default)
    EXPLORATION       // Curiosity-driven (dopaminergic)
};

/**
 * Focus target: What Melvin is currently attending to
 */
struct FocusTarget {
    std::string id;                    // Object or concept ID
    std::string type;                  // "visual", "audio", "concept", "internal"
    
    // Spatial location (for visual targets)
    int center_x, center_y;
    int bbox_x1, bbox_y1, bbox_x2, bbox_y2;
    
    // Attention scores
    float saliency;                    // Bottom-up
    float relevance;                   // Top-down
    float curiosity;                   // Novelty
    float focus_strength;              // Combined F score
    
    // Temporal properties
    int duration_frames;               // How long focused
    double start_time;
    
    // Graph links
    uint64_t graph_node_id;
    std::vector<uint64_t> active_concepts; // Related concepts
    
    FocusTarget() : center_x(0), center_y(0), bbox_x1(0), bbox_y1(0),
                    bbox_x2(0), bbox_y2(0), saliency(0), relevance(0),
                    curiosity(0), focus_strength(0), duration_frames(0),
                    start_time(0), graph_node_id(0) {}
    
    bool is_valid() const { return !id.empty(); }
};

/**
 * Saccade: Attention shift from one target to another
 */
struct Saccade {
    FocusTarget from;
    FocusTarget to;
    double timestamp;
    std::string reason;      // Why the shift? "high_saliency", "goal_relevant", "curious"
    
    Saccade() : timestamp(0) {}
};

// ============================================================================
// MELVIN FOCUS - Attention Control System
// ============================================================================

/**
 * MelvinFocus: FEF + Superior Colliculus analog
 * 
 * Implements:
 * - Saccade control (attention shifts)
 * - Focus maintenance (persistent tracking)
 * - Inhibition of return (don't revisit too soon)
 * - Goal-directed modulation (top-down biasing)
 * - Active concept broadcast (what's being thought about)
 * 
 * Biological parallels:
 * - select_focus() → FEF/SC saccade generation
 * - maintain_focus() → Pulvinar sustained attention
 * - inhibit_recently_focused() → IOR (inhibition of return)
 * - broadcast_active_concepts() → Global workspace theory
 */
class MelvinFocus {
public:
    struct Config {
        AttentionMode mode;               // Default attention mode
        
        // Focus selection weights
        float alpha_saliency;             // Bottom-up (default: 0.4)
        float beta_relevance;             // Top-down (default: 0.3)
        float gamma_curiosity;            // Exploration (default: 0.3)
        
        // Focus dynamics
        float inertia_bonus;              // Resist switching (default: 0.15)
        float switch_threshold;           // Min F_new/F_current to switch (default: 1.15)
        int min_focus_duration;           // Frames before switch (default: 5)
        int max_focus_duration;           // Force switch after (default: 100)
        
        // Inhibition of return
        bool enable_ior;                  // Suppress recent targets
        int ior_duration;                 // Frames to suppress (default: 30)
        float ior_penalty;                // Suppression strength (default: 0.5)
        
        // Working memory
        int working_memory_size;          // Recent focus history (default: 7)
        
        Config() : mode(BALANCED),
                   alpha_saliency(0.4f), beta_relevance(0.3f), gamma_curiosity(0.3f),
                   inertia_bonus(0.15f), switch_threshold(1.15f),
                   min_focus_duration(5), max_focus_duration(100),
                   enable_ior(true), ior_duration(30), ior_penalty(0.5f),
                   working_memory_size(7) {}
    };
    
    MelvinFocus(MelvinGraph* graph, const Config& config = Config());
    ~MelvinFocus();
    
    // ========================================================================
    // FOCUS SELECTION
    // ========================================================================
    
    /**
     * Select focus target from visual candidates
     * Implements FEF/SC saccade generation
     */
    FocusTarget select_focus(const std::vector<VisualNode>& candidates);
    
    /**
     * Update current focus (called every frame)
     */
    void update_focus(const std::vector<VisualNode>& current_objects);
    
    /**
     * Force focus shift to specific target (external control)
     */
    void force_focus(const FocusTarget& target);
    
    /**
     * Release current focus (return to distributed attention)
     */
    void release_focus();
    
    // ========================================================================
    // TOP-DOWN CONTROL
    // ========================================================================
    
    /**
     * Set active goal to bias attention
     * Example: "find fire" → boost relevance of red/hot objects
     */
    void set_goal(const std::string& goal);
    
    /**
     * Set active concepts (from reasoning)
     * Boosts relevance of matching visual objects
     */
    void set_active_concepts(const std::vector<std::string>& concepts);
    
    /**
     * Set attention mode dynamically
     */
    void set_mode(AttentionMode mode);
    
    /**
     * Adjust attention weights on the fly
     */
    void adjust_weights(float alpha, float beta, float gamma);
    
    // ========================================================================
    // FOCUS QUERIES
    // ========================================================================
    
    /**
     * Get current focus target
     */
    const FocusTarget& get_current_focus() const { return current_focus_; }
    
    /**
     * Get focus history (working memory)
     */
    const std::deque<FocusTarget>& get_focus_history() const { return focus_history_; }
    
    /**
     * Get recent saccades
     */
    const std::vector<Saccade>& get_saccade_history() const { return saccade_history_; }
    
    /**
     * Is currently focused on something?
     */
    bool is_focused() const { return current_focus_.is_valid(); }
    
    /**
     * How long have we been focused on current target?
     */
    int get_focus_duration() const { return current_focus_.duration_frames; }
    
    // ========================================================================
    // GLOBAL WORKSPACE BROADCAST
    // ========================================================================
    
    /**
     * Broadcast currently focused concepts to entire system
     * This is what Melvin is "consciously aware" of
     */
    struct ConsciousContent {
        FocusTarget focus;
        std::vector<uint64_t> active_concepts;
        std::vector<std::string> active_labels;
        float activation_strength;
    };
    
    ConsciousContent get_conscious_content() const;
    
    /**
     * Update global workspace (called after reasoning)
     */
    void update_workspace(const std::vector<uint64_t>& concept_nodes);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct FocusStats {
        uint64_t total_saccades;
        uint64_t forced_saccades;
        uint64_t goal_driven_saccades;
        uint64_t curiosity_driven_saccades;
        double avg_focus_duration_frames;
        uint64_t total_focus_shifts;
    };
    
    FocusStats get_stats() const { return stats_; }
    void print_stats() const;
    
private:
    // ========================================================================
    // INTERNAL FOCUS CONTROL
    // ========================================================================
    
    // Compute combined focus score
    float compute_focus_score(const VisualNode& obj);
    
    // Check if should switch focus
    bool should_switch_focus(const FocusTarget& new_candidate);
    
    // Apply inhibition of return
    void apply_ior(std::vector<VisualNode>& candidates);
    
    // Perform saccade (attention shift)
    void perform_saccade(const FocusTarget& new_focus, const std::string& reason);
    
    // Update focus history
    void update_history(const FocusTarget& focus);
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    MelvinGraph* graph_;
    Config config_;
    
    // Current attention state
    FocusTarget current_focus_;
    AttentionMode current_mode_;
    
    // Focus history (working memory)
    std::deque<FocusTarget> focus_history_;
    std::vector<Saccade> saccade_history_;
    
    // Top-down control
    std::string active_goal_;
    std::vector<std::string> active_concepts_;
    
    // IOR tracking
    struct IOREntry {
        std::string target_id;
        int frames_remaining;
    };
    std::vector<IOREntry> ior_list_;
    
    // Statistics
    FocusStats stats_;
    
    // Frame counter
    uint64_t frame_count_;
};

} // namespace uca
} // namespace melvin


