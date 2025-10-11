/*
 * MELVIN GRAPH-GUIDED PREDICTIVE SYSTEM DIAGNOSTIC & AUTO-TUNING
 * 
 * This program performs comprehensive diagnostics and auto-tuning of:
 *   - Graph bias influence on predictions
 *   - Node-token embedding alignment
 *   - Leap node semantic effectiveness
 * 
 * Usage:
 *   ./diagnostic_main [--auto-tune] [--full-report]
 * 
 * Options:
 *   --auto-tune     : Run full auto-tuning procedure
 *   --full-report   : Generate detailed markdown report
 *   --quick         : Run quick diagnostic (5 tests only)
 *   --help          : Show this help
 */

#include "melvin_leap_nodes.h"
#include "predictive_sampler.h"
#include "src/embeddings/embedding_bridge.h"
#include "leap_diagnostic.h"
#include "leap_auto_tune.h"
#include "src/util/config.h"
#include <iostream>
#include <string>
#include <cstring>

// Stub Node/Edge structures for standalone testing (global scope, like melvin.cpp)
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
    uint8_t rel;
    float last_used;
    float freq_ratio = 0.0f;
    
    Edge() : u(0), v(0), locB(0), weight(0.5f), w_core(0.3f), w_ctx(0.2f), count(1), rel(0), last_used(0.0f), freq_ratio(0.0f) {}
    
    // Add stub update_frequency method
    void update_frequency(uint64_t) {}
};

// Globals (in real usage, these would be initialized from melvin.cpp)
std::unordered_map<uint64_t, Node> G_nodes;
std::vector<Edge> G_edges;
std::unordered_map<uint64_t, std::vector<size_t>> G_adj;
uint64_t G_total_nodes = 0;

// Initialize test graph with sample data
void initialize_test_graph() {
    std::cout << "🔧 Initializing test graph...\n";
    
    // Create sample nodes for testing
    std::vector<std::string> concepts = {
        "fire", "water", "heat", "smoke", "steam", "cool",
        "music", "emotion", "song", "feeling", "melody",
        "robot", "person", "machine", "human", "artificial",
        "sun", "night", "day", "moon", "dark",
        "anger", "calm", "peace", "rage", "tranquil",
        "bird", "flight", "fly", "wing", "sky",
        "tree", "air", "oxygen", "leaf", "breathe",
        "food", "energy", "eat", "fuel", "nutrition",
        "thought", "memory", "remember", "think", "recall",
        "rain", "growth", "plant", "water", "develop"
    };
    
    // Create nodes
    uint64_t node_id = 1000;
    for (const auto& concept_text : concepts) {
        Node node;
        node.id = node_id;
        node.text = concept_text;
        node.type = 1; // Word type
        node.freq = 10;
        
        // Initialize with random embedding
        node.emb.resize(64);
        for (int i = 0; i < 64; ++i) {
            node.emb[i] = ((float)rand() / RAND_MAX) * 0.1f;
        }
        
        // Set some semantic similarity manually for testing
        // (In real system, embeddings would be learned)
        if (concept_text == "fire" || concept_text == "heat" || concept_text == "smoke") {
            // Fire-related concepts have similar embeddings
            for (int i = 0; i < 10; ++i) {
                node.emb[i] = 0.8f + ((float)rand() / RAND_MAX) * 0.1f;
            }
        } else if (concept_text == "water" || concept_text == "cool" || concept_text == "steam") {
            for (int i = 0; i < 10; ++i) {
                node.emb[i] = -0.8f + ((float)rand() / RAND_MAX) * 0.1f;
            }
        }
        
        node.activation = 0.1f;
        
        G_nodes[node_id] = node;
        node_id++;
    }
    
    G_total_nodes = node_id;
    
    // Create some edges
    for (auto& [id1, node1] : G_nodes) {
        for (auto& [id2, node2] : G_nodes) {
            if (id1 >= id2) continue;
            
            // Create edge if concepts are related
            bool related = false;
            
            // Simple relatedness rules
            if ((node1.text == "fire" && node2.text == "heat") ||
                (node1.text == "fire" && node2.text == "smoke") ||
                (node1.text == "water" && node2.text == "cool") ||
                (node1.text == "music" && node2.text == "emotion") ||
                (node1.text == "bird" && node2.text == "flight")) {
                related = true;
            }
            
            if (related) {
                Edge edge;
                edge.u = id1;
                edge.v = id2;
                edge.locB = id2;
                edge.weight = 0.7f;
                edge.freq_ratio = 0.5f;
                edge.rel = static_cast<uint8_t>(melvin::Rel::TEMPORAL);
                
                size_t edge_idx = G_edges.size();
                G_edges.push_back(edge);
                G_adj[id1].push_back(edge_idx);
            }
        }
    }
    
    std::cout << "✅ Test graph initialized: " << G_nodes.size() 
              << " nodes, " << G_edges.size() << " edges\n\n";
}

void print_usage() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║  MELVIN GRAPH-GUIDED PREDICTIVE SYSTEM DIAGNOSTIC TOOL        ║
╚═══════════════════════════════════════════════════════════════╝

USAGE:
  ./diagnostic_main [OPTIONS]

OPTIONS:
  --auto-tune      Run comprehensive auto-tuning procedure
  --full-report    Generate detailed markdown report
  --quick          Run quick diagnostic (5 tests only)
  --csv-only       Only generate CSV output
  --help           Show this help message

EXAMPLES:
  ./diagnostic_main                    # Basic diagnostics
  ./diagnostic_main --auto-tune        # Full auto-tuning
  ./diagnostic_main --full-report      # Detailed report
  ./diagnostic_main --quick --csv-only # Quick CSV export

OUTPUT FILES:
  - leap_diagnostics.csv      : Raw diagnostic data
  - leap_tuning_report.md     : Markdown report (with --full-report)
  - leap_tuning_results.txt   : Auto-tuning results (with --auto-tune)

)";
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool run_auto_tune = false;
    bool full_report = false;
    bool quick_mode = false;
    bool csv_only = false;
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--auto-tune") == 0) {
            run_auto_tune = true;
        } else if (strcmp(argv[i], "--full-report") == 0) {
            full_report = true;
        } else if (strcmp(argv[i], "--quick") == 0) {
            quick_mode = true;
        } else if (strcmp(argv[i], "--csv-only") == 0) {
            csv_only = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            std::cerr << "Use --help for usage information.\n";
            return 1;
        }
    }
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN GRAPH-GUIDED PREDICTIVE SYSTEM DIAGNOSTICS            ║\n";
    std::cout << "║  Measuring graph bias, embeddings, and leap effectiveness    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    // Initialize test graph
    initialize_test_graph();
    
    // Load configuration
    melvin_config::load_config();
    
    // Initialize components
    melvin::LeapConfig leap_config;
    melvin::LeapController leap_controller(leap_config);
    
    melvin::embeddings::EmbeddingBridgeConfig emb_config;
    emb_config.lambda_graph_bias = melvin_config::get_config().lambda_graph_bias;
    melvin::embeddings::EmbeddingBridge embedding_bridge(emb_config);
    
    melvin::PredictiveConfig pred_config;
    pred_config.lambda_graph_bias = melvin_config::get_config().lambda_graph_bias;
    pred_config.enable_leap_system = true;
    pred_config.enable_embedding_bridge = true;
    
    // ==================== RUN DIAGNOSTICS ====================
    
    std::vector<melvin::diagnostic::LeapMetrics> all_metrics;
    
    if (quick_mode) {
        std::cout << "⚡ Running quick diagnostic (5 tests)...\n\n";
        auto test_prompts = melvin::diagnostic::get_standard_test_prompts();
        for (int i = 0; i < 5; ++i) {
            auto metrics = melvin::diagnostic::run_single_diagnostic_test(
                test_prompts[i], leap_controller, embedding_bridge, pred_config);
            all_metrics.push_back(metrics);
        }
    } else {
        std::cout << "🔬 Running full diagnostic suite (10 tests)...\n\n";
        all_metrics = melvin::diagnostic::run_full_diagnostic_suite(
            leap_controller, embedding_bridge, pred_config);
    }
    
    // Compute summary
    auto summary = melvin::diagnostic::compute_diagnostic_summary(all_metrics);
    
    if (!csv_only) {
        std::cout << "\n";
        melvin::diagnostic::print_diagnostic_summary(summary);
    }
    
    // Save results
    melvin::diagnostic::save_diagnostics_csv(all_metrics, "leap_diagnostics.csv");
    
    if (full_report || !csv_only) {
        melvin::diagnostic::generate_diagnostic_report(
            all_metrics, summary, "leap_tuning_report.md");
    }
    
    // ==================== AUTO-TUNING ====================
    
    if (run_auto_tune) {
        std::cout << "\n\n";
        std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  STARTING AUTO-TUNING PROCEDURE                               ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
        
        melvin::autotuning::TuningConfig tuning_config;
        
        if (quick_mode) {
            // Reduce sweep range for quick mode
            tuning_config.lambda_step = 0.4f;
            tuning_config.entropy_threshold_step = 0.2f;
            tuning_config.learning_rate_step = 0.02f;
            tuning_config.test_samples_per_config = 3;
        }
        
        auto tuning_summary = melvin::autotuning::run_comprehensive_auto_tune(
            leap_controller, embedding_bridge, pred_config, tuning_config);
        
        // Print tuning summary
        melvin::autotuning::print_tuning_summary(tuning_summary);
        
        // Save results
        melvin::autotuning::save_tuning_results(tuning_summary, "leap_tuning_results.txt");
        
        // Print config update instructions
        melvin::autotuning::update_config_file(tuning_summary.best_config);
        
        std::cout << "\n📊 Recommendations:\n";
        std::cout << tuning_summary.recommendations << "\n";
    }
    
    // ==================== FINAL SUMMARY ====================
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DIAGNOSTIC COMPLETE                                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "📁 Output files generated:\n";
    std::cout << "   ✓ leap_diagnostics.csv\n";
    if (full_report || !csv_only) {
        std::cout << "   ✓ leap_tuning_report.md\n";
    }
    if (run_auto_tune) {
        std::cout << "   ✓ leap_tuning_results.txt\n";
    }
    std::cout << "\n";
    
    // Print next steps
    std::cout << "🎯 Next Steps:\n";
    
    if (summary.entropy_reduction_healthy && 
        summary.context_similarity_healthy && 
        summary.leap_success_rate_healthy) {
        std::cout << "   ✅ System is healthy - continue monitoring\n";
        std::cout << "   ✅ Run production workloads\n";
    } else {
        if (!run_auto_tune) {
            std::cout << "   ⚠️  System needs tuning - run with --auto-tune\n";
        } else {
            std::cout << "   ⚠️  Apply tuned parameters and re-test\n";
            std::cout << "   ⚠️  Consider increasing training data\n";
        }
    }
    
    std::cout << "   📖 Review leap_tuning_report.md for details\n";
    std::cout << "\n";
    
    // Print statistics
    leap_controller.print_statistics();
    embedding_bridge.print_statistics();
    
    std::cout << "\n✅ Diagnostic session complete!\n\n";
    
    return 0;
}

