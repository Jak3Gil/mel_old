/*
 * MELVIN GRAPH DUMP TOOL
 * 
 * Displays nodes, EXACT edges, and LEAP edges with full metadata
 * Supports both text and JSON output formats
 */

#include "../../include/melvin_storage.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstring>

using namespace melvin_storage;

enum class OutputFormat {
    TEXT,
    JSON
};

struct Options {
    OutputFormat format = OutputFormat::TEXT;
    bool show_nodes = true;
    bool show_exact = true;
    bool show_leaps = true;
    int top_n = 0;  // 0 = show all
    bool sort_by_time = true;
    std::string brain_file = "melvin_unified_brain.bin";
};

// Parse command line arguments
Options parse_args(int argc, char* argv[]) {
    Options opts;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--format" && i + 1 < argc) {
            std::string fmt = argv[++i];
            if (fmt == "json") opts.format = OutputFormat::JSON;
            else if (fmt == "text") opts.format = OutputFormat::TEXT;
        }
        else if (arg == "--top" && i + 1 < argc) {
            opts.top_n = std::stoi(argv[++i]);
        }
        else if (arg == "--nodes-only") {
            opts.show_exact = false;
            opts.show_leaps = false;
        }
        else if (arg == "--exact-only") {
            opts.show_nodes = false;
            opts.show_leaps = false;
        }
        else if (arg == "--leaps-only" || arg == "--leaps") {
            opts.show_nodes = false;
            opts.show_exact = false;
        }
        else if (arg == "--sort-time") {
            opts.sort_by_time = true;
        }
        else if (arg == "--sort-weight") {
            opts.sort_by_time = false;
        }
        else if (arg == "--brain" && i + 1 < argc) {
            opts.brain_file = argv[++i];
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: dump_graph [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --format text|json     Output format (default: text)\n";
            std::cout << "  --top N                Show only top N entries\n";
            std::cout << "  --nodes-only           Show only nodes\n";
            std::cout << "  --exact-only           Show only EXACT edges\n";
            std::cout << "  --leaps-only, --leaps  Show only LEAP edges\n";
            std::cout << "  --sort-time            Sort by timestamp (default)\n";
            std::cout << "  --sort-weight          Sort by weight/score\n";
            std::cout << "  --brain FILE           Brain file to load (default: melvin_unified_brain.bin)\n";
            std::cout << "  --help, -h             Show this help\n\n";
            std::cout << "Examples:\n";
            std::cout << "  ./dump_graph --leaps --top 50\n";
            std::cout << "  ./dump_graph --format json > graph.json\n";
            std::cout << "  ./dump_graph --exact-only --sort-weight\n\n";
            exit(0);
        }
    }
    
    return opts;
}

// Convert timestamp to readable string
std::string format_timestamp(uint64_t ts_ns) {
    if (ts_ns == 0) return "never";
    
    // Simple formatting - just show the raw nanoseconds for now
    std::stringstream ss;
    ss << ts_ns;
    return ss.str();
}

// Escape string for JSON
std::string json_escape(const std::string& str) {
    std::stringstream ss;
    for (char c : str) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << c; break;
        }
    }
    return ss.str();
}

// Dump nodes in TEXT format
void dump_nodes_text(const Options& opts) {
    std::cout << "\n📦 NODES (" << nodes.size() << " total)\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    int count = 0;
    for (size_t i = 0; i < nodes.size() && (opts.top_n == 0 || count < opts.top_n); ++i) {
        const auto& node = nodes[i];
        std::cout << "[" << i << "] \"" << node.data << "\""
                  << " type=" << (int)node.type
                  << " sensory=" << (int)node.sensory_type
                  << " last_used=" << format_timestamp(node.last_used)
                  << "\n";
        count++;
    }
}

// Dump EXACT edges in TEXT format
void dump_exact_text(const Options& opts) {
    std::vector<std::pair<int, const Edge*>> exact_edges;
    
    for (size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].is_exact()) {
            exact_edges.push_back({(int)i, &edges[i]});
        }
    }
    
    // Sort
    if (opts.sort_by_time) {
        std::sort(exact_edges.begin(), exact_edges.end(),
                 [](const auto& a, const auto& b) { return a.second->last_used > b.second->last_used; });
    } else {
        std::sort(exact_edges.begin(), exact_edges.end(),
                 [](const auto& a, const auto& b) { return a.second->w > b.second->w; });
    }
    
    std::cout << "\n✅ EXACT EDGES (" << exact_edges.size() << " total)\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    int count = 0;
    for (const auto& [idx, edge] : exact_edges) {
        if (opts.top_n > 0 && count >= opts.top_n) break;
        
        std::cout << "  " << nodes[edge->a].data
                  << " =[" << rel_to_str(edge->rel) << "]=> "
                  << nodes[edge->b].data
                  << " w=" << std::fixed << std::setprecision(3) << edge->w
                  << " count=" << edge->count
                  << " used=" << format_timestamp(edge->last_used)
                  << "\n";
        count++;
    }
}

// Dump LEAP edges in TEXT format
void dump_leaps_text(const Options& opts) {
    std::vector<std::pair<int, const Edge*>> leap_edges;
    
    for (size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].is_leap()) {
            leap_edges.push_back({(int)i, &edges[i]});
        }
    }
    
    // Sort
    if (opts.sort_by_time) {
        std::sort(leap_edges.begin(), leap_edges.end(),
                 [](const auto& a, const auto& b) { return a.second->created_at > b.second->created_at; });
    } else {
        std::sort(leap_edges.begin(), leap_edges.end(),
                 [](const auto& a, const auto& b) { return a.second->leap_score > b.second->leap_score; });
    }
    
    std::cout << "\n🔮 LEAP EDGES (" << leap_edges.size() << " total)\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    int count = 0;
    for (const auto& [idx, edge] : leap_edges) {
        if (opts.top_n > 0 && count >= opts.top_n) break;
        
        std::cout << "  " << nodes[edge->a].data
                  << " ~LEAP~> "
                  << nodes[edge->b].data
                  << " support=" << std::fixed << std::setprecision(2) << edge->leap_score
                  << " w=" << std::fixed << std::setprecision(3) << edge->w
                  << " successes=" << edge->successes
                  << " created=" << format_timestamp(edge->created_at)
                  << "\n";
        count++;
    }
}

// Dump entire graph in JSON format
void dump_json(const Options& opts) {
    std::cout << "{\n";
    
    // Nodes
    if (opts.show_nodes) {
        std::cout << "  \"nodes\": [\n";
        int count = 0;
        for (size_t i = 0; i < nodes.size() && (opts.top_n == 0 || count < opts.top_n); ++i) {
            if (count > 0) std::cout << ",\n";
            const auto& node = nodes[i];
            std::cout << "    {\"id\": " << i
                     << ", \"data\": \"" << json_escape(node.data) << "\""
                     << ", \"type\": " << (int)node.type
                     << ", \"sensory_type\": " << (int)node.sensory_type
                     << ", \"last_used\": " << node.last_used
                     << "}";
            count++;
        }
        std::cout << "\n  ]";
        if (opts.show_exact || opts.show_leaps) std::cout << ",";
        std::cout << "\n";
    }
    
    // EXACT edges
    if (opts.show_exact) {
        std::vector<std::pair<int, const Edge*>> exact_edges;
        for (size_t i = 0; i < edges.size(); ++i) {
            if (edges[i].is_exact()) exact_edges.push_back({(int)i, &edges[i]});
        }
        
        std::cout << "  \"exact_edges\": [\n";
        int count = 0;
        for (const auto& [idx, edge] : exact_edges) {
            if (opts.top_n > 0 && count >= opts.top_n) break;
            if (count > 0) std::cout << ",\n";
            
            std::cout << "    {\"from\": " << edge->a
                     << ", \"to\": " << edge->b
                     << ", \"from_text\": \"" << json_escape(nodes[edge->a].data) << "\""
                     << ", \"to_text\": \"" << json_escape(nodes[edge->b].data) << "\""
                     << ", \"rel\": \"" << rel_to_str(edge->rel) << "\""
                     << ", \"weight\": " << edge->w
                     << ", \"count\": " << edge->count
                     << ", \"last_used\": " << edge->last_used
                     << "}";
            count++;
        }
        std::cout << "\n  ]";
        if (opts.show_leaps) std::cout << ",";
        std::cout << "\n";
    }
    
    // LEAP edges
    if (opts.show_leaps) {
        std::vector<std::pair<int, const Edge*>> leap_edges;
        for (size_t i = 0; i < edges.size(); ++i) {
            if (edges[i].is_leap()) leap_edges.push_back({(int)i, &edges[i]});
        }
        
        std::cout << "  \"leap_edges\": [\n";
        int count = 0;
        for (const auto& [idx, edge] : leap_edges) {
            if (opts.top_n > 0 && count >= opts.top_n) break;
            if (count > 0) std::cout << ",\n";
            
            std::cout << "    {\"from\": " << edge->a
                     << ", \"to\": " << edge->b
                     << ", \"from_text\": \"" << json_escape(nodes[edge->a].data) << "\""
                     << ", \"to_text\": \"" << json_escape(nodes[edge->b].data) << "\""
                     << ", \"support\": " << edge->leap_score
                     << ", \"weight\": " << edge->w
                     << ", \"successes\": " << edge->successes
                     << ", \"created_at\": " << edge->created_at
                     << "}";
            count++;
        }
        std::cout << "\n  ]\n";
    }
    
    std::cout << "}\n";
}

int main(int argc, char* argv[]) {
    Options opts = parse_args(argc, argv);
    
    // Load brain
    if (!load_brain(opts.brain_file)) {
        std::cerr << "❌ Failed to load brain from: " << opts.brain_file << "\n";
        std::cerr << "   Starting with empty brain...\n\n";
    }
    
    if (opts.format == OutputFormat::TEXT) {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  MELVIN GRAPH DUMP                                           ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
        
        if (opts.show_nodes) dump_nodes_text(opts);
        if (opts.show_exact) dump_exact_text(opts);
        if (opts.show_leaps) dump_leaps_text(opts);
        
        std::cout << "\n";
    } else {
        dump_json(opts);
    }
    
    return 0;
}

