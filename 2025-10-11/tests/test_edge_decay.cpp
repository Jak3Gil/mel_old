/**
 * test_edge_decay.cpp - Unit tests for Edge Decay System
 * 
 * Tests Ebbinghaus forgetting curve with simulated time.
 * 
 * Tests:
 * 1. Basic decay over time (7 days → 50% weight)
 * 2. Reinforcement resets decay timer
 * 3. Floor clamping (never below minimum)
 * 4. Max clamping (never above maximum)
 * 5. Multiple decay cycles
 * 6. Decay statistics accuracy
 */

#include "../src/learning/edge_decay.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

// Edge structure
struct Edge {
    uint64_t u, v;
    float weight = 1.0f;
    std::string rel;
    uint32_t count = 0;
    uint32_t last_access_time = 0;
    
    void update_frequency(uint64_t) {}
};

using namespace melvin::learning;

// Test 1: Basic decay (7 days → 50% weight)
void test_basic_decay() {
    std::cout << "[TEST 1] Basic decay (half-life)... ";
    
    DecayConfig config;
    config.half_life_days = 7.0;
    config.floor = 0.05;
    config.max = 4.0;
    config.enabled = true;
    config.check_interval_hours = 0.0;  // No interval check for testing
    
    SimulatedClock clock(1000.0);  // Start at t=1000s
    EdgeDecay decay(config, &clock);
    
    // Create edge with initial weight 1.0
    Edge edge;
    edge.u = 1;
    edge.v = 2;
    edge.weight = 1.0f;
    edge.last_access_time = static_cast<uint32_t>(clock.now_seconds());
    
    // Advance 7 days (half-life)
    clock.advance(7.0 * 86400.0);  // 7 days in seconds
    
    std::vector<Edge> edges = {edge};
    auto stats = decay.apply_decay(edges);
    
    // After 7 days, weight should be ≈ 0.5 (within tolerance)
    // w(7d) = 0.05 + (1.0 - 0.05) * 0.5^(7/7) = 0.05 + 0.95 * 0.5 = 0.525
    double expected = 0.525;
    double actual = edges[0].weight;
    double tolerance = 0.05;
    
    assert(std::abs(actual - expected) < tolerance);
    assert(stats.edges_decayed > 0);
    
    std::cout << "✓ PASSED" << std::endl;
    std::cout << "    Initial: 1.0, After 7 days: " << actual << " (expected: ~" << expected << ")" << std::endl;
}

// Test 2: Reinforcement resets timer and increases weight
void test_reinforcement() {
    std::cout << "[TEST 2] Reinforcement resets decay... ";
    
    DecayConfig config;
    config.half_life_days = 7.0;
    config.floor = 0.05;
    config.max = 4.0;
    
    SimulatedClock clock(1000.0);
    EdgeDecay decay(config, &clock);
    
    Edge edge;
    edge.u = 1;
    edge.v = 2;
    edge.weight = 1.0f;
    edge.last_access_time = static_cast<uint32_t>(clock.now_seconds());
    
    // Advance 3 days
    clock.advance(3.0 * 86400.0);
    
    // Reinforce before it decays much
    decay.reinforce_edge(edge, 0.2);
    
    // Should have: weight increased, timer reset
    assert(edge.weight > 1.0f);
    assert(edge.last_access_time == static_cast<uint32_t>(clock.now_seconds()));
    
    // Advance another 7 days from reinforcement
    clock.advance(7.0 * 86400.0);
    
    std::vector<Edge> edges = {edge};
    decay.apply_decay(edges);
    
    // Decay should be from the reinforced weight, not original
    // Reinforced weight was 1.2, so after 7 days:
    // w = 0.05 + (1.2 - 0.05) * 0.5 = 0.05 + 0.575 = 0.625
    assert(edges[0].weight > 0.5f);
    
    std::cout << "✓ PASSED" << std::endl;
    std::cout << "    Reinforcement reset timer and weight increased correctly" << std::endl;
}

// Test 3: Floor clamping
void test_floor_clamping() {
    std::cout << "[TEST 3] Floor clamping... ";
    
    DecayConfig config;
    config.half_life_days = 1.0;  // Fast decay for testing
    config.floor = 0.1;
    config.max = 4.0;
    
    SimulatedClock clock(1000.0);
    EdgeDecay decay(config, &clock);
    
    Edge edge;
    edge.weight = 0.5f;
    edge.last_access_time = static_cast<uint32_t>(clock.now_seconds());
    
    // Advance 30 days (many half-lives)
    clock.advance(30.0 * 86400.0);
    
    std::vector<Edge> edges = {edge};
    decay.apply_decay(edges);
    
    // Should be clamped to floor
    assert(edges[0].weight >= config.floor - 0.001);
    assert(edges[0].weight <= config.floor + 0.001);
    
    std::cout << "✓ PASSED" << std::endl;
    std::cout << "    Weight clamped to floor: " << edges[0].weight << std::endl;
}

// Test 4: Max clamping on reinforcement
void test_max_clamping() {
    std::cout << "[TEST 4] Max clamping on reinforcement... ";
    
    DecayConfig config;
    config.half_life_days = 7.0;
    config.floor = 0.05;
    config.max = 2.0;
    
    SimulatedClock clock(1000.0);
    EdgeDecay decay(config, &clock);
    
    Edge edge;
    edge.weight = 1.8f;
    edge.last_access_time = static_cast<uint32_t>(clock.now_seconds());
    
    // Reinforce heavily
    decay.reinforce_edge(edge, 5.0);
    
    // Should be clamped to max
    assert(edge.weight <= config.max + 0.001);
    assert(edge.weight >= config.max - 0.001);
    
    std::cout << "✓ PASSED" << std::endl;
    std::cout << "    Weight clamped to max: " << edge.weight << std::endl;
}

// Test 5: Progressive decay over time
void test_multiple_cycles() {
    std::cout << "[TEST 5] Progressive decay over time... ";
    
    DecayConfig config;
    config.half_life_days = 7.0;
    config.floor = 0.05;
    config.max = 4.0;
    config.check_interval_hours = 0.0;  // Allow every decay call
    
    SimulatedClock clock(1000.0);
    EdgeDecay decay(config, &clock);
    
    // Test decay at different time points
    Edge edge1, edge2, edge3;
    edge1.weight = edge2.weight = edge3.weight = 1.0f;
    uint32_t start_time = static_cast<uint32_t>(clock.now_seconds());
    edge1.last_access_time = start_time;
    edge2.last_access_time = start_time;
    edge3.last_access_time = start_time;
    
    // Test 1 day decay
    clock.set_time(start_time + 1.0 * 86400.0);
    std::vector<Edge> edges1 = {edge1};
    decay.apply_decay(edges1);
    double weight_1d = edges1[0].weight;
    
    // Test 2 day decay
    clock.set_time(start_time + 2.0 * 86400.0);
    std::vector<Edge> edges2 = {edge2};
    decay.apply_decay(edges2);
    double weight_2d = edges2[0].weight;
    
    // Test 7 day decay
    clock.set_time(start_time + 7.0 * 86400.0);
    std::vector<Edge> edges3 = {edge3};
    decay.apply_decay(edges3);
    double weight_7d = edges3[0].weight;
    
    // Progressive decay
    assert(weight_1d < 1.0f);
    assert(weight_2d < weight_1d);
    assert(weight_7d < weight_2d);
    
    // 7 days should be ~0.525
    assert(std::abs(weight_7d - 0.525) < 0.05);
    
    std::cout << "✓ PASSED" << std::endl;
    std::cout << "    1d: " << weight_1d 
              << ", 2d: " << weight_2d
              << ", 7d: " << weight_7d << std::endl;
}

// Test 6: Decay statistics
void test_decay_stats() {
    std::cout << "[TEST 6] Decay statistics... ";
    
    DecayConfig config;
    config.half_life_days = 7.0;
    config.floor = 0.1;
    config.max = 4.0;
    config.check_interval_hours = 0.0;
    
    SimulatedClock clock(1000.0);
    EdgeDecay decay(config, &clock);
    
    // Create 10 edges
    std::vector<Edge> edges;
    for (int i = 0; i < 10; i++) {
        Edge e;
        e.u = i;
        e.v = i + 1;
        e.weight = 1.0f;
        e.last_access_time = static_cast<uint32_t>(clock.now_seconds());
        edges.push_back(e);
    }
    
    // Advance 7 days
    clock.advance(7.0 * 86400.0);
    
    auto stats = decay.apply_decay(edges);
    
    // Verify statistics
    assert(stats.edges_checked == 10);
    assert(stats.edges_decayed == 10);  // All should decay
    assert(stats.avg_weight_before > stats.avg_weight_after);
    assert(stats.total_weight_lost > 0.0);
    
    std::cout << "✓ PASSED" << std::endl;
    std::cout << "    Edges checked: " << stats.edges_checked << std::endl;
    std::cout << "    Edges decayed: " << stats.edges_decayed << std::endl;
    std::cout << "    Avg weight: " << stats.avg_weight_before 
              << " → " << stats.avg_weight_after << std::endl;
    std::cout << "    Total weight lost: " << stats.total_weight_lost << std::endl;
}

// Main test runner
int main() {
    std::cout << "╔═══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                       ║" << std::endl;
    std::cout << "║        EDGE DECAY SYSTEM - UNIT TESTS                ║" << std::endl;
    std::cout << "║         Phase 2 - Ebbinghaus Forgetting              ║" << std::endl;
    std::cout << "║                                                       ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    try {
        test_basic_decay();
        test_reinforcement();
        test_floor_clamping();
        test_max_clamping();
        test_multiple_cycles();
        test_decay_stats();
        
        std::cout << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "✅ ALL TESTS PASSED (6/6)" << std::endl;
        std::cout << std::endl;
        std::cout << "Phase 2 Complete:" << std::endl;
        std::cout << "  ✓ Ebbinghaus decay curve implemented" << std::endl;
        std::cout << "  ✓ Half-life model working (7 days → 50%)" << std::endl;
        std::cout << "  ✓ Reinforcement resets decay timer" << std::endl;
        std::cout << "  ✓ Floor/max clamping operational" << std::endl;
        std::cout << "  ✓ Decay statistics accurate" << std::endl;
        std::cout << "  ✓ Time simulation working" << std::endl;
        std::cout << std::endl;
        std::cout << "Edge decay system is fully operational!" << std::endl;
        std::cout << "Melvin can now forget unused knowledge naturally." << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}

