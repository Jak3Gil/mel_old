/*
 * Text Output Generator
 */

#include "../core/types.h"
#include <string>
#include <iostream>

namespace melvin {
namespace io {

/**
 * Format answer as text output
 */
std::string generate_text_output(const Answer& answer) {
    // Simple formatting for now
    return answer.text;
}

/**
 * Print answer to console with formatting
 */
void print_answer(const Answer& answer, bool verbose = false) {
    std::cout << "\n" << answer.text << "\n";
    
    if (verbose) {
        std::cout << "(confidence: " << answer.confidence << ")\n";
        
        if (!answer.paths.empty()) {
            std::cout << "Reasoning path:\n";
            const auto& path = answer.paths[0];
            std::cout << "  " << path.nodes.size() << " nodes, "
                     << path.edges.size() << " edges\n";
        }
    }
}

} // namespace io
} // namespace melvin

