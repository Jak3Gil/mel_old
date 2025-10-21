#pragma once

#include "../core/atomic_graph.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace melvin {
namespace vision {

/**
 * Curiosity Object - Tracks objects under investigation
 */
struct CuriosityObject {
    uint64_t id;              // Node ID in graph
    std::string label;        // Current label
    float confidence;         // Detection confidence
    int frame_count;          // How long we've observed it
    bool resolved;            // Whether we've identified it
    float last_similarity;    // Last embedding similarity
    
    CuriosityObject(uint64_t nid, const std::string& lbl, float conf)
        : id(nid), label(lbl), confidence(conf), frame_count(0), 
          resolved(false), last_similarity(0.0f) {}
};

/**
 * Curiosity Loop - Biological curiosity for unknown objects
 * 
 * Behavior:
 * - Low confidence or "unknown" → enters curiosity loop
 * - Tracked for >10 frames → creates unknown node
 * - High similarity later → resolves and merges
 * - Never re-seen → decays and removes
 */
class CuriosityLoop {
public:
    explicit CuriosityLoop(AtomicGraph* graph);
    ~CuriosityLoop();
    
    /**
     * Observe an object (called per detection)
     * If confidence < threshold or label="unknown", enters curiosity
     */
    void observe(const std::string& label, float confidence, uint64_t node_id);
    
    /**
     * Update curiosity objects (called per frame)
     * Creates unknown nodes for persistent mysteries
     */
    void update();
    
    /**
     * Reinforce curiosity object with new evidence
     * If similarity > threshold, marks as resolved
     */
    void reinforce(uint64_t id, float similarity);
    
    /**
     * Decay forgotten curiosity objects
     */
    void decay();
    
    /**
     * Get statistics
     */
    struct Stats {
        size_t active_curiosities = 0;
        size_t resolved_mysteries = 0;
        size_t created_unknowns = 0;
        size_t decayed_objects = 0;
    };
    
    Stats get_stats() const;
    void print_stats() const;
    
private:
    AtomicGraph* graph_;
    std::unordered_map<uint64_t, CuriosityObject> active_;
    
    Stats stats_;
    
    // Parameters
    float confidence_threshold_ = 0.5f;     // Below this = curious
    int curiosity_duration_ = 10;           // Frames to track
    float resolution_threshold_ = 0.8f;     // Similarity to resolve
    float decay_rate_ = 0.95f;              // Confidence decay per tick
};

} // namespace vision
} // namespace melvin

