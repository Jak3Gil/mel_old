/*
 * Quick script to populate Melvin's knowledge base with test facts
 */

#include "melvin/core/storage.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include <iostream>
#include <vector>

using namespace melvin;

int main() {
    std::cout << "Populating Melvin's knowledge base...\n\n";
    
    auto storage = std::make_unique<Storage>();
    auto learning = std::make_unique<LearningSystem>(storage.get());
    
    // Teach basic facts
    std::vector<std::string> facts = {
        "fire produces heat",
        "fire produces light",
        "heat makes warm",
        "heat causes expansion",
        "water flows downhill",
        "water is wet",
        "water contains hydrogen",
        "water contains oxygen",
        "sun is hot",
        "sun produces light",
        "sun is a star",
        "stars produce energy",
        "plants need water",
        "plants need sunlight",
        "plants produce oxygen",
        "humans need oxygen",
        "humans need water",
        "dogs are mammals",
        "cats are mammals",
        "mammals have fur"
    };
    
    std::cout << "Teaching " << facts.size() << " facts...\n";
    int count = 0;
    for (const auto& fact : facts) {
        if (learning->teach_fact(fact, "initial_knowledge")) {
            count++;
            std::cout << "  ✓ " << fact << "\n";
        }
    }
    
    std::cout << "\n✅ Taught " << count << " facts successfully!\n\n";
    
    // Show stats before LEAP creation
    std::cout << "Before LEAP inference:\n";
    storage->print_stats();
    
    // Create LEAP connections
    std::cout << "\nCreating LEAP connections...\n";
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 3;
    leap_config.min_shared_neighbors = 2;
    leap_config.leap_confidence = 0.7f;
    leap_config.verbose = true;
    
    leap::LEAPInference leap_system(leap_config);
    int leaps_created = leap_system.create_leap_connections(storage.get());
    
    std::cout << "\n✅ Created " << leaps_created << " LEAP connections!\n\n";
    
    // Show final stats
    std::cout << "After LEAP inference:\n";
    storage->print_stats();
    
    // Save
    std::cout << "\nSaving knowledge base...\n";
    if (storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cout << "✅ Saved to melvin/data/\n";
    } else {
        std::cerr << "❌ Failed to save\n";
    }
    
    return 0;
}

