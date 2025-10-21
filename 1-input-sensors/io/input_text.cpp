/*
 * Text Input Adapter
 */

#include "../core/types.h"
#include <string>
#include <iostream>

namespace melvin {
namespace io {

/**
 * Process text input and prepare for reasoning
 */
Query process_text_input(const std::string& text) {
    Query query;
    query.text = text;
    query.metadata["modality"] = "text";
    
    // TODO: Add text preprocessing (normalization, tokenization)
    // TODO: Generate embeddings if available
    
    return query;
}

/**
 * Read text from stdin interactively
 */
std::string read_interactive() {
    std::string line;
    std::cout << "> ";
    std::getline(std::cin, line);
    return line;
}

} // namespace io
} // namespace melvin

