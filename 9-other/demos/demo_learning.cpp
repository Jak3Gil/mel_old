/*
 * Learning Demo - Test teaching and learning
 */

#include "../core/melvin.h"
#include "../UCAConfig.h"
#include <iostream>
#include <vector>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN LEARNING DEMO                                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Create Melvin instance with fresh memory
    MelvinCore melvin;
    
    std::cout << "Teaching facts...\n\n";
    
    // Teach some basic facts
    std::vector<std::pair<std::string, std::string>> facts = {
        {"fire produces heat", "physics"},
        {"heat causes warmth", "physics"},
        {"warmth feels good", "sensation"},
        {"water is wet", "physics"},
        {"wet is opposite of dry", "properties"}
    };
    
    for (const auto& [fact, context] : facts) {
        std::cout << "Teaching: " << fact << " (" << context << ")\n";
        melvin.teach_fact(fact, context);
    }
    
    std::cout << "\n";
    melvin.print_stats();
    
    // Save memory
    std::cout << "\nSaving learned facts...\n";
    melvin.save_memory();
    
    std::cout << "✅ Learning demo complete!\n\n";
    
    return 0;
}

