/*
 * teach_curriculum - Melvin Curriculum Teaching System
 * 
 * Teaches Melvin from multiple .tch files in a directory, in order.
 * Stops on failure if --stop-on-fail is set.
 * 
 * Usage:
 *   ./teach_curriculum --dir lessons/
 *   ./teach_curriculum --dir lessons/ --stop-on-fail
 */

#include "../teaching/teaching_format.h"
#include "../teaching/teaching_ingest.h"
#include "../teaching/teaching_verify.h"
#include "../teaching/teaching_metrics.h"
#include "../../storage.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
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
    std::cout << "Usage: teach_curriculum [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --dir PATH         Directory containing .tch files\n";
    std::cout << "  --stop-on-fail     Stop teaching if a file fails (default: on)\n";
    std::cout << "  --continue         Continue even if files fail\n";
    std::cout << "  --shuffle          Randomize order (default: off, lexical order)\n";
    std::cout << "  --verbose          Verbose output (default: on)\n";
    std::cout << "  --quiet            Minimal output\n";
    std::cout << "  --help             Show this help\n";
    std::cout << "\nExamples:\n";
    std::cout << "  ./teach_curriculum --dir lessons/\n";
    std::cout << "  ./teach_curriculum --dir lessons/ --continue\n";
}

int main(int argc, char* argv[]) {
    std::string directory;
    bool stop_on_fail = true;
    bool shuffle = false;
    bool verbose = true;
    bool do_snapshot = true;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) {
            directory = argv[++i];
        } else if (strcmp(argv[i], "--stop-on-fail") == 0) {
            stop_on_fail = true;
        } else if (strcmp(argv[i], "--continue") == 0) {
            stop_on_fail = false;
        } else if (strcmp(argv[i], "--shuffle") == 0) {
            shuffle = true;
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
    
    if (directory.empty()) {
        std::cerr << "Error: --dir is required\n\n";
        print_usage();
        return 1;
    }
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           MELVIN CURRICULUM TEACHING SYSTEM                   ║\n";
    std::cout << "║           Multi-File Knowledge Ingestion                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Load existing brain state
    std::cout << "📂 Loading brain state...\n";
    bool loaded = melvin::load_brain_snapshot("melvin_brain.bin", G_nodes, G_edges);
    if (loaded) {
        std::cout << "✅ Loaded: " << G_nodes.size() << " nodes, " << G_edges.size() << " edges\n\n";
    } else {
        std::cout << "ℹ️  No existing snapshot. Starting fresh.\n\n";
    }
    
    // Find all .tch files in directory
    std::vector<std::string> tch_files;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".tch") {
                    tch_files.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error reading directory: " << e.what() << "\n";
        return 1;
    }
    
    if (tch_files.empty()) {
        std::cerr << "❌ No .tch files found in: " << directory << "\n";
        return 1;
    }
    
    // Sort lexically (unless shuffle)
    if (!shuffle) {
        std::sort(tch_files.begin(), tch_files.end());
    }
    
    std::cout << "📚 Found " << tch_files.size() << " teaching files\n";
    if (verbose) {
        for (const auto& file : tch_files) {
            std::cout << "  • " << file << "\n";
        }
    }
    std::cout << "\n";
    
    // Process each file
    int files_processed = 0;
    int files_passed = 0;
    int files_failed = 0;
    
    for (const auto& file : tch_files) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "Teaching: " << file << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        auto file_start = std::chrono::steady_clock::now();
        
        uint32_t nodes_before = G_nodes.size();
        uint32_t edges_before = G_edges.size();
        
        // Parse
        melvin::teaching::ParseOptions parse_opts;
        parse_opts.verbose = verbose;
        auto parse_result = melvin::teaching::TeachingParser::parse_file(file, parse_opts);
        
        if (!parse_result.success) {
            std::cerr << "❌ Parse failed: " << file << "\n";
            files_failed++;
            if (stop_on_fail) {
                std::cerr << "Stopping curriculum (--stop-on-fail)\n";
                break;
            }
            continue;
        }
        
        // Ingest
        melvin::teaching::IngestOptions ingest_opts;
        ingest_opts.snapshot_after = false;  // We'll snapshot at the end
        ingest_opts.verbose = verbose;
        
        auto ingest_result = melvin::teaching::TeachingIngestor::ingest(
            parse_result.doc, G_nodes, G_edges, ingest_opts
        );
        
        // Verify
        melvin::teaching::VerifyOptions verify_opts;
        verify_opts.verbose = verbose;
        
        auto verify_result = melvin::teaching::TeachingVerifier::verify(
            parse_result.doc, G_nodes, G_edges, verify_opts
        );
        
        auto file_end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(file_end - file_start).count();
        
        // Create session
        auto session = melvin::teaching::TeachingMetrics::create_session(
            file,
            parse_result.doc,
            nodes_before,
            G_nodes.size(),
            edges_before,
            G_edges.size(),
            ingest_result,
            verify_result,
            duration_ms
        );
        
        // Log to JSONL
        melvin::teaching::TeachingMetrics::log_to_jsonl(session);
        
        files_processed++;
        if (verify_result.success) {
            files_passed++;
            std::cout << "✅ " << file << " - PASSED\n\n";
        } else {
            files_failed++;
            std::cout << "❌ " << file << " - FAILED (" << verify_result.pass_rate * 100.0f << "% pass rate)\n\n";
            if (stop_on_fail) {
                std::cerr << "Stopping curriculum (--stop-on-fail)\n";
                break;
            }
        }
    }
    
    // Final snapshot
    if (do_snapshot) {
        std::cout << "💾 Saving final snapshot...\n";
        if (melvin::save_brain_snapshot("melvin_brain.bin", G_nodes, G_edges)) {
            std::cout << "✅ Snapshot saved: " << G_nodes.size() << " nodes, " << G_edges.size() << " edges\n\n";
        }
    }
    
    // Summary
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                CURRICULUM COMPLETE                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "📊 Curriculum Statistics:\n";
    std::cout << "  Files processed: " << files_processed << " / " << tch_files.size() << "\n";
    std::cout << "  Passed: " << files_passed << "\n";
    std::cout << "  Failed: " << files_failed << "\n";
    std::cout << "  Success rate: " << std::fixed << std::setprecision(1) 
              << (files_processed > 0 ? (static_cast<float>(files_passed) / files_processed * 100.0f) : 0.0f) << "%\n\n";
    
    if (files_passed == files_processed && files_processed > 0) {
        std::cout << "🎉 All lessons learned successfully!\n\n";
        return 0;
    } else if (files_passed > 0) {
        std::cout << "⚠️  Some lessons failed - review verification results\n\n";
        return 1;
    } else {
        std::cout << "❌ Curriculum failed - no lessons passed\n\n";
        return 1;
    }
}

