/*
 * Large Knowledge Base - Interconnected Facts for LEAP Formation
 */

#include "melvin/core/storage.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include <iostream>
#include <vector>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  BUILDING LARGE KNOWLEDGE BASE                ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    
    auto storage = std::make_unique<Storage>();
    auto learning = std::make_unique<LearningSystem>(storage.get());
    
    // Physics & Chemistry (multi-hop chains!)
    std::vector<std::string> physics_facts = {
        "fire produces heat",
        "heat produces warmth",
        "warmth creates comfort",
        "fire produces light",
        "light enables vision",
        "vision allows seeing",
        "heat causes expansion",
        "expansion increases volume",
        "volume occupies space",
        "heat transfers energy",
        "energy powers motion",
        "motion creates work"
    };
    
    // Water cycle (creates multiple LEAPs!)
    std::vector<std::string> water_facts = {
        "water evaporates",
        "evaporation creates vapor",
        "vapor forms clouds",
        "clouds produce rain",
        "rain falls down",
        "rain soaks ground",
        "ground absorbs water",
        "water flows downhill",
        "water reaches ocean",
        "ocean contains salt",
        "salt dissolves",
        "water is wet"
    };
    
    // Biology (complex chains)
    std::vector<std::string> biology_facts = {
        "sun produces light",
        "light enables photosynthesis",
        "photosynthesis creates glucose",
        "glucose stores energy",
        "plants perform photosynthesis",
        "plants need water",
        "plants need sunlight",
        "plants need carbon_dioxide",
        "plants produce oxygen",
        "oxygen supports life",
        "life requires energy",
        "animals need oxygen",
        "animals need water",
        "animals need food",
        "food provides energy"
    };
    
    // Mammals & Animals
    std::vector<std::string> animal_facts = {
        "dogs are mammals",
        "cats are mammals",
        "humans are mammals",
        "mammals have fur",
        "fur provides warmth",
        "mammals are warm_blooded",
        "warm_blooded maintains temperature",
        "mammals nurse young",
        "young grow",
        "growth requires nutrition",
        "nutrition comes from food"
    };
    
    // Astronomy
    std::vector<std::string> astronomy_facts = {
        "sun is star",
        "star produces light",
        "star produces heat",
        "heat warms planets",
        "earth is planet",
        "planet orbits star",
        "orbit is circular",
        "gravity causes orbit",
        "gravity pulls objects",
        "objects have mass"
    };
    
    // Chemistry
    std::vector<std::string> chemistry_facts = {
        "water contains hydrogen",
        "water contains oxygen",
        "hydrogen is element",
        "oxygen is element",
        "elements form molecules",
        "molecules form matter",
        "matter has mass",
        "atoms form elements",
        "atoms are tiny",
        "tiny means small"
    };
    
    // Causality chains
    std::vector<std::string> causality_facts = {
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
    
    // Combine all facts
    std::vector<std::vector<std::string>> all_categories = {
        physics_facts,
        water_facts,
        biology_facts,
        animal_facts,
        astronomy_facts,
        chemistry_facts,
        causality_facts
    };
    
    std::vector<std::string> category_names = {
        "Physics",
        "Water Cycle",
        "Biology",
        "Animals",
        "Astronomy",
        "Chemistry",
        "Causality"
    };
    
    int total_taught = 0;
    
    for (size_t i = 0; i < all_categories.size(); i++) {
        std::cout << "\n📚 Teaching " << category_names[i] << " ("
                  << all_categories[i].size() << " facts)...\n";
        
        for (const auto& fact : all_categories[i]) {
            if (learning->teach_fact(fact, category_names[i])) {
                total_taught++;
                std::cout << "  ✓ " << fact << "\n";
            }
        }
    }
    
    std::cout << "\n✅ Successfully taught " << total_taught << " facts!\n\n";
    
    // Show stats before LEAP creation
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "BEFORE LEAP Inference:\n";
    std::cout << "═══════════════════════════════════════════════\n";
    storage->print_stats();
    
    // Create LEAP connections
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  CREATING LEAP CONNECTIONS                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 4;  // Allow longer chains
    leap_config.min_shared_neighbors = 2;
    leap_config.leap_confidence = 0.7f;
    leap_config.verbose = true;
    
    leap::LEAPInference leap_system(leap_config);
    int leaps_created = leap_system.create_leap_connections(storage.get());
    
    std::cout << "\n✅ Created " << leaps_created << " LEAP connections!\n\n";
    
    // Show final stats
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "AFTER LEAP Inference:\n";
    std::cout << "═══════════════════════════════════════════════\n";
    storage->print_stats();
    
    // Save
    std::cout << "\n💾 Saving knowledge base...\n";
    if (storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cout << "✅ Saved to melvin/data/\n";
    } else {
        std::cerr << "❌ Failed to save\n";
        return 1;
    }
    
    // Show some example reasoning chains
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  EXAMPLE REASONING CHAINS                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    
    // Find some interesting multi-hop chains
    auto nodes = storage->get_all_nodes();
    
    if (nodes.size() >= 2) {
        std::cout << "Sample paths in knowledge graph:\n\n";
        
        // Try to find paths between interesting concepts
        struct PathTest {
            std::string from;
            std::string to;
        };
        
        std::vector<PathTest> interesting_paths = {
            {"fire", "comfort"},
            {"water", "ocean"},
            {"sun", "life"},
            {"exercise", "wisdom"},
            {"plants", "animals"}
        };
        
        for (const auto& test : interesting_paths) {
            auto from_nodes = storage->find_nodes(test.from);
            auto to_nodes = storage->find_nodes(test.to);
            
            if (!from_nodes.empty() && !to_nodes.empty()) {
                auto paths = storage->find_paths(from_nodes[0].id, to_nodes[0].id, 10);
                
                if (!paths.empty()) {
                    std::cout << "  " << test.from << " → " << test.to << ": "
                              << paths[0].hop_count() << " hops\n";
                    
                    // Show the path
                    std::cout << "    Path: ";
                    for (size_t i = 0; i < paths[0].nodes.size(); i++) {
                        std::cout << storage->get_node_content(paths[0].nodes[i]);
                        if (i < paths[0].nodes.size() - 1) std::cout << " → ";
                    }
                    std::cout << "\n";
                }
            }
        }
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  KNOWLEDGE BASE READY FOR AUTONOMOUS LEARNING ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    
    return 0;
}

