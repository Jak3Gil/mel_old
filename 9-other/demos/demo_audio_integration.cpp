/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN AUDIO INTEGRATION DEMO                                            ║
 * ║  Demonstrates full audio perception pipeline                              ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../audio/audio_pipeline.h"
#include "../audio/audio_bridge.h"
#include "../core/atomic_graph.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace melvin;
using namespace melvin::audio;

// ============================================================================
// SIMULATED VISION SYSTEM
// ============================================================================

class SimulatedVision {
public:
    std::vector<VisualEvent> get_recent_events(float current_time) {
        // Simulate detecting objects
        std::vector<VisualEvent> events;
        
        // Simulate seeing a stove at t=1.0s
        if (current_time > 1.0f && current_time < 1.5f && !stove_seen_) {
            events.push_back(VisualEvent(100, current_time, "stove", "object", 0.92f));
            stove_seen_ = true;
            std::cout << "👁️  Vision: Detected 'stove' at t=" << current_time << "s" << std::endl;
        }
        
        return events;
    }
    
private:
    bool stove_seen_ = false;
};

// ============================================================================
// DEMO SCENARIOS
// ============================================================================

void demo_basic_audio_capture() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 1: Basic Audio Capture & Processing                ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioPipeline::Config pipeline_config;
    pipeline_config.enable_speech = true;
    pipeline_config.enable_ambient = true;
    
    AudioPipeline audio(pipeline_config);
    AudioBridge bridge;
    
    // Start audio stream (would capture from mic in real system)
    audio.start_stream();
    
    // Simulate some audio events
    std::cout << "📝 Simulating audio events...\n" << std::endl;
    
    AudioEvent event1(1, 0.5f, "turn on the stove", "speech", 0.95f, "mic");
    event1.volume = 0.7f;
    event1.duration = 1.2f;
    
    AudioEvent event2(2, 2.0f, "dog barking", "ambient", 0.82f, "mic");
    event2.volume = 0.5f;
    event2.duration = 0.8f;
    
    // Process events
    bridge.process(event1, graph);
    bridge.process(event2, graph);
    
    // Print results
    std::cout << "\n📊 Results:" << std::endl;
    graph.print_stats();
    bridge.print_stats();
    
    audio.stop_stream();
}

void demo_cross_modal_integration() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 2: Cross-Modal Integration (Audio + Vision)        ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioPipeline audio;
    AudioBridge bridge;
    SimulatedVision vision;
    
    audio.start_stream();
    
    std::cout << "🎬 Simulating real-time multimodal perception...\n" << std::endl;
    
    // Timeline:
    // t=0.5s: User says "turn on the stove"
    // t=1.1s: Vision detects stove
    // t=3.0s: Sync creates cross-modal link
    
    std::vector<AudioEvent> audio_events;
    std::vector<VisualEvent> visual_events;
    
    // Simulate perception loop
    for (float t = 0.0f; t <= 3.5f; t += 0.5f) {
        std::cout << "⏱️  t=" << t << "s" << std::endl;
        
        // Audio events
        if (t == 0.5f) {
            AudioEvent speech(1, t, "turn on the stove", "speech", 0.95f, "mic");
            audio_events.push_back(speech);
            bridge.process(speech, graph);
        }
        
        // Vision events
        auto vision_events_now = vision.get_recent_events(t);
        visual_events.insert(visual_events.end(), vision_events_now.begin(), vision_events_now.end());
        
        // Cross-modal sync
        if (t >= 3.0f) {
            std::cout << "\n🔗 Running cross-modal synchronization..." << std::endl;
            bridge.sync_with_vision(audio_events, visual_events, graph);
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\n📊 Results:" << std::endl;
    graph.print_stats();
    bridge.print_stats();
    
    // Save graph
    std::cout << "\n💾 Saving knowledge graph..." << std::endl;
    graph.save("audio_demo_nodes.bin", "audio_demo_edges.bin");
    std::cout << "   Saved to audio_demo_nodes.bin / audio_demo_edges.bin" << std::endl;
    
    audio.stop_stream();
}

void demo_reflection_mode() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 3: Reflection Mode (Learning from Past Audio)      ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    
    // Load previous session
    std::cout << "📂 Loading previous session..." << std::endl;
    
    try {
        graph.load("audio_demo_nodes.bin", "audio_demo_edges.bin");
        std::cout << "✅ Previous session loaded:" << std::endl;
        graph.print_stats();
        
        // Analyze what was learned
        std::cout << "\n🔍 Analyzing learned associations..." << std::endl;
        
        auto stove_nodes = graph.find_nodes("audio:stove");
        if (!stove_nodes.empty()) {
            uint64_t stove_node = stove_nodes[0];
            auto neighbors = graph.neighbors(stove_node);
            
            std::cout << "   'stove' is connected to:" << std::endl;
            for (auto neighbor_id : neighbors) {
                const Node* neighbor = graph.get_node(neighbor_id);
                if (neighbor) {
                    std::cout << "     - " << neighbor->label << std::endl;
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "⚠️  No previous session found. Run demo 2 first." << std::endl;
        std::cout << "   (Error: " << e.what() << ")" << std::endl;
    }
}

void demo_continuous_learning() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 4: Continuous Learning Loop                        ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioPipeline audio;
    AudioBridge bridge;
    
    audio.start_stream();
    
    std::cout << "🔄 Running continuous learning loop..." << std::endl;
    std::cout << "   (Simulating 5 iterations)\n" << std::endl;
    
    float time = 0.0f;
    
    for (int iteration = 1; iteration <= 5; iteration++) {
        std::cout << "🔁 Iteration " << iteration << " (t=" << time << "s)" << std::endl;
        
        // Simulate varied audio input
        std::string phrases[] = {
            "turn on the light",
            "open the door", 
            "close the window",
            "start the music",
            "stop the timer"
        };
        
        AudioEvent event(iteration, time, phrases[iteration - 1], "speech", 0.9f, "mic");
        bridge.process(event, graph);
        
        // Apply learning dynamics
        bridge.reinforce_patterns(graph, 0.95f);  // 5% decay
        
        std::cout << "   Nodes: " << graph.node_count() 
                  << " | Edges: " << graph.edge_count() << std::endl;
        
        time += 2.0f;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "\n📊 Final Results:" << std::endl;
    graph.print_stats();
    
    // Save learned knowledge
    graph.save("audio_continuous_nodes.bin", "audio_continuous_edges.bin");
    
    audio.stop_stream();
}

// ============================================================================
// MAIN DEMO SELECTOR
// ============================================================================

void print_menu() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🎤 MELVIN AUDIO INTEGRATION DEMO                         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\nSelect a demo:" << std::endl;
    std::cout << "  1. Basic Audio Capture & Processing" << std::endl;
    std::cout << "  2. Cross-Modal Integration (Audio + Vision)" << std::endl;
    std::cout << "  3. Reflection Mode (Learning from Past Audio)" << std::endl;
    std::cout << "  4. Continuous Learning Loop" << std::endl;
    std::cout << "  5. Run All Demos" << std::endl;
    std::cout << "  0. Exit" << std::endl;
    std::cout << "\nChoice: ";
}

int main(int argc, char** argv) {
    std::cout << "\n";
    
    // If argument provided, run that demo directly
    if (argc > 1) {
        int choice = std::atoi(argv[1]);
        
        switch (choice) {
            case 1: demo_basic_audio_capture(); break;
            case 2: demo_cross_modal_integration(); break;
            case 3: demo_reflection_mode(); break;
            case 4: demo_continuous_learning(); break;
            case 5:
                demo_basic_audio_capture();
                demo_cross_modal_integration();
                demo_reflection_mode();
                demo_continuous_learning();
                break;
            default:
                std::cerr << "Invalid demo number" << std::endl;
                return 1;
        }
        
        std::cout << "\n✨ Demo complete!\n" << std::endl;
        return 0;
    }
    
    // Interactive menu
    while (true) {
        print_menu();
        
        int choice;
        std::cin >> choice;
        
        if (choice == 0) {
            std::cout << "\n👋 Goodbye!\n" << std::endl;
            break;
        }
        
        switch (choice) {
            case 1: demo_basic_audio_capture(); break;
            case 2: demo_cross_modal_integration(); break;
            case 3: demo_reflection_mode(); break;
            case 4: demo_continuous_learning(); break;
            case 5:
                demo_basic_audio_capture();
                demo_cross_modal_integration();
                demo_reflection_mode();
                demo_continuous_learning();
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
    
    return 0;
}

