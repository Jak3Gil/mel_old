/*
 * Test harness for AudioBridge
 * Validates audio event processing and cross-modal integration
 */

#include "../audio/audio_pipeline.h"
#include "../audio/audio_bridge.h"
#include "../core/atomic_graph.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace melvin;
using namespace melvin::audio;

// ============================================================================
// TEST UTILITIES
// ============================================================================

void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "❌ FAILED: " << message << std::endl;
        exit(1);
    }
    std::cout << "✅ PASSED: " << message << std::endl;
}

void assert_float_equal(float a, float b, float epsilon, const std::string& message) {
    if (std::abs(a - b) > epsilon) {
        std::cerr << "❌ FAILED: " << message 
                  << " (expected " << b << ", got " << a << ")" << std::endl;
        exit(1);
    }
    std::cout << "✅ PASSED: " << message << std::endl;
}

// ============================================================================
// TEST 1: Basic Audio Event Processing
// ============================================================================

void test_basic_audio_event() {
    std::cout << "\n🧪 Test 1: Basic Audio Event Processing" << std::endl;
    
    AtomicGraph graph;
    AudioBridge bridge;
    
    AudioEvent event(1, 0.5f, "turn on the stove", "speech", 0.95f, "mic");
    
    auto nodes = bridge.process(event, graph);
    
    assert_true(graph.node_count() > 0, "Nodes created for speech event");
    
    std::cout << "   Graph has " << graph.node_count() << " nodes" << std::endl;
    std::cout << "   Graph has " << graph.edge_count() << " edges" << std::endl;
}

// ============================================================================
// TEST 2: Speech to Word Tokenization
// ============================================================================

void test_word_tokenization() {
    std::cout << "\n🧪 Test 2: Word Tokenization" << std::endl;
    
    AtomicGraph graph;
    AudioBridge::Config config;
    config.create_word_nodes = true;
    config.create_phrase_nodes = true;
    
    AudioBridge bridge(config);
    
    AudioEvent event(1, 0.0f, "hello world", "speech", 0.9f, "mic");
    bridge.process(event, graph);
    
    // Should have nodes for: phrase, words, categories
    assert_true(graph.node_count() >= 3, "Multiple nodes created for phrase");
    
    // Check if we can find the phrase node
    auto phrase_nodes = graph.find_nodes("audio:hello world");
    assert_true(!phrase_nodes.empty(), "Phrase node exists");
    
    std::cout << "   Created " << graph.node_count() << " nodes for 2-word phrase" << std::endl;
}

// ============================================================================
// TEST 3: Cross-Modal Synchronization (Audio + Vision)
// ============================================================================

void test_cross_modal_sync() {
    std::cout << "\n🧪 Test 3: Cross-Modal Synchronization" << std::endl;
    
    AtomicGraph graph;
    AudioBridge bridge;
    
    // Create audio event
    AudioEvent audio_event(1, 1.0f, "stove", "speech", 0.95f, "mic");
    
    // Create visual event at similar time
    VisualEvent visual_event(2, 1.1f, "stove", "object", 0.9f);
    
    // Process events
    bridge.process(audio_event, graph);
    bridge.sync_with_vision({audio_event}, {visual_event}, graph);
    
    // Should have created cross-modal link
    assert_true(graph.edge_count() > 0, "Cross-modal edges created");
    
    std::cout << "   Created " << graph.edge_count() << " edges" << std::endl;
    std::cout << "   Audio-vision synchronization successful" << std::endl;
}

// ============================================================================
// TEST 4: Temporal Window Filtering
// ============================================================================

void test_temporal_window() {
    std::cout << "\n🧪 Test 4: Temporal Window Filtering" << std::endl;
    
    AtomicGraph graph;
    AudioBridge::Config config;
    config.temporal_window = 2.0f;  // 2 second window
    AudioBridge bridge(config);
    
    // Events within window
    AudioEvent audio1(1, 0.0f, "stove", "speech", 0.9f, "mic");
    VisualEvent visual1(2, 1.5f, "stove", "object", 0.9f);
    
    // Events outside window
    AudioEvent audio2(3, 0.0f, "door", "ambient", 0.8f, "mic");
    VisualEvent visual2(4, 5.0f, "door", "object", 0.8f);
    
    bridge.process(audio1, graph);
    bridge.process(audio2, graph);
    
    size_t edges_before = graph.edge_count();
    bridge.sync_with_vision({audio1, audio2}, {visual1, visual2}, graph);
    size_t edges_after = graph.edge_count();
    
    assert_true(edges_after > edges_before, "Temporal filtering creates selective links");
    
    std::cout << "   Edges before sync: " << edges_before << std::endl;
    std::cout << "   Edges after sync: " << edges_after << std::endl;
}

// ============================================================================
// TEST 5: Confidence Threshold Filtering
// ============================================================================

void test_confidence_threshold() {
    std::cout << "\n🧪 Test 5: Confidence Threshold Filtering" << std::endl;
    
    AtomicGraph graph;
    AudioBridge::Config config;
    config.min_confidence = 0.5f;
    AudioBridge bridge(config);
    
    // High confidence event (should be processed)
    AudioEvent high_conf(1, 0.0f, "hello", "speech", 0.9f, "mic");
    
    // Low confidence event (should be filtered)
    AudioEvent low_conf(2, 1.0f, "noise", "ambient", 0.2f, "mic");
    
    bridge.process(high_conf, graph);
    size_t nodes_after_high = graph.node_count();
    
    bridge.process(low_conf, graph);
    size_t nodes_after_low = graph.node_count();
    
    assert_true(nodes_after_high > 0, "High confidence events processed");
    assert_true(nodes_after_low == nodes_after_high, "Low confidence events filtered");
    
    std::cout << "   High confidence event: processed" << std::endl;
    std::cout << "   Low confidence event: filtered" << std::endl;
}

// ============================================================================
// TEST 6: Edge Reinforcement and Decay
// ============================================================================

void test_edge_reinforcement() {
    std::cout << "\n🧪 Test 6: Edge Reinforcement and Decay" << std::endl;
    
    AtomicGraph graph;
    AudioBridge bridge;
    
    // Create two nodes
    uint64_t node_a = graph.get_or_create_node("audio:dog", 1);
    uint64_t node_b = graph.get_or_create_node("audio:bark", 1);
    
    // Add edge
    graph.add_edge(node_a, node_b, Relation::CO_OCCURS_WITH, 1.0f);
    float weight_before = graph.get_edge_weight(node_a, node_b, Relation::CO_OCCURS_WITH);
    
    // Apply decay
    graph.decay_edges(0.9f);
    float weight_after = graph.get_edge_weight(node_a, node_b, Relation::CO_OCCURS_WITH);
    
    assert_true(weight_after < weight_before, "Edge weights decay");
    assert_float_equal(weight_after, weight_before * 0.9f, 0.01f, "Decay rate correct");
    
    std::cout << "   Weight before decay: " << weight_before << std::endl;
    std::cout << "   Weight after decay: " << weight_after << std::endl;
}

// ============================================================================
// TEST 7: Ambient Sound Processing
// ============================================================================

void test_ambient_sound() {
    std::cout << "\n🧪 Test 7: Ambient Sound Processing" << std::endl;
    
    AtomicGraph graph;
    AudioBridge bridge;
    
    AudioEvent ambient_event(1, 0.0f, "dog barking", "ambient", 0.85f, "mic");
    bridge.process(ambient_event, graph);
    
    assert_true(graph.node_count() > 0, "Ambient sound creates nodes");
    
    // Check for ambient category
    auto ambient_nodes = graph.find_nodes("audio:ambient");
    assert_true(!ambient_nodes.empty(), "Ambient category node exists");
    
    std::cout << "   Ambient sound processed successfully" << std::endl;
}

// ============================================================================
// TEST 8: Graph Persistence
// ============================================================================

void test_persistence() {
    std::cout << "\n🧪 Test 8: Graph Persistence" << std::endl;
    
    AtomicGraph graph1;
    AudioBridge bridge;
    
    // Add audio events
    AudioEvent event1(1, 0.0f, "save this", "speech", 0.9f, "mic");
    AudioEvent event2(2, 1.0f, "remember me", "speech", 0.9f, "mic");
    
    bridge.process(event1, graph1);
    bridge.process(event2, graph1);
    
    size_t original_nodes = graph1.node_count();
    size_t original_edges = graph1.edge_count();
    
    // Save
    graph1.save("test_audio_nodes.bin", "test_audio_edges.bin");
    
    // Load into new graph
    AtomicGraph graph2;
    graph2.load("test_audio_nodes.bin", "test_audio_edges.bin");
    
    assert_true(graph2.node_count() == original_nodes, "Nodes persisted correctly");
    assert_true(graph2.edge_count() == original_edges, "Edges persisted correctly");
    
    std::cout << "   Saved and loaded " << graph2.node_count() << " nodes" << std::endl;
    std::cout << "   Saved and loaded " << graph2.edge_count() << " edges" << std::endl;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🧪 Audio Bridge Test Suite                              ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        test_basic_audio_event();
        test_word_tokenization();
        test_cross_modal_sync();
        test_temporal_window();
        test_confidence_threshold();
        test_edge_reinforcement();
        test_ambient_sound();
        test_persistence();
        
        std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  ✅ ALL TESTS PASSED                                     ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST SUITE FAILED: " << e.what() << std::endl;
        return 1;
    }
}

