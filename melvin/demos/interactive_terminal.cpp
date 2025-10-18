/*
 * Interactive Terminal - Simple REPL for Melvin
 */

#include "../core/melvin.h"
#include "../UCAConfig.h"
#include <iostream>
#include <string>

using namespace melvin;

void print_banner() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN INTERACTIVE TERMINAL                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Commands:\n";
    std::cout << "  /help    - Show this message\n";
    std::cout << "  /stats   - Show statistics\n";
    std::cout << "  /save    - Save memory\n";
    std::cout << "  /quit    - Exit\n";
    std::cout << "\n";
}

int main() {
    print_banner();
    
    MelvinCore melvin;
    melvin.load_memory(config::STORAGE_PATH_NODES, config::STORAGE_PATH_EDGES);
    
    std::string line;
    while (true) {
        std::cout << "melvin> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        if (line == "/quit" || line == "/exit") {
            std::cout << "Saving and exiting...\n";
            melvin.save_memory();
            break;
        } else if (line == "/help") {
            print_banner();
            continue;
        } else if (line == "/stats") {
            melvin.print_stats();
            continue;
        } else if (line == "/save") {
            melvin.save_memory();
            std::cout << "✅ Memory saved\n";
            continue;
        }
        
        // Process query
        auto answer = melvin.query(line);
        std::cout << "\n" << answer.text << "\n\n";
    }
    
    return 0;
}

