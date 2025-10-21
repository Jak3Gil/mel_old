/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  Integration Test - Audio → Graph → Cross-Modal                           ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../core/input_manager.h"
#include "../core/atomic_graph.h"
#include "../audio/audio_bridge.h"
#include "../audio/audio_pipeline.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace melvin;
using namespace melvin::audio;

// Test utilities
void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "❌ FAILED: " << message << std::endl;
        exit(1);
    }
    std::cout << "✅ PASSED: " << message << std::endl;
}

// ============================================================================
// TEST 1: Input Manager Initialization
// ============================================================================

void test_input_manager_init() {
    std::cout << "\n🧪 Test 1: InputManager Initialization" << std::endl;
    
    InputManager::Config config;
    config.enable_audio = true;
    
    InputManager manager(config);
    
    // Don't actually start audio stream in test
    // Just verify the manager was created
    assert_true(manager.audio_pipeline().get_runtime_seconds() >= 0, 
                "AudioPipeline accessible");
    
    std::cout << "   InputManager created successfully" << std::endl;
}

// ============================================================================
// TEST 2: Audio Event → Graph Integration
// ============================================================================

void test_audio_to_graph() {
    std::cout << "\n🧪 Test 2: Audio Event → Graph Integration" << std::endl;
    
    AtomicGraph graph;
    AudioBridge bridge;
    
    // Create test audio event
    AudioEvent speech;
    speech.id = 1;
    speech.timestamp = 0.5f;
    speech.label = "turn on the stove";
    speech.type = "speech";
    speech.confidence = 0.95f;
    speech.source = "mic";
    
    // Process to graph
    size_t nodes_before = graph.node_count();
    bridge.process(speech, graph);
    size_t nodes_after = graph.node_count();
    
    assert_true(nodes_after > nodes_before, "Nodes created from speech event");
    assert_true(graph.edge_count() > 0, "Edges created from speech event");
    
    // Verify phrase node exists
    auto phrase_nodes = graph.find_nodes("audio:turn on the stove");
    assert_true(!phrase_nodes.empty(), "Phrase node created");
    
    // Verify word nodes exist
    auto word_nodes = graph.find_nodes("audio:stove");
    assert_true(!word_nodes.empty(), "Word nodes created");
    
    std::cout << "   Created " << nodes_after - nodes_before << " nodes, "
              << graph.edge_count() << " edges" << std::endl;
}

// ============================================================================
// TEST 3: Cross-Modal Audio-Vision Sync
// ============================================================================

void test_cross_modal_sync() {
    std::cout << "\n🧪 Test 3: Cross-Modal Audio-Vision Sync" << std::endl;
    
    AtomicGraph graph;
    AudioBridge bridge;
    
    // Create audio event
    AudioEvent audio_event;
    audio_event.id = 1;
    audio_event.timestamp = 0.5f;
    audio_event.label = "stove";
    audio_event.type = "speech";
    audio_event.confidence = 0.95f;
    audio_event.source = "mic";
    
    // Create visual event (simulated)
    VisualEvent visual_event;
    visual_event.id = 2;
    visual_event.timestamp = 1.1f;  // 0.6s after audio
    visual_event.label = "stove";
    visual_event.type = "object";
    visual_event.confidence = 0.90f;
    
    // Process both
    bridge.process(audio_event, graph);
    
    size_t edges_before = graph.edge_count();
    
    // Sync (should create cross-modal link)
    bridge.sync_with_vision({audio_event}, {visual_event}, graph);
    
    size_t edges_after = graph.edge_count();
    
    assert_true(edges_after > edges_before, "Cross-modal edges created");
    
    std::cout << "   Audio-vision synchronization successful" << std::endl;
    std::cout << "   Created " << edges_after - edges_before << " cross-modal edges" << std::endl;
}

// ============================================================================
// TEST 4: InputManager Full Workflow
// ============================================================================

void test_full_workflow() {
    std::cout << "\n🧪 Test 4: InputManager Full Workflow" << std::endl;
    
    InputManager::Config config;
    config.enable_audio = true;
    
    InputManager manager(config);
    AtomicGraph graph;
    
    // Simulate processing cycle
    manager.tick(0.016f);  // 60 FPS
    
    // Manually inject a test event (since we're not actually capturing audio)
    AudioEvent test_event;
    test_event.id = 1;
    test_event.timestamp = 0.0f;
    test_event.label = "hello world";
    test_event.type = "speech";
    test_event.confidence = 0.9f;
    test_event.source = "test";
    
    // Process via bridge
    manager.audio_bridge().process(test_event, graph);
    
    assert_true(graph.node_count() > 0, "Graph populated via InputManager");
    assert_true(graph.edge_count() > 0, "Edges created via InputManager");
    
    std::cout << "   Full workflow successful" << std::endl;
    std::cout << "   Graph: " << graph.node_count() << " nodes, "
              << graph.edge_count() << " edges" << std::endl;
}

// ============================================================================
// TEST 5: Persistence Integration
// ============================================================================

void test_persistence() {
    std::cout << "\n🧪 Test 5: Persistence Integration" << std::endl;
    
    AtomicGraph graph1;
    AudioBridge bridge;
    
    // Create and process events
    AudioEvent event1;
    event1.id = 1;
    event1.timestamp = 0.0f;
    event1.label = "save this";
    event1.type = "speech";
    event1.confidence = 0.9f;
    event1.source = "mic";
    
    AudioEvent event2;
    event2.id = 2;
    event2.timestamp = 1.0f;
    event2.label = "remember me";
    event2.type = "speech";
    event2.confidence = 0.9f;
    event2.source = "mic";
    
    bridge.process(event1, graph1);
    bridge.process(event2, graph1);
    
    size_t original_nodes = graph1.node_count();
    size_t original_edges = graph1.edge_count();
    
    // Save
    graph1.save("test_integration_nodes.bin", "test_integration_edges.bin");
    
    // Load into new graph
    AtomicGraph graph2;
    graph2.load("test_integration_nodes.bin", "test_integration_edges.bin");
    
    assert_true(graph2.node_count() == original_nodes, "Nodes persisted");
    assert_true(graph2.edge_count() == original_edges, "Edges persisted");
    
    std::cout << "   Persistence verified" << std::endl;
    std::cout << "   Saved and loaded " << graph2.node_count() << " nodes, "
              << graph2.edge_count() << " edges" << std::endl;
}

// ============================================================================
// TEST 6: Edge Decay Integration
// ============================================================================

void test_edge_decay() {
    std::cout << "\n🧪 Test 6: Edge Decay Integration" << std::endl;
    
    AtomicGraph graph;
    AudioBridge bridge;
    
    // Create nodes and edge
    uint64_t node_a = graph.get_or_create_node("audio:test_a", 1);
    uint64_t node_b = graph.get_or_create_node("audio:test_b", 1);
    
    graph.add_edge(node_a, node_b, Relation::CO_OCCURS_WITH, 1.0f);
    
    float weight_before = graph.get_edge_weight(node_a, node_b, Relation::CO_OCCURS_WITH);
    
    // Apply decay
    graph.decay_edges(0.9f);
    
    float weight_after = graph.get_edge_weight(node_a, node_b, Relation::CO_OCCURS_WITH);
    
    assert_true(weight_after < weight_before, "Edges decay over time");
    assert_true(std::abs(weight_after - weight_before * 0.9f) < 0.01f, "Decay rate correct");
    
    std::cout << "   Edge decay verified" << std::endl;
    std::cout << "   Weight: " << weight_before << " → " << weight_after << std::endl;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🧪 Audio Integration Test Suite                         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        test_input_manager_init();
        test_audio_to_graph();
        test_cross_modal_sync();
        test_full_workflow();
        test_persistence();
        test_edge_decay();
        
        std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  ✅ ALL INTEGRATION TESTS PASSED                         ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST SUITE FAILED: " << e.what() << std::endl;
        return 1;
    }
}

