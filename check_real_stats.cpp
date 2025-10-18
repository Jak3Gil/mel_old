/*
 * Quick stats checker - shows REAL node/edge counts
 */

#include "melvin/core/storage.h"
#include <iostream>

using namespace melvin;

int main() {
    auto storage = std::make_unique<Storage>();
    
    if (!storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cerr << "Failed to load\n";
        return 1;
    }
    
    size_t nodes = storage->node_count();
    size_t edges = storage->edge_count();
    size_t exact = storage->edge_count_by_type(RelationType::EXACT);
    size_t leap = storage->edge_count_by_type(RelationType::LEAP);
    
    // Output in simple format for easy parsing
    std::cout << nodes << " " << edges << " " << exact << " " << leap << "\n";
    
    return 0;
}

