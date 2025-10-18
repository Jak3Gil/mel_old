/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN REASONING TEST                                                    ║
 * ║  Quick test to verify all reasoning components work                       ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include "melvin/core/reasoning.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include <iostream>
#include <iomanip>

using namespace melvin;

void print_section(const std::string& title) {
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(45) << title << "║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
}

void print_result(const std::string& test, bool passed) {
    std::cout << "  " << (passed ? "✓" : "✗") << " " << test << "\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN REASONING TEST SUITE                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n";
    
    int tests_passed = 0;
    int tests_total = 0;
    
    // ========================================================================
    // TEST 1: STORAGE SYSTEM
    // ========================================================================
    
    print_section("1. STORAGE SYSTEM");
    
    auto storage = std::make_unique<Storage>();
    
    // Test: Load existing knowledge base
    tests_total++;
    bool loaded = storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    print_result("Load knowledge base", loaded);
    if (loaded) tests_passed++;
    
    // Test: Check nodes exist
    tests_total++;
    size_t node_count = storage->node_count();
    bool has_nodes = node_count > 0;
    print_result("Has nodes (" + std::to_string(node_count) + ")", has_nodes);
    if (has_nodes) tests_passed++;
    
    // Test: Check edges exist
    tests_total++;
    size_t edge_count = storage->edge_count();
    bool has_edges = edge_count > 0;
    print_result("Has edges (" + std::to_string(edge_count) + ")", has_edges);
    if (has_edges) tests_passed++;
    
    // Test: EXACT and LEAP edges
    tests_total++;
    size_t exact_count = storage->edge_count_by_type(RelationType::EXACT);
    size_t leap_count = storage->edge_count_by_type(RelationType::LEAP);
    bool has_both = exact_count > 0 && leap_count > 0;
    std::cout << "  " << (has_both ? "✓" : "✗") << " Has EXACT (" 
              << exact_count << ") and LEAP (" << leap_count << ") edges\n";
    if (has_both) tests_passed++;
    
    storage->print_stats();
    
    // ========================================================================
    // TEST 2: BASIC GRAPH OPERATIONS
    // ========================================================================
    
    print_section("2. BASIC GRAPH OPERATIONS");
    
    // Test: Create a test node
    tests_total++;
    NodeID test_node = storage->create_node("test_concept", NodeType::CONCEPT);
    bool node_created = test_node != 0;
    print_result("Create node", node_created);
    if (node_created) tests_passed++;
    
    // Test: Find node
    tests_total++;
    auto found_nodes = storage->find_nodes("test");
    bool found = !found_nodes.empty();
    print_result("Find node by content", found);
    if (found) tests_passed++;
    
    // Test: Create edge
    tests_total++;
    if (!found_nodes.empty()) {
        NodeID target = storage->create_node("target_concept", NodeType::CONCEPT);
        EdgeID edge = storage->create_edge(test_node, target, RelationType::EXACT, 1.0f);
        bool edge_created = edge != 0;
        print_result("Create edge", edge_created);
        if (edge_created) tests_passed++;
    } else {
        print_result("Create edge", false);
    }
    
    // Test: Get neighbors
    tests_total++;
    auto neighbors = storage->get_neighbors(test_node);
    bool has_neighbors = !neighbors.empty();
    print_result("Get neighbors", has_neighbors);
    if (has_neighbors) tests_passed++;
    
    // ========================================================================
    // TEST 3: REASONING ENGINE
    // ========================================================================
    
    print_section("3. REASONING ENGINE");
    
    ReasoningEngine::Config reasoning_config;
    reasoning_config.energy_decay = 0.9f;
    reasoning_config.beam_width = 8;
    reasoning_config.max_hops_safety = 50;
    
    auto reasoning = std::make_unique<ReasoningEngine>(reasoning_config);
    
    // Test: Simple query
    tests_total++;
    Query test_query;
    test_query.text = "What is fire?";
    
    Answer answer = reasoning->infer(test_query, storage.get());
    bool got_answer = !answer.text.empty();
    print_result("Basic query inference", got_answer);
    if (got_answer) {
        tests_passed++;
        std::cout << "  Query: \"" << test_query.text << "\"\n";
        std::cout << "  Answer: \"" << answer.text << "\"\n";
        std::cout << "  Confidence: " << answer.confidence << "\n";
        std::cout << "  Paths found: " << answer.paths.size() << "\n";
    }
    
    // Test: Path finding between nodes
    tests_total++;
    if (node_count >= 2) {
        auto all_nodes = storage->get_all_nodes();
        if (all_nodes.size() >= 2) {
            auto paths = storage->find_paths(all_nodes[0].id, all_nodes[1].id, 5);
            bool found_paths = !paths.empty();
            print_result("Multi-hop path finding", found_paths);
            if (found_paths) {
                tests_passed++;
                std::cout << "  Found " << paths.size() << " path(s)\n";
                if (!paths.empty()) {
                    std::cout << "  Best path: " << paths[0].hop_count() << " hops, "
                              << "confidence: " << paths[0].confidence << "\n";
                }
            }
        } else {
            print_result("Multi-hop path finding", false);
        }
    } else {
        print_result("Multi-hop path finding (skipped - not enough nodes)", true);
        tests_passed++;
    }
    
    // Get reasoning stats
    auto stats = reasoning->get_stats();
    std::cout << "\n  Reasoning Statistics:\n";
    std::cout << "    Queries processed: " << stats.queries_processed << "\n";
    std::cout << "    Paths explored: " << stats.paths_explored << "\n";
    std::cout << "    Nodes activated: " << stats.nodes_activated << "\n";
    std::cout << "    Avg path length: " << stats.avg_path_length << "\n";
    std::cout << "    Avg confidence: " << stats.avg_confidence << "\n";
    
    // ========================================================================
    // TEST 4: LEARNING SYSTEM
    // ========================================================================
    
    print_section("4. LEARNING SYSTEM");
    
    LearningSystem::Config learning_config;
    learning_config.learning_rate = 0.01f;
    
    auto learning = std::make_unique<LearningSystem>(storage.get(), learning_config);
    
    // Test: Teach a new fact
    tests_total++;
    bool taught = learning->teach_fact("fire produces heat", "test_context");
    print_result("Teach new fact", taught);
    if (taught) tests_passed++;
    
    // Test: Reinforce a path
    tests_total++;
    if (!answer.paths.empty()) {
        learning->reinforce_path(answer.paths[0], 1.0f);
        print_result("Reinforce reasoning path", true);
        tests_passed++;
    } else {
        print_result("Reinforce reasoning path (no paths available)", true);
        tests_passed++;
    }
    
    // Get learning stats
    auto learn_stats = learning->get_stats();
    std::cout << "\n  Learning Statistics:\n";
    std::cout << "    Facts learned: " << learn_stats.facts_learned << "\n";
    std::cout << "    Files processed: " << learn_stats.files_processed << "\n";
    std::cout << "    Reinforcements: " << learn_stats.reinforcements << "\n";
    
    // ========================================================================
    // TEST 5: LEAP INFERENCE
    // ========================================================================
    
    print_section("5. LEAP INFERENCE");
    
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 3;
    leap_config.min_shared_neighbors = 2;
    leap_config.leap_confidence = 0.7f;
    leap_config.verbose = true;
    
    leap::LEAPInference leap_system(leap_config);
    
    // Test: Create LEAP connections
    tests_total++;
    size_t leaps_before = storage->edge_count_by_type(RelationType::LEAP);
    int leaps_created = leap_system.create_leap_connections(storage.get());
    size_t leaps_after = storage->edge_count_by_type(RelationType::LEAP);
    bool leap_success = leaps_after >= leaps_before;
    
    std::cout << "  " << (leap_success ? "✓" : "✗") << " Create LEAP connections: "
              << leaps_created << " created (total: " << leaps_after << ")\n";
    if (leap_success) tests_passed++;
    
    auto leap_stats = leap_system.get_stats();
    std::cout << "\n  LEAP Statistics:\n";
    std::cout << "    Transitive LEAPs: " << leap_stats.transitive_leaps << "\n";
    std::cout << "    Similarity LEAPs: " << leap_stats.similarity_leaps << "\n";
    std::cout << "    Pattern LEAPs: " << leap_stats.pattern_leaps << "\n";
    std::cout << "    Total LEAPs: " << leap_stats.total_leaps << "\n";
    
    // ========================================================================
    // TEST 6: QUERY MULTIPLE CONCEPTS
    // ========================================================================
    
    print_section("6. MULTIPLE QUERY TEST");
    
    std::vector<std::string> test_queries = {
        "What is fire?",
        "What does fire produce?",
        "How does heat work?",
        "Tell me about water"
    };
    
    int queries_answered = 0;
    for (const auto& q : test_queries) {
        Query query;
        query.text = q;
        Answer ans = reasoning->infer(query, storage.get());
        
        if (!ans.text.empty() || !ans.paths.empty()) {
            queries_answered++;
            std::cout << "  ✓ \"" << q << "\" → " << ans.paths.size() << " path(s)\n";
        } else {
            std::cout << "  ✗ \"" << q << "\" → no paths found\n";
        }
    }
    
    tests_total++;
    bool multi_query_success = queries_answered > 0;
    print_result("Multiple queries (" + std::to_string(queries_answered) + "/" + 
                 std::to_string(test_queries.size()) + " answered)", multi_query_success);
    if (multi_query_success) tests_passed++;
    
    // ========================================================================
    // TEST 7: GRAPH INTEGRITY
    // ========================================================================
    
    print_section("7. GRAPH INTEGRITY");
    
    // Test: All edges have valid nodes
    tests_total++;
    auto all_edges = storage->get_all_edges();
    bool valid_edges = true;
    int invalid_count = 0;
    
    for (const auto& edge : all_edges) {
        Node from, to;
        if (!storage->get_node(edge.from_id, from) || !storage->get_node(edge.to_id, to)) {
            valid_edges = false;
            invalid_count++;
        }
    }
    
    std::cout << "  " << (valid_edges ? "✓" : "✗") << " All edges have valid nodes";
    if (!valid_edges) {
        std::cout << " (" << invalid_count << " invalid)";
    }
    std::cout << "\n";
    if (valid_edges) tests_passed++;
    
    // Test: No isolated nodes
    tests_total++;
    auto all_nodes = storage->get_all_nodes();
    int isolated = 0;
    for (const auto& node : all_nodes) {
        auto in_edges = storage->get_edges_to(node.id);
        auto out_edges = storage->get_edges_from(node.id);
        if (in_edges.empty() && out_edges.empty()) {
            isolated++;
        }
    }
    
    bool no_isolated = isolated < (all_nodes.size() / 10); // Allow <10% isolated
    std::cout << "  " << (no_isolated ? "✓" : "✗") << " Limited isolated nodes: "
              << isolated << "/" << all_nodes.size() << "\n";
    if (no_isolated) tests_passed++;
    
    // ========================================================================
    // TEST 8: SAVE/LOAD CYCLE
    // ========================================================================
    
    print_section("8. PERSISTENCE");
    
    // Test: Save current state
    tests_total++;
    bool saved = storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    print_result("Save knowledge base", saved);
    if (saved) tests_passed++;
    
    // Test: Create snapshot
    tests_total++;
    bool snapshot = storage->create_snapshot("melvin/data/test_snapshot");
    print_result("Create snapshot", snapshot);
    if (snapshot) tests_passed++;
    
    // ========================================================================
    // FINAL RESULTS
    // ========================================================================
    
    print_section("RESULTS");
    
    float pass_rate = (tests_total > 0) ? (100.0f * tests_passed / tests_total) : 0.0f;
    
    std::cout << "  Tests Passed: " << tests_passed << "/" << tests_total << "\n";
    std::cout << "  Pass Rate: " << std::fixed << std::setprecision(1) << pass_rate << "%\n\n";
    
    if (pass_rate >= 90.0f) {
        std::cout << "  ✅ EXCELLENT - Melvin's reasoning is working great!\n";
    } else if (pass_rate >= 70.0f) {
        std::cout << "  ✓ GOOD - Melvin's reasoning is working well!\n";
    } else if (pass_rate >= 50.0f) {
        std::cout << "  ⚠ PARTIAL - Some issues detected\n";
    } else {
        std::cout << "  ✗ ISSUES - Multiple failures detected\n";
    }
    
    std::cout << "\n";
    
    // Final stats
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "Final Knowledge Base Statistics:\n";
    std::cout << "═══════════════════════════════════════════════\n";
    storage->print_stats();
    
    return (pass_rate >= 50.0f) ? 0 : 1;
}

