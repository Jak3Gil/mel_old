#pragma once

#include "../core/atomic_graph.h"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <string>

namespace melvin {
namespace vision {

// ============================================================================
// VISUAL EVENT STRUCTURE
// ============================================================================

/**
 * VisualEvent - Visual perception event from camera/vision system
 */
struct VisualEvent {
    uint64_t id;                // Unique event identifier
    float timestamp;            // Time in seconds since start
    std::string label;          // Object/scene name (e.g., "dog", "person", "bottle")
    std::string type;           // "object", "scene", "action"
    float confidence;           // Detection confidence 0.0 to 1.0
    
    // Bounding box (optional)
    int x1, y1, x2, y2;
    
    // Optional metadata
    float area;                 // Area of bounding box
    int frame_number;           // Frame index
    
    VisualEvent() 
        : id(0), timestamp(0.0f), confidence(0.0f), 
          x1(0), y1(0), x2(0), y2(0), area(0.0f), frame_number(0) {}
    
    VisualEvent(uint64_t event_id, float ts, const std::string& lbl, 
                const std::string& t, float conf)
        : id(event_id), timestamp(ts), label(lbl), type(t), 
          confidence(conf), x1(0), y1(0), x2(0), y2(0), 
          area(0.0f), frame_number(0) {}
};

// ============================================================================
// VISION BRIDGE CLASS
// ============================================================================

/**
 * VisionBridge - Integrates visual events into AtomicGraph
 * 
 * Similar to AudioBridge but for visual perception:
 * - Convert VisualEvent → Graph nodes
 * - Create vision-specific edges
 * - Cross-modal synchronization (vision ↔ audio)
 * - Temporal association within window
 * - Reinforcement of co-occurrence patterns
 * 
 * Node format: "object:dog", "object:person", etc.
 * Edge types: CO_OCCURS_WITH (spatial), TEMPORAL_NEXT (temporal)
 */
class VisionBridge {
public:
    struct Config {
        float temporal_window = 3.0f;        // Cross-modal sync window (seconds)
        bool enable_cross_modal = true;       // Enable audio-vision sync
        bool create_object_nodes = true;      // Create nodes for each object
        bool create_scene_nodes = false;      // Create scene-level nodes
        float min_confidence = 0.3f;          // Minimum confidence to process
        bool verbose = false;                 // Log processing
        
        Config() {}
    };
    
    VisionBridge();
    explicit VisionBridge(const Config& config);
    ~VisionBridge();
    
    // ========================================================================
    // CORE PROCESSING
    // ========================================================================
    
    /**
     * Process single visual event into graph
     * Creates nodes and edges for detected objects
     */
    void process(const VisualEvent& event, AtomicGraph& graph);
    
    /**
     * Process batch of visual events (more efficient)
     */
    void process_batch(const std::vector<VisualEvent>& events, AtomicGraph& graph);
    
    // ========================================================================
    // CROSS-MODAL SYNCHRONIZATION
    // ========================================================================
    
    /**
     * Synchronize vision with audio events
     * Links visual and audio concepts that occur close in time
     * 
     * Example: visual "dog" at t=1.0s + audio "dog" at t=1.2s
     *          → Create edge: visual:dog ↔ audio:dog
     */
    void sync_with_audio(
        const std::vector<VisualEvent>& visual_events,
        const std::vector<uint64_t>& audio_node_ids,
        const std::vector<float>& audio_timestamps,
        AtomicGraph& graph
    );
    
    // ========================================================================
    // TEMPORAL PROCESSING
    // ========================================================================
    
    /**
     * Create temporal edges between objects across frames
     * Links objects that appear in consecutive frames
     */
    void create_temporal_edges(
        const std::vector<VisualEvent>& current_events,
        const std::vector<VisualEvent>& previous_events,
        AtomicGraph& graph
    );
    
    /**
     * Create spatial co-occurrence edges
     * Links objects that appear together in same frame
     */
    void create_spatial_edges(
        const std::vector<VisualEvent>& events,
        AtomicGraph& graph
    );
    
    // ========================================================================
    // NODE MANAGEMENT
    // ========================================================================
    
    /**
     * Get or create node for visual concept
     * Returns node ID for the visual concept
     */
    uint64_t get_or_create_visual_node(
        const std::string& label,
        const std::string& type,
        AtomicGraph& graph
    );
    
    /**
     * Record visual event for temporal tracking
     */
    void record_event(const VisualEvent& event, uint64_t node_id);
    
    // ========================================================================
    // STATISTICS & DIAGNOSTICS
    // ========================================================================
    
    struct Stats {
        uint64_t events_processed = 0;
        uint64_t nodes_created = 0;
        uint64_t nodes_reused = 0;
        uint64_t spatial_edges_created = 0;
        uint64_t temporal_edges_created = 0;
        uint64_t cross_modal_links = 0;
        float avg_confidence = 0.0f;
    };
    
    Stats get_stats() const;
    void reset_stats();
    void print_stats() const;
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    const Config& get_config() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace vision
} // namespace melvin

