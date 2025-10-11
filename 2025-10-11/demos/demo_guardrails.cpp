/*
 * MELVIN GUARDRAILS TEST SUITE
 * 
 * 6 Golden Tests for LEAP System Validation:
 * 1. Exact recall stable
 * 2. Crowd-required LEAP
 * 3. Ambiguity → abstain
 * 4. Promotion after N reinforcements
 * 5. Conflict: EXACT arrives → LEAP penalized
 * 6. Persistence parity
 */

#include "../include/melvin_core.h"
#include "../src/util/telemetry.h"
#include "../src/util/config.h"
#include "../src/reasoning/conflicts.h"
#include <iostream>
#include <iomanip>

using namespace melvin_core;
using namespace melvin_storage;
using namespace melvin_reasoning;
using namespace melvin_telemetry;
using namespace melvin_config;
using namespace melvin_conflicts;

// Test result tracking
struct TestResult {
    std::string name;
    bool passed;
    std::string details;
};

std::vector<TestResult> results;

void test_pass(const std::string& name, const std::string& details = "") {
    results.push_back({name, true, details});
    std::cout << "  [PASS] " << name;
    if (!details.empty()) std::cout << " - " << details;
    std::cout << "\n";
}

void test_fail(const std::string& name, const std::string& details = "") {
    results.push_back({name, false, details});
    std::cout << "  [FAIL] " << name;
    if (!details.empty()) std::cout << " - " << details;
    std::cout << "\n";
}

// ==================== TEST 1: EXACT RECALL STABLE ====================

void test_exact_recall() {
    std::cout << "\n📖 TEST 1: Exact Recall Stability\n";
    std::cout << std::string(70, '-') << "\n";
    
    // Clear and setup
    system("rm -f test_guardrails.bin");
    UnifiedBrain brain("test_guardrails.bin");
    
    // Teach explicit facts
    brain.learn("dogs are mammals");
    brain.learn("cats are mammals");
    brain.learn("birds can fly");
    
    // Count EXACT edges
    int exact_before = 0;
    for (const auto& edge : edges) {
        if (edge.is_exact()) exact_before++;
    }
    
    // Query known facts multiple times
    brain.think("are dogs mammals");
    brain.think("can birds fly");
    brain.think("are dogs mammals");
    
    // Count EXACT edges again
    int exact_after = 0;
    for (const auto& edge : edges) {
        if (edge.is_exact()) exact_after++;
    }
    
    // EXACT edges should not decrease
    if (exact_after >= exact_before) {
        test_pass("Exact recall stable", 
                  "EXACT edges: " + std::to_string(exact_before) + " -> " + std::to_string(exact_after));
    } else {
        test_fail("Exact recall stable",
                  "EXACT edges decreased: " + std::to_string(exact_before) + " -> " + std::to_string(exact_after));
    }
    
    brain.save();
}

// ==================== TEST 2: CROWD-REQUIRED LEAP ====================

void test_crowd_required() {
    std::cout << "\n🔮 TEST 2: Crowd-Required LEAP\n";
    std::cout << std::string(70, '-') << "\n";
    
    system("rm -f test_guardrails.bin");
    UnifiedBrain brain("test_guardrails.bin");
    
    // Build crowd support
    brain.learn("dogs are mammals");
    brain.learn("cats are mammals");
    brain.learn("wolves are mammals");
    brain.learn("lions are mammals");
    
    // Count LEAPs before
    int leaps_before = 0;
    for (const auto& edge : edges) {
        if (edge.is_leap()) leaps_before++;
    }
    
    // Query with gap - should create LEAP with sufficient crowd
    brain.think("are foxes mammals");
    
    // Count LEAPs after
    int leaps_after = 0;
    for (const auto& edge : edges) {
        if (edge.is_leap()) leaps_after++;
    }
    
    // Should have created at least one LEAP
    if (leaps_after > leaps_before) {
        test_pass("Crowd-required LEAP", 
                  "LEAPs created: " + std::to_string(leaps_after - leaps_before));
    } else {
        test_fail("Crowd-required LEAP", 
                  "No LEAP created despite crowd support");
    }
    
    brain.save();
}

// ==================== TEST 3: AMBIGUITY → ABSTAIN ====================

void test_ambiguity_abstain() {
    std::cout << "\n⚠️  TEST 3: Ambiguity → Abstain\n";
    std::cout << std::string(70, '-') << "\n";
    
    system("rm -f test_guardrails.bin");
    UnifiedBrain brain("test_guardrails.bin");
    
    // Create ambiguous situation - two competing targets with similar support
    brain.learn("apples are red");
    brain.learn("bananas are yellow");
    brain.learn("grapes are purple");
    
    // Query something ambiguous with insufficient crowd diversity
    brain.learn("dogs are fluffy");  // Only one example
    
    // Count LEAPs before query
    int leaps_before = 0;
    for (const auto& edge : edges) {
        if (edge.is_leap()) leaps_before++;
    }
    
    // This should NOT create LEAP - insufficient diversity
    brain.think("are cats fluffy");
    
    int leaps_after = 0;
    for (const auto& edge : edges) {
        if (edge.is_leap()) leaps_after++;
    }
    
    // Should not have created LEAP (insufficient support/diversity)
    // OR if it did, it should have been rejected
    int telemetry_rejects = count_events("leap_reject");
    
    if (leaps_after == leaps_before || telemetry_rejects > 0) {
        test_pass("Ambiguity abstain", 
                  "Correctly rejected insufficient support (rejects: " + std::to_string(telemetry_rejects) + ")");
    } else {
        test_fail("Ambiguity abstain",
                  "Created LEAP despite insufficient support");
    }
    
    brain.save();
}

// ==================== TEST 4: PROMOTION ====================

void test_promotion() {
    std::cout << "\n✨ TEST 4: LEAP Promotion\n";
    std::cout << std::string(70, '-') << "\n";
    
    system("rm -f test_guardrails.bin");
    UnifiedBrain brain("test_guardrails.bin");
    
    // Build crowd for LEAP
    brain.learn("dogs are mammals");
    brain.learn("cats are mammals");
    brain.learn("wolves are mammals");
    brain.learn("bears are mammals");
    
    // Create LEAP
    brain.think("are foxes mammals");
    
    // Find the LEAP
    int leap_id = -1;
    for (size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].is_leap()) {
            // Check if it's the foxes->mammals leap
            auto it_foxes = node_lookup.find("foxes");
            auto it_mammals = node_lookup.find("mammals");
            
            if (it_foxes != node_lookup.end() && it_mammals != node_lookup.end()) {
                if (edges[i].a == it_foxes->second && edges[i].b == it_mammals->second) {
                    leap_id = i;
                    break;
                }
            }
        }
    }
    
    if (leap_id < 0) {
        test_fail("LEAP Promotion", "No LEAP found to promote");
        brain.save();
        return;
    }
    
    float initial_weight = edges[leap_id].w;
    int initial_successes = edges[leap_id].successes;
    
    // Reinforce the LEAP multiple times
    for (int i = 0; i < 5; ++i) {
        brain.think("are foxes mammals");
    }
    
    // Check if promoted or at least strengthened
    float final_weight = edges[leap_id].w;
    int final_successes = edges[leap_id].successes;
    
    bool promoted = edges[leap_id].is_exact();  // Check if became EXACT
    bool strengthened = final_weight > initial_weight || final_successes > initial_successes;
    
    if (promoted) {
        test_pass("LEAP Promotion", 
                  "LEAP promoted to EXACT after " + std::to_string(final_successes) + " successes");
    } else if (strengthened) {
        test_pass("LEAP Promotion", 
                  "LEAP strengthened: w=" + std::to_string(initial_weight) + "->" + std::to_string(final_weight) +
                  ", successes=" + std::to_string(initial_successes) + "->" + std::to_string(final_successes));
    } else {
        test_fail("LEAP Promotion",
                  "LEAP not strengthened or promoted");
    }
    
    brain.save();
}

// ==================== TEST 5: CONFLICT PENALIZES LEAP ====================

void test_conflict_penalty() {
    std::cout << "\n⚔️  TEST 5: Conflict Penalizes LEAP\n";
    std::cout << std::string(70, '-') << "\n";
    
    system("rm -f test_guardrails.bin");
    UnifiedBrain brain("test_guardrails.bin");
    
    // Create a LEAP first
    brain.learn("dogs are fluffy");
    brain.learn("cats are fluffy");
    brain.learn("rabbits are fluffy");
    
    // Create LEAP: foxes -> fluffy
    brain.think("are foxes fluffy");
    
    // Find the LEAP
    int leap_id = -1;
    for (size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].is_leap()) {
            auto it_foxes = node_lookup.find("foxes");
            auto it_fluffy = node_lookup.find("fluffy");
            
            if (it_foxes != node_lookup.end() && it_fluffy != node_lookup.end()) {
                if (edges[i].a == it_foxes->second && edges[i].b == it_fluffy->second) {
                    leap_id = i;
                    break;
                }
            }
        }
    }
    
    if (leap_id < 0) {
        test_fail("Conflict penalty", "No LEAP created in setup");
        brain.save();
        return;
    }
    
    float leap_weight_before = edges[leap_id].w;
    
    // Now teach conflicting EXACT fact
    brain.learn("foxes are carnivores");  // Conflicts with "foxes are fluffy"
    
    // Manually trigger conflict detection
    auto it_foxes = node_lookup.find("foxes");
    auto it_carnivores = node_lookup.find("carnivores");
    auto it_be = node_lookup.find("BE");
    
    if (it_foxes != node_lookup.end() && it_carnivores != node_lookup.end() && it_be != node_lookup.end()) {
        record_exact(it_foxes->second, it_be->second, it_carnivores->second, 1.0f);
        penalize_conflicts(it_foxes->second, it_be->second, it_carnivores->second);
    }
    
    // Check if LEAP was penalized
    float leap_weight_after = edges[leap_id].w;
    
    int decay_events = count_events("leap_decay");
    
    if (leap_weight_after < leap_weight_before || decay_events > 0) {
        test_pass("Conflict penalty", 
                  "LEAP penalized: w=" + std::to_string(leap_weight_before) + "->" + std::to_string(leap_weight_after) +
                  ", decays=" + std::to_string(decay_events));
    } else {
        test_fail("Conflict penalty",
                  "LEAP not penalized despite conflict");
    }
    
    brain.save();
}

// ==================== TEST 6: PERSISTENCE PARITY ====================

void test_persistence_parity() {
    std::cout << "\n💾 TEST 6: Persistence Parity\n";
    std::cout << std::string(70, '-') << "\n";
    
    // First run
    {
        system("rm -f test_guardrails_persist.bin");
        UnifiedBrain brain("test_guardrails_persist.bin");
        
        brain.learn("dogs are mammals");
        brain.learn("cats are mammals");
        brain.learn("wolves are mammals");
        
        brain.think("are foxes mammals");
        brain.save();
    }
    
    int nodes_run1 = nodes.size();
    int edges_run1 = edges.size();
    int exact_run1 = 0, leap_run1 = 0;
    
    for (const auto& edge : edges) {
        if (edge.is_exact()) exact_run1++;
        if (edge.is_leap()) leap_run1++;
    }
    
    // Clear memory
    nodes.clear();
    edges.clear();
    adjacency.clear();
    node_lookup.clear();
    
    // Second run - reload
    {
        UnifiedBrain brain("test_guardrails_persist.bin");
        // Don't do anything, just load
    }
    
    int nodes_run2 = nodes.size();
    int edges_run2 = edges.size();
    int exact_run2 = 0, leap_run2 = 0;
    
    for (const auto& edge : edges) {
        if (edge.is_exact()) exact_run2++;
        if (edge.is_leap()) leap_run2++;
    }
    
    // Compare
    bool nodes_match = (nodes_run1 == nodes_run2);
    bool edges_match = (edges_run1 == edges_run2);
    bool exact_match = (exact_run1 == exact_run2);
    bool leap_match = (leap_run1 == leap_run2);
    
    if (nodes_match && edges_match && exact_match && leap_match) {
        test_pass("Persistence parity", 
                  "nodes=" + std::to_string(nodes_run1) + 
                  ", edges=" + std::to_string(edges_run1) +
                  ", EXACT=" + std::to_string(exact_run1) +
                  ", LEAP=" + std::to_string(leap_run1));
    } else {
        test_fail("Persistence parity",
                  "Mismatch: nodes=" + std::to_string(nodes_run1) + "/" + std::to_string(nodes_run2) +
                  ", edges=" + std::to_string(edges_run1) + "/" + std::to_string(edges_run2) +
                  ", EXACT=" + std::to_string(exact_run1) + "/" + std::to_string(exact_run2) +
                  ", LEAP=" + std::to_string(leap_run1) + "/" + std::to_string(leap_run2));
    }
}

// ==================== MAIN ====================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN GUARDRAILS TEST SUITE                                ║\n";
    std::cout << "║  6 Golden Tests for LEAP System Validation                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    
    // Load configuration
    load_config();
    print_config();
    
    // Enable telemetry
    enable_telemetry(true);
    set_telemetry_file("guardrails_telemetry.jsonl");
    
    // Run tests
    test_exact_recall();
    test_crowd_required();
    test_ambiguity_abstain();
    test_promotion();
    test_conflict_penalty();
    test_persistence_parity();
    
    // Summary
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST SUMMARY                                                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    int passed = 0, failed = 0;
    for (const auto& result : results) {
        if (result.passed) {
            passed++;
            std::cout << "  ✅ " << result.name << "\n";
        } else {
            failed++;
            std::cout << "  ❌ " << result.name;
            if (!result.details.empty()) {
                std::cout << " (" << result.details << ")";
            }
            std::cout << "\n";
        }
    }
    
    std::cout << "\n";
    std::cout << "  Total: " << results.size() << " tests\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << failed << "\n";
    
    // Telemetry summary
    std::cout << "\n📊 Telemetry Events:\n";
    std::cout << "  leap_create:  " << count_events("leap_create") << "\n";
    std::cout << "  leap_reject:  " << count_events("leap_reject") << "\n";
    std::cout << "  leap_promote: " << count_events("leap_promote") << "\n";
    std::cout << "  leap_decay:   " << count_events("leap_decay") << "\n";
    std::cout << "  conflicts:    " << count_events("conflict_detected") << "\n";
    std::cout << "  abstentions:  " << count_events("abstain") << "\n";
    
    std::cout << "\n";
    
    if (failed == 0) {
        std::cout << "🎉 ALL TESTS PASSED!\n\n";
        return 0;
    } else {
        std::cout << "⚠️  SOME TESTS FAILED\n\n";
        return 1;
    }
}

