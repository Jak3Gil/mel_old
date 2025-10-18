/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN WITH AUDIO PERCEPTION                                             ║
 * ║  Full integration example showing audio + knowledge graph                 ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../core/atomic_graph.h"
#include "../audio/audio_pipeline.h"
#include "../audio/audio_bridge.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

using namespace melvin;
using namespace melvin::audio;

// Global flag for clean shutdown
volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n\n🛑 Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

class MelvinWithAudio {
public:
    MelvinWithAudio() {
        std::cout << "🧠 Initializing Melvin with Audio Perception..." << std::endl;
        
        // Configure audio pipeline
        AudioPipeline::Config audio_config;
        audio_config.sample_rate = 16000;
        audio_config.buffer_size_ms = 3000;
        audio_config.vad_threshold = 0.02f;
        audio_config.enable_speech = true;
        audio_config.enable_ambient = true;
        
        audio_pipeline_ = new AudioPipeline(audio_config);
        
        // Configure audio bridge
        AudioBridge::Config bridge_config;
        bridge_config.temporal_window = 3.0f;
        bridge_config.enable_cross_modal = true;
        bridge_config.enable_causal_inference = true;
        bridge_config.create_word_nodes = true;
        
        audio_bridge_ = new AudioBridge(bridge_config);
        
        std::cout << "✅ Melvin initialized" << std::endl;
    }
    
    ~MelvinWithAudio() {
        delete audio_pipeline_;
        delete audio_bridge_;
    }
    
    bool start() {
        std::cout << "\n📂 Loading knowledge base..." << std::endl;
        
        // Load existing knowledge
        try {
            graph_.load("melvin_nodes.bin", "melvin_edges.bin");
            std::cout << "✅ Loaded existing knowledge:" << std::endl;
            std::cout << "   Nodes: " << graph_.node_count() << std::endl;
            std::cout << "   Edges: " << graph_.edge_count() << std::endl;
        } catch (...) {
            std::cout << "📝 Starting with fresh knowledge base" << std::endl;
        }
        
        // Start audio capture
        std::cout << "\n🎤 Starting audio capture..." << std::endl;
        if (!audio_pipeline_->start_stream()) {
            std::cerr << "❌ Failed to start audio stream" << std::endl;
            return false;
        }
        
        std::cout << "✅ Audio stream started" << std::endl;
        std::cout << "\n🎧 Melvin is now listening..." << std::endl;
        std::cout << "   (Press Ctrl+C to stop)\n" << std::endl;
        
        last_save_ = std::chrono::steady_clock::now();
        last_stats_ = last_save_;
        
        return true;
    }
    
    void tick(float dt) {
        if (!g_running) return;
        
        // Process audio
        audio_pipeline_->tick(dt);
        auto audio_events = audio_pipeline_->get_recent_events();
        
        // Integrate audio into knowledge graph
        if (!audio_events.empty()) {
            for (const auto& event : audio_events) {
                std::cout << "🎤 [" << event.type << "] \"" << event.label 
                          << "\" (confidence: " << event.confidence << ")" << std::endl;
                
                audio_bridge_->process(event, graph_);
            }
            
            // Apply learning dynamics
            audio_bridge_->reinforce_patterns(graph_, 0.99f);
            
            std::cout << "   📊 Graph: " << graph_.node_count() << " nodes, "
                      << graph_.edge_count() << " edges" << std::endl;
            std::cout << std::endl;
        }
        
        // Periodic statistics (every 10 seconds)
        auto now = std::chrono::steady_clock::now();
        auto stats_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_);
        if (stats_elapsed.count() >= 10) {
            print_stats();
            last_stats_ = now;
        }
        
        // Periodic save (every 30 seconds)
        auto save_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_save_);
        if (save_elapsed.count() >= 30) {
            save();
            last_save_ = now;
        }
    }
    
    void stop() {
        std::cout << "\n🛑 Stopping Melvin..." << std::endl;
        
        audio_pipeline_->stop_stream();
        save();
        
        std::cout << "\n📊 Final Session Statistics:" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        graph_.print_stats();
        std::cout << std::endl;
        audio_bridge_->print_stats();
        
        std::cout << "\n✅ Melvin shut down cleanly" << std::endl;
    }
    
private:
    void save() {
        graph_.save("melvin_nodes.bin", "melvin_edges.bin");
        std::cout << "💾 Knowledge saved (nodes: " << graph_.node_count() 
                  << ", edges: " << graph_.edge_count() << ")" << std::endl;
    }
    
    void print_stats() {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "📊 Periodic Status Update" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "   Knowledge: " << graph_.node_count() << " nodes, "
                  << graph_.edge_count() << " edges" << std::endl;
        std::cout << "   Volume: " << audio_pipeline_->get_current_volume() << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    }
    
    AtomicGraph graph_;
    AudioPipeline* audio_pipeline_;
    AudioBridge* audio_bridge_;
    
    std::chrono::steady_clock::time_point last_save_;
    std::chrono::steady_clock::time_point last_stats_;
};

int main() {
    // Setup signal handler for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Print header
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 MELVIN WITH AUDIO PERCEPTION                      ║\n";
    std::cout << "║  Real-time auditory learning system                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Create and start Melvin
    MelvinWithAudio melvin;
    
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

