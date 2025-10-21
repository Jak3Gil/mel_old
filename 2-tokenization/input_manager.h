#pragma once

#include "../audio/audio_pipeline.h"
#include "../audio/audio_bridge.h"
#include "../vision/opencv_attention.h"
#include "../vision/vision_bridge.h"
#include "atomic_graph.h"
#include <vector>
#include <memory>

namespace melvin {

/**
 * InputManager - Unified Multi-Modal Input System
 * 
 * Manages and coordinates all input modalities:
 * - Audio (speech + ambient sounds)
 * - Vision (objects + scenes) [future]
 * - Text (queries + documents) [existing]
 * 
 * Responsibilities:
 * - Capture raw input from sensors
 * - Process and recognize patterns
 * - Convert to events for graph integration
 * - Synchronize cross-modal inputs
 */
class InputManager {
public:
    /**
     * Configuration for input manager
     */
    struct Config {
        // Audio configuration
        bool enable_audio = true;
        uint32_t audio_sample_rate = 16000;
        float audio_vad_threshold = 0.02f;
        float audio_temporal_window = 3.0f;
        
        // Vision configuration
        bool enable_vision = true;
        int vision_camera_index = 0;
        int vision_width = 640;
        int vision_height = 480;
        float vision_confidence = 0.3f;
        float vision_temporal_window = 3.0f;
        
        // Text configuration
        bool enable_text = true;
    };
    
    InputManager();
    explicit InputManager(const Config& config);
    ~InputManager();
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    /**
     * Initialize all input systems
     * @return true if successful
     */
    bool init();
    
    /**
     * Shutdown all input systems
     */
    void shutdown();
    
    /**
     * Process inputs for this frame
     * @param dt Delta time in seconds
     */
    void tick(float dt);
    
    // ========================================================================
    // EVENT RETRIEVAL
    // ========================================================================
    
    /**
     * Get recent audio events since last call
     */
    std::vector<audio::AudioEvent> get_audio_events() const;
    
    /**
     * Get recent visual events since last call
     */
    std::vector<vision::VisualEvent> get_visual_events() const;
    
    /**
     * Get recent text events (future)
     */
    // std::vector<audio::TextEvent> get_text_events() const;
    
    // ========================================================================
    // GRAPH INTEGRATION
    // ========================================================================
    
    /**
     * Process all recent events and integrate into graph
     * @param graph Target knowledge graph
     */
    void process_to_graph(AtomicGraph& graph);
    
    /**
     * Synchronize cross-modal events
     * @param graph Target knowledge graph
     */
    void sync_cross_modal(AtomicGraph& graph);
    
    // ========================================================================
    // STATUS & DIAGNOSTICS
    // ========================================================================
    
    bool is_audio_enabled() const { return config_.enable_audio && audio_initialized_; }
    bool is_vision_enabled() const { return config_.enable_vision && vision_initialized_; }
    bool is_text_enabled() const { return config_.enable_text; }
    
    float get_audio_volume() const;
    size_t get_audio_event_count() const;
    size_t get_vision_event_count() const;
    
    void print_stats() const;
    
    // ========================================================================
    // DIRECT ACCESS (for advanced use)
    // ========================================================================
    
    audio::AudioPipeline& audio_pipeline() { return *audio_pipeline_; }
    audio::AudioBridge& audio_bridge() { return *audio_bridge_; }
    
    vision::OpenCVAttention& vision_pipeline() { return *vision_pipeline_; }
    vision::VisionBridge& vision_bridge() { return *vision_bridge_; }
    
private:
    Config config_;
    
    // Audio subsystem
    std::unique_ptr<audio::AudioPipeline> audio_pipeline_;
    std::unique_ptr<audio::AudioBridge> audio_bridge_;
    bool audio_initialized_ = false;
    
    // Vision subsystem
    std::unique_ptr<vision::OpenCVAttention> vision_pipeline_;
    std::unique_ptr<vision::VisionBridge> vision_bridge_;
    bool vision_initialized_ = false;
    
    // Event storage
    mutable std::vector<audio::AudioEvent> recent_audio_events_;
    mutable std::vector<vision::VisualEvent> recent_visual_events_;
    
    // Statistics
    size_t total_events_processed_ = 0;
    size_t audio_events_processed_ = 0;
    size_t vision_events_processed_ = 0;
};

} // namespace melvin

