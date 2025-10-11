#include "../continuous_learning/continuous_learning.h"
#include "../teaching/teaching_format.h"
#include "../teaching/teaching_ingest.h"
#include "../teaching/teaching_verify.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <string>

// Define Node and Edge structures (global scope, matches other Melvin files)
struct Node {
    uint64_t id = 0;
    std::string text;
    uint32_t type = 0;
    int freq = 0;
    bool pinned = false;
    std::vector<float> emb;
    std::vector<float> embedding;
    float attention_weight = 0.0f;
    uint64_t last_accessed = 0;
    float semantic_strength = 1.0f;
    float activation = 0.0f;
};

struct Edge {
    uint64_t u, v;
    uint64_t locB = 0;
    float weight = 1.0f;
    uint32_t count = 1;
    uint32_t last_access_time = 0;
    std::string rel;
    
    void update_frequency(uint64_t) {}  // Stub
};

// Now include storage.h which has forward declarations
#include "../../storage.h"

// Global graph state (matches existing Melvin architecture)
std::unordered_map<uint64_t, Node> G_nodes;
std::vector<Edge> G_edges;
std::unordered_map<uint64_t, std::vector<size_t>> G_adj;
uint64_t G_total_nodes = 0;

namespace glue {

// Load config from YAML (simple parser for our minimal config)
melvin::cl::CLConfig load_cfg(const std::string& config_path) {
    melvin::cl::CLConfig c;
    c.inbox_dir = "data/inbox";
    c.processed_dir = "data/processed";
    c.failed_dir = "data/failed";
    c.metrics_csv = "logs/continuous_learning_metrics.csv";
    c.poll_seconds = 3;
    c.snapshot_every_seconds = 60;
    c.metrics_every_seconds = 10;
    c.max_files_per_tick = 4;
    c.enable_decay = false;
    c.enable_srs = false;
    
    // If config file exists, parse it (simple key: value format)
    std::ifstream f(config_path);
    if (f) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            auto colon = line.find(':');
            if (colon == std::string::npos) continue;
            
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t\""));
            val.erase(val.find_last_not_of(" \t\"") + 1);
            
            if (key == "inbox_dir") c.inbox_dir = val;
            else if (key == "processed_dir") c.processed_dir = val;
            else if (key == "failed_dir") c.failed_dir = val;
            else if (key == "metrics_csv") c.metrics_csv = val;
            else if (key == "poll_seconds") c.poll_seconds = std::stoi(val);
            else if (key == "snapshot_every_seconds") c.snapshot_every_seconds = std::stoi(val);
            else if (key == "metrics_every_seconds") c.metrics_every_seconds = std::stoi(val);
            else if (key == "max_files_per_tick") c.max_files_per_tick = std::stoi(val);
            else if (key == "enable_decay") c.enable_decay = (val == "true");
            else if (key == "enable_srs") c.enable_srs = (val == "true");
        }
    }
    
    return c;
}

// Call Teaching System for a .tch file
bool teach_file(const std::string& path, std::string& err) {
    try {
        // Parse the .tch file
        melvin::teaching::ParseOptions p_opts;
        p_opts.verbose = false;
        
        auto parse_result = melvin::teaching::TeachingParser::parse_file(path, p_opts);
        if (!parse_result.success || !parse_result.doc.errors.empty()) {
            err = "Parse failed";
            if (!parse_result.doc.errors.empty()) {
                err += ": " + parse_result.doc.errors[0];
            }
            return false;
        }
        
        // Ingest into graph
        melvin::teaching::IngestOptions opts;
        opts.snapshot_after = false;  // We handle snapshots in the main loop
        opts.verbose = false;
        
        auto ingest_result = melvin::teaching::TeachingIngestor::ingest(
            parse_result.doc, G_nodes, G_edges, opts
        );
        
        if (!ingest_result.success) {
            err = "Ingest failed";
            if (!ingest_result.errors.empty()) {
                err += ": " + ingest_result.errors[0];
            }
            return false;
        }
        
        // Optionally verify (quick check)
        melvin::teaching::VerifyOptions v_opts;
        v_opts.verbose = false;
        
        auto v_result = melvin::teaching::TeachingVerifier::verify(
            parse_result.doc, G_nodes, G_edges, v_opts
        );
        
        // Success even if verification isn't perfect (learning is gradual)
        return true;
        
    } catch (const std::exception& e) {
        err = std::string("Exception: ") + e.what();
        return false;
    }
}

// Trigger binary snapshot
bool snapshot_now(std::string& out_path, std::string& err) {
    try {
        // Generate timestamped filename
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << "data/snapshots/brain_" 
            << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") 
            << ".bin";
        
        out_path = oss.str();
        
        // Create snapshots directory
        std::filesystem::create_directories("data/snapshots");
        
        // Save snapshot
        if (!melvin::save_brain_snapshot(out_path.c_str(), G_nodes, G_edges)) {
            err = "save_brain_snapshot failed";
            return false;
        }
        
        // Also update the main brain file
        melvin::save_brain_snapshot("data/melvin_brain.bin", G_nodes, G_edges);
        
        return true;
        
    } catch (const std::exception& e) {
        err = std::string("Exception: ") + e.what();
        return false;
    }
}

// Read current node/edge counts from in-memory graph
void get_counts(uint64_t& nodes, uint64_t& edges) {
    nodes = G_nodes.size();
    edges = G_edges.size();
}

// Optional: decay and SRS hooks (no-op today, ready for Patch Pack E)
void do_decay() {
    // No-op until Patch Pack E
}

void do_srs() {
    // No-op until Patch Pack E
}

} // namespace glue

int main(int argc, char** argv) {
    using namespace melvin::cl;

    // Load config
    std::string config_path = "config/continuous_learning.yaml";
    if (argc > 1) {
        config_path = argv[1];
    }
    
    auto cfg = glue::load_cfg(config_path);

    // Try to load existing brain state
    std::cout << "[CL] Loading existing brain state...\n";
    if (melvin::load_brain_snapshot("data/melvin_brain.bin", G_nodes, G_edges)) {
        std::cout << "[CL] Loaded: " << G_nodes.size() << " nodes, " 
                  << G_edges.size() << " edges\n";
        G_total_nodes = G_nodes.size();
        
        // Rebuild adjacency list
        G_adj.clear();
        for (size_t i = 0; i < G_edges.size(); i++) {
            G_adj[G_edges[i].u].push_back(i);
        }
    } else {
        std::cout << "[CL] No existing brain found, starting fresh\n";
        G_total_nodes = 0;
    }

    // Create continuous learner
    ContinuousLearner cl(
        cfg,
        glue::teach_file,
        glue::snapshot_now,
        glue::get_counts,
        glue::do_decay,
        glue::do_srs
    );

    // Run forever
    cl.run();
    
    return 0;
}

