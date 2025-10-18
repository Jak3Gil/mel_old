#include "input_manager.h"
#include <iostream>

namespace melvin {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

InputManager::InputManager()
    : InputManager(Config()) {
}

InputManager::InputManager(const Config& config)
    : config_(config) {
    
    std::cout << "🎮 InputManager initializing..." << std::endl;
    
    // Create audio subsystem if enabled
    if (config_.enable_audio) {
        audio::AudioPipeline::Config audio_config;
        audio_config.sample_rate = config_.audio_sample_rate;
        audio_config.vad_threshold = config_.audio_vad_threshold;
        audio_config.enable_speech = true;
        audio_config.enable_ambient = true;
        
        audio_pipeline_ = std::make_unique<audio::AudioPipeline>(audio_config);
        
        audio::AudioBridge::Config bridge_config;
        bridge_config.temporal_window = config_.audio_temporal_window;
        bridge_config.enable_cross_modal = true;
        bridge_config.create_word_nodes = true;
        
        audio_bridge_ = std::make_unique<audio::AudioBridge>(bridge_config);
        
        std::cout << "   🎤 Audio subsystem created" << std::endl;
    }
    
    // Create vision subsystem if enabled
    if (config_.enable_vision) {
        vision::OpenCVAttention::Config vision_config;
        vision_config.camera_index = config_.vision_camera_index;
        vision_config.width = config_.vision_width;
        vision_config.height = config_.vision_height;
        vision_config.confidence = config_.vision_confidence;
        vision_config.show_display = false;  // No display in unified mode
        
        // Vision pipeline will be initialized in init()
        // (needs to open camera, can't do in constructor)
        
        vision::VisionBridge::Config bridge_config;
        bridge_config.temporal_window = config_.vision_temporal_window;
        bridge_config.enable_cross_modal = true;
        bridge_config.create_object_nodes = true;
        
        vision_bridge_ = std::make_unique<vision::VisionBridge>(bridge_config);
        
        std::cout << "   👁️  Vision subsystem created" << std::endl;
    }
    
    std::cout << "✅ InputManager initialized" << std::endl;
}

InputManager::~InputManager() {
    shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool InputManager::init() {
    std::cout << "🚀 Starting input systems..." << std::endl;
    
    // Initialize audio
    if (config_.enable_audio && audio_pipeline_) {
        if (audio_pipeline_->start_stream()) {
            audio_initialized_ = true;
            std::cout << "   ✅ Audio stream started" << std::endl;
        } else {
            std::cerr << "   ⚠️  Audio stream failed to start (continuing without)" << std::endl;
            audio_initialized_ = false;
        }
    }
    
    // Initialize vision
    // Note: Vision pipeline in OpenCVAttention uses blocking run()
    // For unified system, vision will be processed frame-by-frame in tick()
    if (config_.enable_vision && vision_bridge_) {
        vision_initialized_ = true;
        std::cout << "   ✅ Vision system ready" << std::endl;
    }
    
    std::cout << "✅ Input systems started" << std::endl;
    return true;
}

void InputManager::shutdown() {
    std::cout << "🛑 Shutting down input systems..." << std::endl;
    
    if (audio_initialized_ && audio_pipeline_) {
        audio_pipeline_->stop_stream();
        audio_initialized_ = false;
        std::cout << "   ✅ Audio stream stopped" << std::endl;
    }
    
    std::cout << "✅ Input systems shut down" << std::endl;
}

// ============================================================================
// FRAME PROCESSING
// ============================================================================

void InputManager::tick(float dt) {
    // Process audio
    if (audio_initialized_ && audio_pipeline_) {
        audio_pipeline_->tick(dt);
        
        // Retrieve recent events
        recent_audio_events_ = audio_pipeline_->get_recent_events();
        
        if (!recent_audio_events_.empty()) {
            audio_events_processed_ += recent_audio_events_.size();
            total_events_processed_ += recent_audio_events_.size();
        }
    }
    
    // Process vision (future)
    // if (vision_initialized_ && vision_pipeline_) { ... }
}

// ============================================================================
// EVENT RETRIEVAL
// ============================================================================

std::vector<audio::AudioEvent> InputManager::get_audio_events() const {
    return recent_audio_events_;
}

std::vector<vision::VisualEvent> InputManager::get_visual_events() const {
    return recent_visual_events_;
}

// ============================================================================
// GRAPH INTEGRATION
// ============================================================================

void InputManager::process_to_graph(AtomicGraph& graph) {
    // Process audio events
    if (!recent_audio_events_.empty() && audio_bridge_) {
        for (const auto& event : recent_audio_events_) {
            audio_bridge_->process(event, graph);
        }
    }
    
    // Process vision events
    if (!recent_visual_events_.empty() && vision_bridge_) {
        vision_bridge_->process_batch(recent_visual_events_, graph);
    }
}

void InputManager::sync_cross_modal(AtomicGraph& graph) {
    // Synchronize audio and vision
    if (!recent_audio_events_.empty() && !recent_visual_events_.empty() && vision_bridge_) {
        // Extract audio node IDs and timestamps for cross-modal sync
        std::vector<uint64_t> audio_node_ids;
        std::vector<float> audio_timestamps;
        
        for (const auto& audio_event : recent_audio_events_) {
            // Audio events should have been processed, get their node IDs from graph
            // For now, we'll use the event timestamps for temporal sync
            audio_timestamps.push_back(audio_event.timestamp);
        }
        
        // Sync vision with audio
        vision_bridge_->sync_with_audio(
            recent_visual_events_,
            audio_node_ids,
            audio_timestamps,
            graph
        );
    }
    
    // Standard audio-only sync if needed
    if (audio_bridge_ && !recent_audio_events_.empty()) {
        std::vector<audio::VisualEvent> empty_visual;
        std::vector<audio::TextEvent> text_events;
        std::vector<audio::ActionEvent> action_events;
        
        audio_bridge_->sync_with(
            recent_audio_events_,
            empty_visual,
            text_events,
            action_events,
            graph
        );
        
        // Apply learning dynamics
        audio_bridge_->reinforce_patterns(graph, 0.99f);
    }
}

// ============================================================================
// STATUS & DIAGNOSTICS
// ============================================================================

float InputManager::get_audio_volume() const {
    if (audio_initialized_ && audio_pipeline_) {
        return audio_pipeline_->get_current_volume();
    }
    return 0.0f;
}

size_t InputManager::get_audio_event_count() const {
    return audio_events_processed_;
}

size_t InputManager::get_vision_event_count() const {
    return vision_events_processed_;
}

void InputManager::print_stats() const {
    std::cout << "\n📊 InputManager Statistics:" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    if (config_.enable_audio) {
        std::cout << "🎤 Audio:" << std::endl;
        std::cout << "   Status: " << (audio_initialized_ ? "✅ Active" : "❌ Inactive") << std::endl;
        if (audio_initialized_) {
            std::cout << "   Events processed: " << audio_events_processed_ << std::endl;
            std::cout << "   Current volume: " << get_audio_volume() << std::endl;
            
            if (audio_pipeline_) {
                audio_pipeline_->print_stats();
            }
            if (audio_bridge_) {
                audio_bridge_->print_stats();
            }
        }
    }
    
    if (config_.enable_vision) {
        std::cout << "\n👁️  Vision:" << std::endl;
        std::cout << "   Status: " << (vision_initialized_ ? "✅ Active" : "❌ Inactive") << std::endl;
        if (vision_initialized_) {
            std::cout << "   Events processed: " << vision_events_processed_ << std::endl;
            
            if (vision_bridge_) {
                vision_bridge_->print_stats();
            }
        }
    }
    
    std::cout << "\n📈 Total:" << std::endl;
    std::cout << "   Events processed: " << total_events_processed_ << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
}

} // namespace melvin

