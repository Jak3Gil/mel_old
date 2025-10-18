/*
 * Analyze Multi-Hop Potential - Find all possible reasoning chains
 */

#include "../core/storage.h"
#include "../core/types.h"
#include <iostream>
#include <set>
#include <queue>
#include <map>
#include <algorithm>

using namespace melvin;

struct PathInfo {
    std::vector<NodeID> nodes;
    int hops;
};

// BFS to find all paths up to max_depth
std::vector<PathInfo> find_all_paths_bfs(Storage& storage, NodeID start, int max_depth) {
    std::vector<PathInfo> all_paths;
    std::queue<PathInfo> queue;
    
    // Start with single node
    PathInfo initial;
    initial.nodes.push_back(start);
    initial.hops = 0;
    queue.push(initial);
    
    while (!queue.empty()) {
        PathInfo current = queue.front();
        queue.pop();
        
        // Add this path to results
        if (current.hops > 0) {
            all_paths.push_back(current);
        }
        
        // If we can go deeper
        if (current.hops < max_depth) {
            NodeID last = current.nodes.back();
            auto edges = storage.get_edges_from(last);
            
            for (const auto& edge : edges) {
                // Avoid cycles (don't revisit nodes in current path)
                bool cycle = false;
                for (auto node_id : current.nodes) {
                    if (node_id == edge.to_id) {
                        cycle = true;
                        break;
                    }
                }
                
                if (!cycle) {
                    PathInfo new_path = current;
                    new_path.nodes.push_back(edge.to_id);
                    new_path.hops++;
                    queue.push(new_path);
                }
            }
        }
    }
    
    return all_paths;
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MULTI-HOP ANALYSIS                                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load. Run ./demos/teach_knowledge first!\n\n";
        return 1;
    }
    
    std::cout << "📊 Graph Statistics:\n";
    std::cout << "  Nodes: " << storage.node_count() << "\n";
    std::cout << "  Edges: " << storage.edge_count() << "\n\n";
    
    // Count paths by depth
    std::map<int, int> hop_counts;
    std::map<int, std::vector<PathInfo>> example_paths;
    
    int max_depth = 10;  // Search up to 10 hops
    
    std::cout << "Analyzing multi-hop potential...\n";
    std::cout << "(searching up to " << max_depth << " hops)\n\n";
    
    // Analyze from each node
    auto all_nodes = storage.get_all_nodes();
    
    int total_paths = 0;
    int longest_path = 0;
    
    for (const auto& node : all_nodes) {
        auto paths = find_all_paths_bfs(storage, node.id, max_depth);
        
        for (const auto& path : paths) {
            hop_counts[path.hops]++;
            total_paths++;
            
            if (path.hops > longest_path) {
                longest_path = path.hops;
            }
            
            // Keep some examples for each hop count
            if (example_paths[path.hops].size() < 3) {
                example_paths[path.hops].push_back(path);
            }
        }
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "📊 PATH DISTRIBUTION:\n\n";
    
    for (int h = 1; h <= longest_path; ++h) {
        std::cout << "  " << h << "-hop paths: " << hop_counts[h] << "\n";
    }
    
    std::cout << "\n  Total paths: " << total_paths << "\n";
    std::cout << "  Longest chain: " << longest_path << " hops\n\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "🔗 EXAMPLE CHAINS BY DEPTH:\n\n";
    
    for (int h = 1; h <= std::min(longest_path, 7); ++h) {
        std::cout << "  " << h << "-hop examples:\n";
        
        for (size_t i = 0; i < std::min(example_paths[h].size(), size_t(3)); ++i) {
            std::cout << "    ";
            const auto& path = example_paths[h][i];
            
            for (size_t j = 0; j < path.nodes.size(); ++j) {
                std::cout << storage.get_node_content(path.nodes[j]);
                if (j < path.nodes.size() - 1) {
                    std::cout << " → ";
                }
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Find some interesting long chains
    std::cout << "🌟 LONGEST CHAINS FOUND:\n\n";
    
    std::vector<PathInfo> all_long_paths;
    for (const auto& node : all_nodes) {
        auto paths = find_all_paths_bfs(storage, node.id, max_depth);
        for (const auto& path : paths) {
            if (path.hops >= longest_path - 1) {
                all_long_paths.push_back(path);
            }
        }
    }
    
    // Sort by length
    std::sort(all_long_paths.begin(), all_long_paths.end(),
        [](const PathInfo& a, const PathInfo& b) { return a.hops > b.hops; });
    
    for (size_t i = 0; i < std::min(all_long_paths.size(), size_t(5)); ++i) {
        const auto& path = all_long_paths[i];
        std::cout << "  " << path.hops << " hops: ";
        
        for (size_t j = 0; j < path.nodes.size(); ++j) {
            std::cout << storage.get_node_content(path.nodes[j]);
            if (j < path.nodes.size() - 1) {
                std::cout << " → ";
            }
        }
        std::cout << "\n";
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ ANALYSIS COMPLETE                                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Summary:\n";
    std::cout << "  • " << total_paths << " total reasoning paths\n";
    std::cout << "  • Up to " << longest_path << " hops deep\n";
    std::cout << "  • Rich connectivity for LEAP reasoning!\n\n";
    
    return 0;
}

