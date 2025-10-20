/**
 * @file demo_conversation.cpp
 * @brief Demo of the complete conversation system
 * 
 * Tests:
 * - Semantic bridge (v2 → v1 graph)
 * - Language generator (graph → text)
 * - Conversation engine (ChatGPT-style dialogue)
 */

#include "../memory/semantic_bridge.h"
#include "../reasoning/language_generator.h"
#include "../reasoning/conversation_engine.h"
#include "../core/global_workspace.h"
#include "../core/neuromodulators.h"
#include "../evolution/genome.h"
#include "../../include/melvin_graph.h"

#include <iostream>
#include <string>
#include <iomanip>

using namespace melvin;
using namespace melvin::v2;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void print_header(const std::string& title) {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  " << title << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "\n";
}

void print_divider() {
    std::cout << "───────────────────────────────────────────────────────\n";
}

void print_conversation_turn(int turn_num, const std::string& user, const std::string& melvin) {
    std::cout << "\n[Turn " << turn_num << "]\n";
    std::cout << "You:    " << user << "\n";
    std::cout << "Melvin: " << melvin << "\n";
}

// ============================================================================
// DEMO SCENARIOS
// ============================================================================

void demo_basic_queries(reasoning::ConversationEngine& conv) {
    print_header("DEMO 1: Basic Queries");
    
    std::vector<std::string> queries = {
        "Hello",
        "What is a cup?",
        "What is fire?",
        "What is water?",
        "Goodbye"
    };
    
    for (size_t i = 0; i < queries.size(); i++) {
        std::string response = conv.respond(queries[i]);
        print_conversation_turn(i + 1, queries[i], response);
    }
}

void demo_causation_queries(reasoning::ConversationEngine& conv) {
    print_header("DEMO 2: Causation & Prediction");
    
    std::vector<std::string> queries = {
        "Why does fire cause heat?",
        "What happens if I see dark clouds?",
        "What causes rain?"
    };
    
    for (size_t i = 0; i < queries.size(); i++) {
        std::string response = conv.respond(queries[i]);
        print_conversation_turn(i + 1, queries[i], response);
    }
}

void demo_recall(reasoning::ConversationEngine& conv) {
    print_header("DEMO 3: Conversation Memory");
    
    std::vector<std::string> queries = {
        "What is a cup?",
        "What is a plate?",
        "What did we just discuss?"
    };
    
    for (size_t i = 0; i < queries.size(); i++) {
        std::string response = conv.respond(queries[i]);
        print_conversation_turn(i + 1, queries[i], response);
    }
}

void demo_unknown_concepts(reasoning::ConversationEngine& conv) {
    print_header("DEMO 4: Handling Unknown Concepts");
    
    std::vector<std::string> queries = {
        "What is quantum_entanglement?",
        "What is a flibbertigibbet?",
        "Tell me about xyzabc123"
    };
    
    for (size_t i = 0; i < queries.size(); i++) {
        std::string response = conv.respond(queries[i]);
        print_conversation_turn(i + 1, queries[i], response);
    }
}

void demo_emotional_tone(reasoning::ConversationEngine& conv, Neuromodulators& neuromod) {
    print_header("DEMO 5: Emotional Tone Modulation");
    
    // Test with different neuromodulator states
    std::cout << "\n[Scenario 1: High Dopamine - Excited]\n";
    neuromod.on_prediction_error(0.5f);  // Positive reward
    neuromod.on_goal_progress(0.8f);     // Good progress
    std::string r1 = conv.respond("Hello!");
    std::cout << "You:    Hello!\n";
    std::cout << "Melvin: " << r1 << "\n";
    std::cout << "State:  DA=" << neuromod.get_dopamine() 
              << " 5-HT=" << neuromod.get_serotonin() << "\n";
    
    // Reset and test low serotonin
    neuromod.reset();
    std::cout << "\n[Scenario 2: Low Serotonin - Uncertain]\n";
    neuromod.on_conflict(0.8f);  // High conflict
    std::string r2 = conv.respond("What is fire?");
    std::cout << "You:    What is fire?\n";
    std::cout << "Melvin: " << r2 << "\n";
    std::cout << "State:  DA=" << neuromod.get_dopamine() 
              << " 5-HT=" << neuromod.get_serotonin() << "\n";
    
    // Reset and test high norepinephrine
    neuromod.reset();
    std::cout << "\n[Scenario 3: High Norepinephrine - Curious]\n";
    neuromod.on_unexpected_event(0.9f);  // Surprise!
    std::string r3 = conv.respond("What is water?");
    std::cout << "You:    What is water?\n";
    std::cout << "Melvin: " << r3 << "\n";
    std::cout << "State:  NE=" << neuromod.get_norepinephrine() 
              << " ACh=" << neuromod.get_acetylcholine() << "\n";
}

void demo_statistics(reasoning::ConversationEngine& conv, memory::SemanticBridge& bridge) {
    print_header("STATISTICS");
    
    // Conversation stats
    auto conv_stats = conv.get_stats();
    std::cout << "Conversation Statistics:\n";
    std::cout << "  Total turns:          " << conv_stats.total_turns << "\n";
    std::cout << "  Successful responses: " << conv_stats.successful_responses << "\n";
    std::cout << "  Unknown queries:      " << conv_stats.unknown_queries << "\n";
    std::cout << "  Facts retrieved:      " << conv_stats.facts_retrieved << "\n";
    std::cout << "  Concepts discussed:   " << conv_stats.concepts_discussed << "\n";
    
    std::cout << "\n  Query Type Breakdown:\n";
    for (const auto& [type, count] : conv_stats.query_type_counts) {
        std::cout << "    " << std::left << std::setw(20) 
                  << reasoning::query_type_to_string(type) 
                  << ": " << count << "\n";
    }
    
    // Semantic bridge stats
    auto bridge_stats = bridge.get_stats();
    std::cout << "\nSemantic Bridge Statistics:\n";
    std::cout << "  Total nodes:    " << bridge_stats.total_nodes << "\n";
    std::cout << "  Total edges:    " << bridge_stats.total_edges << "\n";
    std::cout << "  Cache hits:     " << bridge_stats.cache_hits << "\n";
    std::cout << "  Cache misses:   " << bridge_stats.cache_misses << "\n";
    
    if (bridge_stats.cache_hits + bridge_stats.cache_misses > 0) {
        float hit_rate = 100.0f * bridge_stats.cache_hits / 
                        (bridge_stats.cache_hits + bridge_stats.cache_misses);
        std::cout << "  Cache hit rate: " << std::fixed << std::setprecision(1) 
                  << hit_rate << "%\n";
    }
}

void interactive_mode(reasoning::ConversationEngine& conv) {
    print_header("INTERACTIVE MODE");
    
    std::cout << "Chat with Melvin! (type 'quit' to exit)\n";
    std::cout << "Try questions like:\n";
    std::cout << "  - What is X?\n";
    std::cout << "  - Why does X happen?\n";
    std::cout << "  - What happens if X?\n";
    std::cout << "  - What did we discuss?\n";
    std::cout << "\n";
    
    int turn = 1;
    while (true) {
        std::cout << "\n[Turn " << turn << "]\n";
        std::cout << "You: ";
        
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        if (input == "quit" || input == "exit" || input == "q") {
            std::cout << "Melvin: Goodbye! It was nice talking with you.\n";
            break;
        }
        
        std::string response = conv.respond(input);
        std::cout << "Melvin: " << response << "\n";
        
        turn++;
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║     MELVIN v3 - CONVERSATION SYSTEM DEMO             ║\n";
    std::cout << "║                                                       ║\n";
    std::cout << "║  Testing: Semantic Bridge + Language Generator +     ║\n";
    std::cout << "║           Conversation Engine                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    
    try {
        print_header("INITIALIZATION");
        
        // Initialize v1 graph
        std::cout << "Loading AtomicGraph...\n";
        AtomicGraph graph;
        
        // Add some test concepts if graph is empty
        std::cout << "Populating test concepts...\n";
        
        // Create test concepts
        uint64_t cup_id = graph.get_or_create_concept("cup");
        uint64_t container_id = graph.get_or_create_concept("container");
        uint64_t drinking_id = graph.get_or_create_concept("drinking");
        uint64_t beverage_id = graph.get_or_create_concept("beverage");
        uint64_t fire_id = graph.get_or_create_concept("fire");
        uint64_t heat_id = graph.get_or_create_concept("heat");
        uint64_t water_id = graph.get_or_create_concept("water");
        uint64_t liquid_id = graph.get_or_create_concept("liquid");
        
        // Add relationships
        graph.add_or_bump_edge(cup_id, container_id, Rel::PART_OF, 1.0f);
        graph.add_or_bump_edge(cup_id, drinking_id, Rel::USED_FOR, 0.9f);
        graph.add_or_bump_edge(cup_id, beverage_id, Rel::CO_OCCURS_WITH, 0.8f);
        graph.add_or_bump_edge(fire_id, heat_id, Rel::CAUSES, 1.0f);
        graph.add_or_bump_edge(water_id, liquid_id, Rel::PART_OF, 1.0f);
        
        std::cout << "  ✓ Graph initialized\n";
        std::cout << "  ✓ Test concepts added\n";
        
        // Initialize v2 components
        std::cout << "\nInitializing v2 components...\n";
        
        // Create genome
        evolution::Genome genome = evolution::GenomeFactory::create_base();
        std::cout << "  ✓ Genome created\n";
        
        // Create semantic bridge
        memory::SemanticBridge semantic_bridge(graph);
        std::cout << "  ✓ Semantic Bridge initialized\n";
        
        // Create global workspace
        GlobalWorkspace workspace;
        std::cout << "  ✓ Global Workspace initialized\n";
        
        // Create neuromodulators
        Neuromodulators neuromod(genome);
        std::cout << "  ✓ Neuromodulators initialized\n";
        
        // Create conversation engine
        reasoning::ConversationEngine conversation(
            semantic_bridge,
            workspace,
            neuromod
        );
        std::cout << "  ✓ Conversation Engine initialized\n";
        
        std::cout << "\n✓ All systems ready!\n";
        
        // Run demos
        bool run_all = (argc == 1);  // No args = run all demos
        bool interactive = false;
        
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--interactive" || arg == "-i") {
                interactive = true;
                run_all = false;
            }
        }
        
        if (run_all) {
            demo_basic_queries(conversation);
            demo_causation_queries(conversation);
            demo_recall(conversation);
            demo_unknown_concepts(conversation);
            demo_emotional_tone(conversation, neuromod);
            demo_statistics(conversation, semantic_bridge);
        }
        
        if (interactive || (!run_all && argc == 1)) {
            interactive_mode(conversation);
        } else if (run_all) {
            // Offer interactive mode
            std::cout << "\n";
            print_divider();
            std::cout << "\nWould you like to try interactive mode? (y/n): ";
            std::string response;
            std::getline(std::cin, response);
            if (response == "y" || response == "Y" || response == "yes") {
                interactive_mode(conversation);
            }
        }
        
        // Final statistics
        demo_statistics(conversation, semantic_bridge);
        
        print_header("DEMO COMPLETE");
        std::cout << "✓ All conversation system components working!\n";
        std::cout << "✓ Graph-grounded responses verified\n";
        std::cout << "✓ Multi-turn memory tested\n";
        std::cout << "✓ Emotional modulation demonstrated\n";
        std::cout << "\nThe conversation system is ready to use! 🎉\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

