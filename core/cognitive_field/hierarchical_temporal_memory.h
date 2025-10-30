#ifndef HIERARCHICAL_TEMPORAL_MEMORY_H
#define HIERARCHICAL_TEMPORAL_MEMORY_H

#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <string>
#include <memory>

namespace melvin {
namespace cognitive_field {

/**
 * Hierarchical Temporal Memory
 * 
 * Multi-level temporal abstraction:
 * Level 1: Frames (100ms - 5s)    - Immediate perceptions
 * Level 2: Scenes (5s - 2min)      - Coherent situations
 * Level 3: Episodes (2min - 1hr)   - Complete events
 * Level 4: Narratives (> 1hr)      - Long-term themes
 * 
 * Each level compresses and stores summaries, preventing context reset
 */

// ============================================================================
// Level 1: Frames (100ms - 5s)
// ============================================================================

struct Frame {
    std::chrono::high_resolution_clock::time_point timestamp;
    std::vector<int> active_nodes;           // Nodes active in this frame
    std::vector<float> activations;          // Their activation levels
    std::vector<float> context_vector;       // Global context at this moment
    int modality_mask;                       // Bitfield: which modalities active
    float energy_level;                      // Total field energy
    float surprise_level;                    // Prediction error
    
    Frame() : modality_mask(0), energy_level(0.0f), surprise_level(0.0f) {}
};

// ============================================================================
// Level 2: Scenes (5s - 2min)
// ============================================================================

struct Scene {
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    
    // Compressed representation
    std::vector<int> key_nodes;              // Most frequent/important nodes
    std::unordered_map<int, float> node_importance;  // How important each node was
    std::vector<float> scene_embedding;      // Average context vector
    
    // Temporal structure
    std::vector<std::pair<int, int>> temporal_links;  // (before, after) pairs
    
    // Scene classification
    float coherence_score;                   // How stable was the scene
    float novelty_score;                     // How different from past scenes
    int dominant_modality;                   // Which modality was primary
    
    // Reference to constituent frames (for replay)
    size_t first_frame_index;
    size_t last_frame_index;
    
    Scene() : coherence_score(0.0f), novelty_score(0.0f), 
              dominant_modality(0), first_frame_index(0), last_frame_index(0) {}
};

// ============================================================================
// Level 3: Episodes (2min - 1hr)
// ============================================================================

struct Episode {
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    
    // High-level summary
    std::vector<int> theme_nodes;            // Core concepts of this episode
    std::vector<float> episode_embedding;    // Abstract representation
    
    // Causal structure
    std::vector<std::pair<int, int>> causal_links;  // (cause, effect) pairs
    int initiating_event;                    // What started this episode
    int outcome_event;                       // How it ended
    
    // Episode metadata
    float emotional_valence;                 // Positive/negative
    float importance_score;                  // How significant
    bool was_goal_directed;                  // Was there a clear goal
    int goal_node;                           // Target concept if goal-directed
    
    // Reference to constituent scenes
    std::vector<size_t> scene_indices;
    
    Episode() : initiating_event(-1), outcome_event(-1), 
                emotional_valence(0.0f), importance_score(0.0f),
                was_goal_directed(false), goal_node(-1) {}
};

// ============================================================================
// Level 4: Narratives (> 1hr)
// ============================================================================

struct Narrative {
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    
    // Abstract themes
    std::vector<int> core_concepts;          // Recurring themes
    std::unordered_map<int, float> concept_evolution;  // How concepts changed
    
    // Long-term patterns
    std::vector<std::pair<int, int>> recurring_patterns;  // Patterns seen multiple times
    std::vector<int> learned_skills;         // New capabilities acquired
    
    // Narrative arc
    float growth_trajectory;                 // Did capabilities improve
    float exploration_rate;                  // How much novelty
    
    // Reference to constituent episodes
    std::vector<size_t> episode_indices;
    
    Narrative() : growth_trajectory(0.0f), exploration_rate(0.0f) {}
};

// ============================================================================
// Hierarchical Memory Manager
// ============================================================================

class HierarchicalMemory {
public:
    HierarchicalMemory(size_t embedding_dim = 128);
    
    // ========================================================================
    // Frame Level (Called at 10-30Hz)
    // ========================================================================
    
    /**
     * Add a new frame from current field state
     */
    void add_frame(const std::vector<int>& active_nodes,
                   const std::vector<float>& activations,
                   const std::vector<float>& context_vector,
                   int modality_mask,
                   float energy_level,
                   float surprise_level);
    
    /**
     * Get recent frames
     */
    std::vector<Frame> get_recent_frames(size_t count) const;
    
    // ========================================================================
    // Scene Level (Auto-segmented)
    // ========================================================================
    
    /**
     * Check if current frames should be consolidated into a scene
     * Called after each frame
     */
    void check_and_create_scene();
    
    /**
     * Get recent scenes
     */
    std::vector<Scene> get_recent_scenes(size_t count) const;
    
    /**
     * Find similar past scenes
     */
    std::vector<std::pair<size_t, float>> find_similar_scenes(
        const std::vector<float>& query_embedding, size_t k) const;
    
    // ========================================================================
    // Episode Level (Auto-segmented)
    // ========================================================================
    
    /**
     * Check if current scenes should be consolidated into an episode
     */
    void check_and_create_episode();
    
    /**
     * Get recent episodes
     */
    std::vector<Episode> get_recent_episodes(size_t count) const;
    
    /**
     * Retrieve episodes related to a concept
     */
    std::vector<size_t> find_episodes_with_concept(int node_id) const;
    
    // ========================================================================
    // Narrative Level (Periodic consolidation)
    // ========================================================================
    
    /**
     * Create narrative from recent episodes (called periodically)
     */
    void consolidate_narrative();
    
    /**
     * Get all narratives
     */
    std::vector<Narrative> get_narratives() const;
    
    // ========================================================================
    // Cross-Level Queries
    // ========================================================================
    
    /**
     * Retrieve full context for a moment in time
     * Returns relevant frames, scenes, episodes, narratives
     */
    struct TemporalContext {
        std::vector<Frame> frames;
        std::vector<Scene> scenes;
        std::vector<Episode> episodes;
        std::vector<Narrative> narratives;
    };
    
    TemporalContext retrieve_context_at_time(
        std::chrono::high_resolution_clock::time_point query_time,
        int lookback_seconds) const;
    
    /**
     * Get temporal trajectory of a concept
     * How has this concept's activation evolved over time
     */
    std::vector<std::pair<std::chrono::high_resolution_clock::time_point, float>>
    get_concept_trajectory(int node_id, int lookback_seconds) const;
    
    // ========================================================================
    // Replay (for consolidation and learning)
    // ========================================================================
    
    /**
     * Replay a scene (for offline learning)
     */
    std::vector<Frame> replay_scene(size_t scene_index) const;
    
    /**
     * Replay an episode
     */
    std::vector<Scene> replay_episode(size_t episode_index) const;
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    struct Stats {
        size_t total_frames;
        size_t total_scenes;
        size_t total_episodes;
        size_t total_narratives;
        float avg_scene_duration_sec;
        float avg_episode_duration_sec;
        float compression_ratio;  // Frames / Scenes
    };
    Stats get_stats() const;
    
    void save(const std::string& filename) const;
    void load(const std::string& filename);
    
private:
    size_t embedding_dim_;
    
    // Storage for each level
    std::deque<Frame> frames_;
    std::deque<Scene> scenes_;
    std::deque<Episode> episodes_;
    std::deque<Narrative> narratives_;
    
    // Limits (oldest entries pruned)
    static constexpr size_t MAX_FRAMES = 10000;      // ~15 min at 10Hz
    static constexpr size_t MAX_SCENES = 1000;       // ~30 hours
    static constexpr size_t MAX_EPISODES = 200;      // ~weeks
    static constexpr size_t MAX_NARRATIVES = 50;     // ~months
    
    // Scene segmentation parameters
    float scene_coherence_threshold_ = 0.7f;
    size_t min_frames_per_scene_ = 10;
    size_t max_frames_per_scene_ = 600;
    
    // Episode segmentation parameters
    float episode_boundary_threshold_ = 0.5f;
    size_t min_scenes_per_episode_ = 3;
    size_t max_scenes_per_episode_ = 50;
    
    // Helper functions
    bool is_scene_boundary() const;
    bool is_episode_boundary() const;
    
    Scene compress_frames_to_scene(size_t start_idx, size_t end_idx);
    Episode compress_scenes_to_episode(const std::vector<size_t>& scene_indices);
    Narrative compress_episodes_to_narrative(const std::vector<size_t>& episode_indices);
    
    float compute_scene_coherence(size_t start_idx, size_t end_idx) const;
    float compute_scene_similarity(const Scene& a, const Scene& b) const;
    
    std::vector<int> extract_key_nodes(size_t start_idx, size_t end_idx) const;
    std::vector<float> compute_average_embedding(size_t start_idx, size_t end_idx) const;
};

} // namespace cognitive_field
} // namespace melvin

#endif // HIERARCHICAL_TEMPORAL_MEMORY_H

