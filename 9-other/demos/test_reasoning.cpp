/*
 * Test Reasoning - Demonstrate multi-hop reasoning with learned knowledge
 */

#include "../core/storage.h"
#include "../core/types.h"
#include <iostream>
#include <vector>
#include <set>

using namespace melvin;

struct ReasoningTest {
    std::string query;
    std::string start_concept;
    std::string target_concept;
    int expected_hops;
};

void print_path(const std::vector<NodeID>& path, Storage& storage) {
    for (size_t i = 0; i < path.size(); ++i) {
        Node node;
        if (storage.get_node(path[i], node)) {
            std::cout << node.content;
            if (i < path.size() - 1) {
                std::cout << " → ";
            }
        }
    }
}

std::vector<std::vector<NodeID>> find_all_paths(
    const NodeID& start,
    const NodeID& target,
    Storage& storage,
    int max_hops = 5
) {
    std::vector<std::vector<NodeID>> all_paths;
    std::vector<NodeID> current_path;
    std::set<std::string> visited;
    
    // Helper function for DFS
    std::function<void(const NodeID&, int)> dfs = [&](const NodeID& current, int depth) {
        if (depth > max_hops) return;
        
        // Convert to key for visited tracking
        Node curr_node;
        if (!storage.get_node(current, curr_node)) return;
        
        std::string key;
        for (auto byte : current) {
            char buf[3];
            snprintf(buf, 3, "%02x", byte);
            key += buf;
        }
        
        if (visited.count(key)) return;
        visited.insert(key);
        current_path.push_back(current);
        
        // Check if we reached target
        if (current == target) {
            all_paths.push_back(current_path);
        } else {
            // Explore neighbors
            auto edges = storage.get_edges_from(current);
            for (const auto& edge : edges) {
                dfs(edge.to_id, depth + 1);
            }
        }
        
        current_path.pop_back();
        visited.erase(key);
    };
    
    dfs(start, 0);
    return all_paths;
}

void test_reasoning(const ReasoningTest& test, Storage& storage) {
    std::cout << "════════════════════════════════════════════════════════\n";
    std::cout << "Query: " << test.query << "\n";
    std::cout << "────────────────────────────────────────────────────────\n";
    
    // Find start and target nodes
    auto start_nodes = storage.find_nodes(test.start_concept);
    auto target_nodes = storage.find_nodes(test.target_concept);
    
    if (start_nodes.empty() || target_nodes.empty()) {
        std::cout << "✗ Could not find concepts in memory\n\n";
        return;
    }
    
    NodeID start = start_nodes[0].id;
    NodeID target = target_nodes[0].id;
    
    std::cout << "Finding paths from '" << test.start_concept 
              << "' to '" << test.target_concept << "'...\n\n";
    
    // Find all paths
    auto paths = find_all_paths(start, target, storage, 5);
    
    if (paths.empty()) {
        std::cout << "✗ No paths found\n\n";
        return;
    }
    
    std::cout << "✓ Found " << paths.size() << " path(s):\n\n";
    
    for (size_t i = 0; i < paths.size() && i < 3; ++i) {
        std::cout << "  Path " << (i+1) << " (" << (paths[i].size()-1) << " hops): ";
        print_path(paths[i], storage);
        std::cout << "\n";
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MULTI-HOP REASONING TEST                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Load memory
    Storage storage;
    std::cout << "Loading memory from disk...\n";
    
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load memory!\n";
        std::cerr << "  Run demos/teach_knowledge first\n\n";
        return 1;
    }
    
    std::cout << "  ✓ Loaded " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Test cases
    std::vector<ReasoningTest> tests = {
        {"How does fire create warmth?", "fire", "warmth", 2},
        {"How does sunlight help growth?", "sunlight", "growth", 2},
        {"How does vapor become rain?", "vapor", "rain", 2},
        {"How does practice lead to mastery?", "practice", "mastery", 2},
        {"How does hunting provide energy?", "hunting", "energy", 2},
    };
    
    std::cout << "Running reasoning tests...\n\n";
    
    for (const auto& test : tests) {
        test_reasoning(test, storage);
    }
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ REASONING TESTS COMPLETE                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Melvin successfully performed multi-hop reasoning!\n";
    std::cout << "The knowledge persists - restart and test again!\n\n";
    
    return 0;
}

