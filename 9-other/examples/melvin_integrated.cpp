/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN - Integrated Multi-Modal Perception System                        ║
 * ║  Audio + Vision + Text unified through AtomicGraph                        ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../core/input_manager.h"
#include "../core/atomic_graph.h"
#include "../logging/audio_logger.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

using namespace melvin;
using namespace melvin::logging;

// Global flag for clean shutdown
volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n\n🛑 Received signal " << signal << ", shutting down gracefully..." << std::endl;
    g_running = false;
}

class IntegratedMelvin {
public:
    IntegratedMelvin() {
        std::cout << "🧠 Initializing Integrated Melvin..." << std::endl;
        
        // Configure input manager
        InputManager::Config config;
        config.enable_audio = true;
        config.enable_vision = false;  // Not yet implemented
        config.enable_text = true;
        config.audio_sample_rate = 16000;
        config.audio_temporal_window = 3.0f;
        
        input_manager_ = std::make_unique<InputManager>(config);
        
        std::cout << "✅ Integrated Melvin initialized" << std::endl;
    }
    
    ~IntegratedMelvin() = default;
    
    bool start() {
        std::cout << "\n📂 Loading knowledge base..." << std::endl;
        
        // Load existing knowledge
        try {
            graph_.load("melvin_integrated_nodes.bin", "melvin_integrated_edges.bin");
            std::cout << "✅ Loaded existing knowledge:" << std::endl;
            std::cout << "   Nodes: " << graph_.node_count() << std::endl;
            std::cout << "   Edges: " << graph_.edge_count() << std::endl;
        } catch (...) {
            std::cout << "📝 Starting with fresh knowledge base" << std::endl;
        }
        
        // Initialize input systems
        std::cout << "\n🚀 Starting input systems..." << std::endl;
        if (!input_manager_->init()) {
            std::cerr << "❌ Failed to initialize input manager" << std::endl;
            return false;
        }
        
        std::cout << "\n🎧 Melvin is now perceiving..." << std::endl;
        std::cout << "   (Press Ctrl+C to stop)\n" << std::endl;
        
        last_save_ = std::chrono::steady_clock::now();
        last_stats_ = last_save_;
        frame_count_ = 0;
        
        return true;
    }
    
    void tick(float dt) {
        if (!g_running) return;
        
        frame_count_++;
        
        // Process all input modalities
        input_manager_->tick(dt);
        
        // Get events from all sources
        auto audio_events = input_manager_->get_audio_events();
        // auto visual_events = input_manager_->get_visual_events(); // Future
        
        // Log and process audio events
        if (!audio_events.empty()) {
            std::cout << "┌─────────────────────────────────────────┐" << std::endl;
            std::cout << "│ New Audio Events                        │" << std::endl;
            std::cout << "└─────────────────────────────────────────┘" << std::endl;
            
            size_t nodes_before = graph_.node_count();
            size_t edges_before = graph_.edge_count();
            
            for (const auto& event : audio_events) {
                log_audio_event(event);
                
                // Process to graph
                input_manager_->audio_bridge().process(event, graph_);
            }
            
            size_t nodes_created = graph_.node_count() - nodes_before;
            size_t edges_created = graph_.edge_count() - edges_before;
            
            std::cout << "  📊 Graph delta: +" << nodes_created << " nodes, +"
                      << edges_created << " edges" << std::endl;
            std::cout << "  📈 Total: " << graph_.node_count() << " nodes, "
                      << graph_.edge_count() << " edges" << std::endl;
            std::cout << std::endl;
        }
        
        // Cross-modal synchronization
        if (!audio_events.empty()) {
            input_manager_->sync_cross_modal(graph_);
        }
        
        // Periodic edge decay
        if (frame_count_ % 10 == 0) {
            graph_.decay_edges(0.99f);
        }
        
        // Periodic statistics (every 30 seconds)
        auto now = std::chrono::steady_clock::now();
        auto stats_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_);
        if (stats_elapsed.count() >= 30) {
            print_stats();
            last_stats_ = now;
        }
        
        // Periodic save (every 60 seconds)
        auto save_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_save_);
        if (save_elapsed.count() >= 60) {
            save();
            last_save_ = now;
        }
    }
    
    void stop() {
        std::cout << "\n🛑 Stopping Integrated Melvin..." << std::endl;
        
        input_manager_->shutdown();
        save();
        
        std::cout << "\n📊 Final Session Statistics:" << std::endl;
        std::cout << "═══════════════════════════════════════════════" << std::endl;
        std::cout << "Frames processed: " << frame_count_ << std::endl;
        std::cout << std::endl;
        
        graph_.print_stats();
        std::cout << std::endl;
        
        input_manager_->print_stats();
        
        std::cout << "\n✅ Melvin shut down cleanly" << std::endl;
    }
    
private:
    void save() {
        graph_.save("melvin_integrated_nodes.bin", "melvin_integrated_edges.bin");
        std::cout << "💾 Knowledge saved (nodes: " << graph_.node_count() 
                  << ", edges: " << graph_.edge_count() << ")" << std::endl;
    }
    
    void print_stats() {
        std::cout << "\n═══════════════════════════════════════════════" << std::endl;
        std::cout << "📊 Periodic Status Update" << std::endl;
        std::cout << "═══════════════════════════════════════════════" << std::endl;
        std::cout << "Frames: " << frame_count_ << std::endl;
        std::cout << "Knowledge: " << graph_.node_count() << " nodes, "
                  << graph_.edge_count() << " edges" << std::endl;
        
        if (input_manager_->is_audio_enabled()) {
            std::cout << "Audio volume: " << input_manager_->get_audio_volume() << std::endl;
            std::cout << "Audio events: " << input_manager_->get_audio_event_count() << std::endl;
        }
        
        std::cout << "═══════════════════════════════════════════════\n" << std::endl;
    }
    
    AtomicGraph graph_;
    std::unique_ptr<InputManager> input_manager_;
    
    size_t frame_count_;
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
    std::cout << "║  🧠 MELVIN - Integrated Multi-Modal System            ║\n";
    std::cout << "║  Audio + Vision + Text → Unified Knowledge            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Create and start Melvin
    IntegratedMelvin melvin;
    
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

