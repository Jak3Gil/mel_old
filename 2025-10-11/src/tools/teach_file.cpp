/*
 * teach_file - Melvin Teaching System CLI
 * 
 * Teaches Melvin from a .tch file, verifies retention, and logs metrics.
 * 
 * Usage:
 *   ./teach_file --file lessons/00_basics.tch
 *   ./teach_file --file lessons/01_animals.tch --verify --snapshot
 */

#include "../teaching/teaching_format.h"
#include "../teaching/teaching_ingest.h"
#include "../teaching/teaching_verify.h"
#include "../teaching/teaching_metrics.h"
#include "../../storage.h"
#include <iostream>
#include <chrono>
#include <cstring>

// Full Node and Edge definitions
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
    uint64_t locB;
    float weight;
    float w_core;
    float w_ctx;
    uint32_t count;
    
    void update_frequency(uint64_t total_nodes) { (void)total_nodes; }
};

// Global graph state
std::unordered_map<uint64_t, Node> G_nodes;
std::vector<Edge> G_edges;

void print_usage() {
    std::cout << "Usage: teach_file [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --file PATH       Teaching file to process (.tch)\n";
    std::cout << "  --verify          Run verification tests (default: on)\n";
    std::cout << "  --no-verify       Skip verification\n";
    std::cout << "  --snapshot        Save snapshot after teaching (default: on)\n";
    std::cout << "  --no-snapshot     Skip snapshot\n";
    std::cout << "  --verbose         Verbose output (default: on)\n";
    std::cout << "  --quiet           Minimal output\n";
    std::cout << "  --help            Show this help\n";
    std::cout << "\nExamples:\n";
    std::cout << "  ./teach_file --file lessons/00_basics.tch\n";
    std::cout << "  ./teach_file --file lessons/01_animals.tch --no-verify\n";
}

int main(int argc, char* argv[]) {
    std::string filepath;
    bool do_verify = true;
    bool do_snapshot = true;
    bool verbose = true;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            filepath = argv[++i];
        } else if (strcmp(argv[i], "--verify") == 0) {
            do_verify = true;
        } else if (strcmp(argv[i], "--no-verify") == 0) {
            do_verify = false;
        } else if (strcmp(argv[i], "--snapshot") == 0) {
            do_snapshot = true;
        } else if (strcmp(argv[i], "--no-snapshot") == 0) {
            do_snapshot = false;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--quiet") == 0) {
            verbose = false;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            print_usage();
            return 1;
        }
    }
    
    if (filepath.empty()) {
        std::cerr << "Error: --file is required\n\n";
        print_usage();
        return 1;
    }
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              MELVIN TEACHING SYSTEM                           ║\n";
    std::cout << "║              Teaching → Ingestion → Verification              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Load existing brain state
    std::cout << "📂 Loading brain state...\n";
    bool loaded = melvin::load_brain_snapshot("melvin_brain.bin", G_nodes, G_edges);
    if (loaded) {
        std::cout << "✅ Loaded: " << G_nodes.size() << " nodes, " << G_edges.size() << " edges\n\n";
    } else {
        std::cout << "ℹ️  No existing snapshot. Starting fresh.\n\n";
    }
    
    uint32_t nodes_before = G_nodes.size();
    uint32_t edges_before = G_edges.size();
    
    // Parse teaching file
    std::cout << "📖 Parsing: " << filepath << "...\n";
    melvin::teaching::ParseOptions parse_opts;
    parse_opts.verbose = verbose;
    
    auto parse_result = melvin::teaching::TeachingParser::parse_file(filepath, parse_opts);
    
    if (!parse_result.success) {
        std::cerr << "❌ Parse failed:\n";
        for (const auto& err : parse_result.doc.errors) {
            std::cerr << "  " << err << "\n";
        }
        return 1;
    }
    
    std::cout << "✅ Parsed: " << parse_result.blocks_parsed << " blocks\n";
    if (!parse_result.doc.warnings.empty()) {
        std::cout << "⚠️  Warnings: " << parse_result.doc.warnings.size() << "\n";
        if (verbose) {
            for (const auto& warn : parse_result.doc.warnings) {
                std::cout << "  " << warn << "\n";
            }
        }
    }
    std::cout << std::endl;
    
    // Ingest
    std::cout << "📥 Ingesting knowledge...\n";
    melvin::teaching::IngestOptions ingest_opts;
    ingest_opts.snapshot_after = do_snapshot;
    ingest_opts.verbose = verbose;
    
    auto ingest_result = melvin::teaching::TeachingIngestor::ingest(
        parse_result.doc, G_nodes, G_edges, ingest_opts
    );
    
    if (!ingest_result.success) {
        std::cerr << "❌ Ingestion failed\n";
        return 1;
    }
    
    // Verify if requested
    melvin::teaching::VerifyResult verify_result;
    if (do_verify) {
        melvin::teaching::VerifyOptions verify_opts;
        verify_opts.verbose = verbose;
        
        verify_result = melvin::teaching::TeachingVerifier::verify(
            parse_result.doc, G_nodes, G_edges, verify_opts
        );
    } else {
        std::cout << "ℹ️  Verification skipped\n\n";
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    // Create and print metrics
    auto session = melvin::teaching::TeachingMetrics::create_session(
        filepath,
        parse_result.doc,
        nodes_before,
        G_nodes.size(),
        edges_before,
        G_edges.size(),
        ingest_result,
        verify_result,
        duration_ms
    );
    
    melvin::teaching::TeachingMetrics::print_summary(session);
    
    // Log to JSONL
    if (melvin::teaching::TeachingMetrics::log_to_jsonl(session)) {
        if (verbose) {
            std::cout << "📝 Logged to logs/teaching/\n\n";
        }
    }
    
    // Return 0 if verification passed or was skipped
    if (!do_verify || verify_result.success) {
        std::cout << "🎉 Teaching session complete!\n\n";
        return 0;
    } else {
        std::cout << "⚠️  Teaching session complete with verification failures\n\n";
        return 1;
    }
}

