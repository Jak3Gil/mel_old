/*
 * Build proper graph structure by parsing facts into concept nodes
 */

#include "melvin/core/storage.h"
#include "melvin/core/leap_inference.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

using namespace melvin;

// Parse "A relation B" into (A, B) nodes with edge
void parse_and_add_fact(Storage* storage, const std::string& fact) {
    std::istringstream iss(fact);
    std::string from, relation, to;
    
    // Simple parser: "A relation B" or "A relation B relation C..."
    iss >> from;
    iss >> relation;  // Skip relation word
    iss >> to;
    
    if (from.empty() || to.empty()) return;
    
    // Create or find nodes
    auto from_nodes = storage->find_nodes(from);
    NodeID from_id;
    if (from_nodes.empty()) {
        from_id = storage->create_node(from, NodeType::CONCEPT);
    } else {
        from_id = from_nodes[0].id;
    }
    
    auto to_nodes = storage->find_nodes(to);
    NodeID to_id;
    if (to_nodes.empty()) {
        to_id = storage->create_node(to, NodeType::CONCEPT);
    } else {
        to_id = to_nodes[0].id;
    }
    
    // Create EXACT edge
    storage->create_edge(from_id, to_id, RelationType::EXACT, 1.0f);
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  BUILDING PROPER GRAPH STRUCTURE              ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    
    auto storage = std::make_unique<Storage>();
    
    // Physics chains
    std::vector<std::string> facts = {
        // Fire → heat → warmth → comfort (3-hop chain!)
        "fire produces heat",
        "heat produces warmth",
        "warmth creates comfort",
        
        // Fire → light → vision → seeing
        "fire produces light",
        "light enables vision",
        "vision allows seeing",
        
        // Heat chains
        "heat causes expansion",
        "expansion increases volume",
        "volume occupies space",
        
        "heat transfers energy",
        "energy powers motion",
        "motion creates work",
        
        // Water cycle (long chain!)
        "water evaporates",
        "evaporation creates vapor",
        "vapor forms clouds",
        "clouds produce rain",
        "rain falls downward",
        "rain soaks ground",
        "ground absorbs water",
        
        // Sun → light → photosynthesis → glucose → energy → life
        "sun produces light",
        "light enables photosynthesis",
        "photosynthesis creates glucose",
        "glucose stores energy",
        "energy supports life",
        
        // Plants chain
        "plants perform photosynthesis",
        "plants need water",
        "plants need sunlight",
        "plants produce oxygen",
        "oxygen supports life",
        
        // Animals
        "animals need oxygen",
        "animals need water",
        "animals need food",
        "food provides energy",
        
        // Mammals
        "dogs are mammals",
        "cats are mammals",
        "humans are mammals",
        "mammals have fur",
        "fur provides warmth",
        "mammals are warm_blooded",
        
        // Astronomy
        "sun is star",
        "star produces light",
        "star produces heat",
        "heat warms planets",
        "earth is planet",
        "planet orbits star",
        "gravity causes orbit",
        "gravity pulls objects",
        "objects have mass",
        
        // Chemistry
        "water contains hydrogen",
        "water contains oxygen",
        "hydrogen is element",
        "oxygen is element",
        "elements form molecules",
        "molecules form matter",
        "matter has mass",
        
        // Causality chains
        "exercise causes fitness",
        "fitness improves health",
        "health enables wellbeing",
        
        "reading increases knowledge",
        "knowledge enables understanding",
        "understanding creates wisdom",
        
        "practice improves skill",
        "skill enables mastery",
        "mastery requires dedication"
    };
    
    std::cout << "Parsing " << facts.size() << " facts into graph...\n\n";
    
    for (const auto& fact : facts) {
        parse_and_add_fact(storage.get(), fact);
        std::cout << "  ✓ " << fact << "\n";
    }
    
    std::cout << "\n";
    storage->print_stats();
    
    // Now create LEAP connections
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  CREATING LEAP CONNECTIONS                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 5;
    leap_config.min_shared_neighbors = 2;
    leap_config.leap_confidence = 0.7f;
    leap_config.verbose = true;
    
    leap::LEAPInference leap_system(leap_config);
    int leaps_created = leap_system.create_leap_connections(storage.get());
    
    std::cout << "\n✅ Created " << leaps_created << " LEAP connections!\n\n";
    
    storage->print_stats();
    
    // Save
    std::cout << "\n💾 Saving knowledge base...\n";
    if (storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cout << "✅ Saved to melvin/data/\n";
    }
    
    // Show example chains
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  EXAMPLE MULTI-HOP REASONING                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    
    struct Test { std::string from, to; };
    std::vector<Test> tests = {
        {"fire", "comfort"},
        {"fire", "seeing"},
        {"water", "clouds"},
        {"sun", "life"},
        {"exercise", "wellbeing"}
    };
    
    for (const auto& test : tests) {
        auto from_nodes = storage->find_nodes(test.from);
        auto to_nodes = storage->find_nodes(test.to);
        
        if (!from_nodes.empty() && !to_nodes.empty()) {
            auto paths = storage->find_paths(from_nodes[0].id, to_nodes[0].id, 10);
            
            if (!paths.empty()) {
                std::cout << test.from << " → " << test.to << ": ";
                std::cout << paths[0].hop_count() << " hops (";
                
                // Count EXACT vs LEAP
                int exact_count = 0, leap_count = 0;
                for (auto edge_id : paths[0].edges) {
                    Edge e;
                    if (storage->get_edge(edge_id, e)) {
                        if (e.relation == RelationType::EXACT) exact_count++;
                        else leap_count++;
                    }
                }
                
                std::cout << exact_count << " EXACT, " << leap_count << " LEAP)\n";
                
                // Show path
                std::cout << "  Path: ";
                for (size_t i = 0; i < paths[0].nodes.size(); i++) {
                    std::cout << storage->get_node_content(paths[0].nodes[i]);
                    if (i < paths[0].nodes.size() - 1) std::cout << " → ";
                }
                std::cout << "\n\n";
            } else {
                std::cout << test.from << " → " << test.to << ": No path found\n\n";
            }
        }
    }
    
    std::cout << "╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  READY FOR AUTONOMOUS LEARNING!               ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    
    return 0;
}

