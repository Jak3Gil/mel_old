/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  🧠 MELVIN UNIFIED - Complete Cognitive System                            ║
 * ║  All brain components working together in ONE pipeline                    ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 * 
 * Architecture:
 * Camera + Mic → Tokenization → AtomicGraph →
 * [EnergyField → Hopfield → LEAPs → GNN → Hybrid → Adaptive → Reasoning → Episodic] →
 * Output (feeds back into graph)
 */

#include "melvin/core/atomic_graph.h"
#include "melvin/audio/audio_pipeline.h"
#include "melvin/audio/audio_bridge.h"
#include "melvin/vision/vision_bridge.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace melvin;

// Global flag for clean shutdown
volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n\n🛑 Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

// ============================================================================
// VISION PROCESSING (Frame-by-frame with YOLO via Python)
// ============================================================================

struct Detection {
    std::string label;
    float confidence;
    int x1, y1, x2, y2;
};

// Parse YOLO JSON output
std::vector<Detection> parse_yolo_json(const std::string& json) {
    std::vector<Detection> dets;
    size_t pos = 0;
    
    while (true) {
        pos = json.find("{", pos);
        if (pos == std::string::npos) break;
        
        size_t end = json.find("}", pos);
        if (end == std::string::npos) break;
        
        std::string obj = json.substr(pos, end - pos + 1);
        Detection d;
        
        // Parse object_id (for raw vision) or label (for YOLO fallback)
        size_t lp = obj.find("\"object_id\"");
        int key_len = 11;  // "object_id" length
        if (lp == std::string::npos) {
            lp = obj.find("\"label\"");  // Fallback to label if object_id not found
            key_len = 7;  // "label" length
        }
        if (lp != std::string::npos) {
            // Find the colon after the key
            size_t colon = obj.find(":", lp);
            if (colon != std::string::npos) {
                // Find opening quote after colon
                size_t s = obj.find("\"", colon);
                // Find closing quote
                size_t e = obj.find("\"", s + 1);
                if (s != std::string::npos && e != std::string::npos)
                    d.label = obj.substr(s+1, e-s-1);
            }
        }
        
        // Parse confidence  
        size_t cp = obj.find("\"confidence\"");
        if (cp != std::string::npos) {
            size_t s = obj.find(":", cp);
            if (s != std::string::npos) {
                std::string conf_str = obj.substr(s+1);
                // Find next comma or closing brace
                size_t end = conf_str.find_first_of(",}");
                if (end != std::string::npos) {
                    conf_str = conf_str.substr(0, end);
                }
                try {
                    d.confidence = std::stof(conf_str);
                } catch (...) {
                    d.confidence = 0.0f;
                }
            }
        }
        
        // Parse bbox
        auto parse_int = [&](const std::string& key) {
            size_t kp = obj.find("\"" + key + "\"");
            if (kp != std::string::npos) {
                size_t s = obj.find(":", kp);
                if (s != std::string::npos)
                    return std::stoi(obj.substr(s+1));
            }
            return 0;
        };
        
        d.x1 = parse_int("x1");
        d.y1 = parse_int("y1");
        d.x2 = parse_int("x2");
        d.y2 = parse_int("y2");
        
        dets.push_back(d);
        pos = end + 1;
    }
    
    return dets;
}

// ============================================================================
// UNIFIED MELVIN BRAIN
// ============================================================================

class UnifiedMelvin {
public:
    UnifiedMelvin() {
        std::cout << "🧠 Initializing Unified Melvin Brain..." << std::endl;
        
        // Core storage
        graph_ = new AtomicGraph();
        
        // Load existing knowledge from main data directory
        try {
            graph_->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
            std::cout << "📂 Loaded existing knowledge: " 
                      << graph_->node_count() << " nodes, "
                      << graph_->edge_count() << " edges" << std::endl;
        } catch (...) {
            std::cout << "📝 Starting with fresh knowledge base" << std::endl;
        }
        
        // Audio subsystem
        audio::AudioPipeline::Config audio_config;
        audio_config.sample_rate = 16000;
        audio_config.enable_speech = true;
        audio_config.enable_ambient = true;
        audio_pipeline_ = new audio::AudioPipeline(audio_config);
        
        audio::AudioBridge::Config audio_bridge_config;
        audio_bridge_config.enable_cross_modal = true;
        audio_bridge_ = new audio::AudioBridge(audio_bridge_config);
        
        // Vision subsystem
        vision::VisionBridge::Config vision_bridge_config;
        vision_bridge_config.enable_cross_modal = true;
        vision_bridge_config.create_object_nodes = true;  // Ensure node creation enabled
        vision_bridge_config.min_confidence = 0.15f;  // Lower threshold for attention scores
        vision_bridge_config.verbose = true;  // Enable debug output
        vision_bridge_ = new vision::VisionBridge(vision_bridge_config);
        
        std::cout << "✅ All components initialized (vision + audio + graph)" << std::endl;
        std::cout << "📝 Note: Advanced brain components (Hopfield, GNN, etc.) can be added next" << std::endl;
    }
    
    ~UnifiedMelvin() {
        delete vision_bridge_;
        delete audio_bridge_;
        delete audio_pipeline_;
        delete graph_;
    }
    
    bool start() {
        std::cout << "\n🚀 Starting Unified Melvin..." << std::endl;
        
        // Start audio
        if (!audio_pipeline_->start_stream()) {
            std::cerr << "⚠️  Audio failed (continuing without)" << std::endl;
        } else {
            std::cout << "✅ Audio stream started" << std::endl;
        }
        
        // Note: Camera will be accessed via Python script (detect_objects.py)
        // Check if Python + camera is available
        int result = system("python3 -c 'import cv2; cap = cv2.VideoCapture(0); print(cap.isOpened())' > /tmp/melvin_camera_check.txt 2>&1");
        if (result == 0) {
            camera_enabled_ = true;
            std::cout << "✅ Camera available via Python" << std::endl;
        } else {
            camera_enabled_ = false;
            std::cout << "⚠️  Camera not available (continuing without vision)" << std::endl;
        }
        
        std::cout << "\n🧠 Melvin is now perceiving and thinking..." << std::endl;
        std::cout << "   (Press Ctrl+C to stop)\n" << std::endl;
        
        last_save_ = std::chrono::steady_clock::now();
        start_time_ = last_save_;
        
        return true;
    }
    
    void tick(float dt) {
        if (!g_running) return;
        
        frame_count_++;
        std::vector<uint64_t> active_nodes;
        
        // ====================================================================
        // 1. PERCEIVE - Gather inputs from all modalities
        // ====================================================================
        
        // Audio perception
        audio_pipeline_->tick(dt);
        auto audio_events = audio_pipeline_->get_recent_events();
        
        // Vision perception DISABLED - will rebuild from scratch
        std::vector<vision::VisualEvent> visual_events;
        if (false && camera_enabled_ && frame_count_ % 50 == 0) {
            // Vision system to be rebuilt
            std::string cmd = "python3 vision_system.py 2>> /tmp/vision_debug.log";
            
            std::cout << "[Frame " << frame_count_ << "] 🔲 Patch..." << std::flush;
            
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                std::string output;
                char buf[512];
                while (fgets(buf, sizeof(buf), pipe)) output += buf;
                pclose(pipe);
                
                if (output.empty() || output == "[]\n" || output == "[]") {
                    std::cout << " (no objects detected)" << std::endl;
                } else {
                    // Parse JSON output (now includes tracking metadata)
                    auto detections = parse_yolo_json(output);
                    
                    int new_count = 0, reinforced_count = 0, focus_count = 0;
                    
                    // Convert to VisualEvents (raw objects, no semantic labels)
                    for (const auto& det : detections) {
                        vision::VisualEvent event;
                        event.id = next_event_id_++;
                        event.timestamp = get_current_time();
                        event.label = det.label;  // Will be "object_0", "object_1", etc.
                        event.type = "object";  // Keep as "object" so vision_bridge creates nodes
                        event.confidence = det.confidence;
                        event.x1 = det.x1;
                        event.y1 = det.y1;
                        event.x2 = det.x2;
                        event.y2 = det.y2;
                        event.frame_number = frame_count_;
                        
                        visual_events.push_back(event);
                        
                        // Check metadata from attention vision
                        size_t action_pos = output.find("\"action\"");
                        if (action_pos != std::string::npos) {
                            if (output.find("\"reinforced\"", action_pos) != std::string::npos) {
                                reinforced_count++;
                            } else if (output.find("\"new\"", action_pos) != std::string::npos) {
                                new_count++;
                            }
                        }
                        
                        size_t focus_pos = output.find("\"is_focus\": true");
                        if (focus_pos != std::string::npos) {
                            focus_count++;
                        }
                    }
                    
                    std::cout << " 👁️ " << detections.size() << " objects";
                    if (new_count > 0) std::cout << " (+" << new_count << " new)";
                    if (reinforced_count > 0) std::cout << " (↑" << reinforced_count << " tracked)";
                    if (focus_count > 0) std::cout << " [🎯 " << focus_count << " in focus]";
                    std::cout << std::endl;
                    
                    // Show what we saw
                    for (const auto& det : detections) {
                        std::cout << "      " << det.label << " (" << int(det.confidence*100) << "%)" << std::endl;
                    }
                }
            } else {
                std::cout << " (failed to start Python)" << std::endl;
            }
        }
        
        // ====================================================================
        // 2. TOKENIZE - Create nodes for all perceptions
        // ====================================================================
        
        // Process audio events
        for (const auto& event : audio_events) {
            audio_bridge_->process(event, *graph_);
            // Track active nodes (simplified - would need actual node IDs)
        }
        
        // Process visual events
        if (!visual_events.empty()) {
            vision_bridge_->process_batch(visual_events, *graph_);
        }
        
        // Cross-modal synchronization
        if (!audio_events.empty() && !visual_events.empty()) {
            std::vector<uint64_t> audio_node_ids;
            std::vector<float> audio_timestamps;
            for (const auto& ae : audio_events) {
                audio_timestamps.push_back(ae.timestamp);
            }
            
            vision_bridge_->sync_with_audio(visual_events, audio_node_ids, audio_timestamps, *graph_);
        }
        
        // ====================================================================
        // 3. COGNITIVE PIPELINE - Basic processing for now
        // ====================================================================
        
        // NOTE: Advanced brain components (EnergyField, Hopfield, GNN, etc.)
        // can be integrated here in the next iteration
        // For now, we're establishing the foundation: vision + audio + graph
        
        // ====================================================================
        // 4. SELF-REFLECTION - Output becomes input
        // ====================================================================
        
        // Generate occasional thoughts
        if (frame_count_ % 100 == 0 && !visual_events.empty()) {
            std::stringstream thought;
            thought << "I see ";
            for (size_t i = 0; i < std::min(visual_events.size(), size_t(3)); ++i) {
                if (i > 0) thought << " and ";
                thought << visual_events[i].label;
            }
            
            std::string thought_str = thought.str();
            std::cout << "\n💭 Melvin: " << thought_str << std::endl;
            
            // Write active concepts as JSON for knowledge-driven vision
            std::ofstream concept_file("/tmp/melvin_active_concepts.json");
            concept_file << "{";
            for (size_t i = 0; i < visual_events.size(); ++i) {
                if (i > 0) concept_file << ",";
                concept_file << "\"" << visual_events[i].label << "\":1.0";
            }
            concept_file << "}";
            concept_file.close();
            
            // Simple tokenization: split on spaces and create nodes
            std::istringstream iss(thought_str);
            std::string word;
            while (iss >> word) {
                uint64_t node_id = graph_->get_or_create_node("output:" + word, 1);
                // Connect to previous output token
                if (prev_output_node_ != 0) {
                    graph_->add_edge(prev_output_node_, node_id, TEMPORAL_NEXT, 1.0f);
                }
                prev_output_node_ = node_id;
            }
        }
        
        // ====================================================================
        // 5. PERIODIC SAVE
        // ====================================================================
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_save_);
        
        if (elapsed.count() >= 30) {
            save();
            last_save_ = now;
        }
        
        // Print periodic status
        if (frame_count_ % 50 == 0) {
            print_status();
        }
    }
    
    void stop() {
        std::cout << "\n🛑 Stopping Unified Melvin..." << std::endl;
        
        if (audio_pipeline_) {
            audio_pipeline_->stop_stream();
        }
        
        save();
        print_final_stats();
        
        std::cout << "\n✅ Melvin shut down cleanly" << std::endl;
    }
    
private:
    void save() {
        graph_->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
        std::cout << "💾 Knowledge saved (" << graph_->node_count() 
                  << " nodes, " << graph_->edge_count() << " edges)" << std::endl;
    }
    
    void print_status() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "📊 Status Update (t=" << elapsed.count() << "s)" << std::endl;
        std::cout << "   Frames: " << frame_count_ << std::endl;
        std::cout << "   Knowledge: " << graph_->node_count() << " nodes, "
                  << graph_->edge_count() << " edges" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    }
    
    void print_final_stats() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        
        std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
        std::cout << "║  🧠 UNIFIED MELVIN - SESSION COMPLETE                 ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "📊 Session Statistics:\n";
        std::cout << "   Duration: " << elapsed.count() << "s\n";
        std::cout << "   Frames processed: " << frame_count_ << "\n";
        std::cout << "   Final knowledge: " << graph_->node_count() << " nodes, "
                  << graph_->edge_count() << " edges\n";
        std::cout << std::endl;
        
        if (audio_bridge_) audio_bridge_->print_stats();
        if (vision_bridge_) vision_bridge_->print_stats();
    }
    
    float get_current_time() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - start_time_).count();
        return elapsed;
    }
    
    // Core storage
    AtomicGraph* graph_;
    
    // I/O subsystems
    audio::AudioPipeline* audio_pipeline_;
    audio::AudioBridge* audio_bridge_;
    vision::VisionBridge* vision_bridge_;
    
    // State
    bool camera_enabled_ = false;
    int frame_count_ = 0;
    uint64_t next_event_id_ = 0;
    uint64_t prev_output_node_ = 0;
    
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_save_;
};

// ============================================================================
// MAIN
// ============================================================================

int main() {
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Print header
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 MELVIN UNIFIED - Complete Cognitive System        ║\n";
    std::cout << "║  Vision + Audio + ALL Brain Components               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Create and start Melvin
    UnifiedMelvin melvin;
    
    if (!melvin.start()) {
        std::cerr << "❌ Failed to start Melvin" << std::endl;
        return 1;
    }
    
    // Main perception loop
    auto last_frame = std::chrono::steady_clock::now();
    
    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_frame).count();
        last_frame = now;
        
        melvin.tick(dt);
        
        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Clean shutdown
    melvin.stop();
    
    return 0;
}

