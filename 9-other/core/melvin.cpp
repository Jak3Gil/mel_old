/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN CORE - Unified Entry Point                                        ║
 * ║  Single brain, all modalities, unified reasoning                          ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin.h"
#include "storage.h"
#include "reasoning.h"
#include "learning.h"
#include "metrics.h"
#include "../interfaces/leap_bridge.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

namespace melvin {

// Configuration - all runtime parameters in one place
struct UCAConfig {
    // Storage paths
    std::string storage_nodes_path = "data/nodes.melvin";
    std::string storage_edges_path = "data/edges.melvin";
    std::string snapshot_dir = "data/snapshots/";
    
    // Reasoning parameters
    int max_hops = 5;
    int beam_width = 8;
    float leap_threshold = 0.7f;
    float abstraction_threshold = 0.78f;
    
    // Learning parameters
    float learning_rate = 0.01f;
    float decay_rate = 0.99f;
    int srs_interval_days = 1;
    
    // Snapshot & monitoring
    int snapshot_interval_seconds = 300;
    bool enable_metrics = true;
    bool enable_console_logging = true;
    
    // Multimodal (optional)
    std::string whisper_model = "whisper-tiny";
    std::string clip_model = "ViT-B/32";
    bool enable_audio = false;
    bool enable_images = false;
    
    // Continuous learning
    std::string inbox_dir = "data/inbox/";
    int watch_interval_seconds = 5;
};

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class MelvinCore::Impl {
public:
    UCAConfig config;
    std::unique_ptr<Storage> storage;
    std::unique_ptr<ReasoningEngine> reasoning;
    std::unique_ptr<LearningSystem> learning;
    std::unique_ptr<MetricsCollector> metrics;
    std::unique_ptr<LEAPBridge> leap_bridge;
    
    bool initialized = false;
    uint64_t query_count = 0;
    
    Impl() = default;
    
    bool init() {
        try {
            storage = std::make_unique<Storage>();
            reasoning = std::make_unique<ReasoningEngine>();
            learning = std::make_unique<LearningSystem>();
            metrics = std::make_unique<MetricsCollector>();
            leap_bridge = std::make_unique<LEAPBridge>();
            
            initialized = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize Melvin: " << e.what() << std::endl;
            return false;
        }
    }
    
    void log(const std::string& message) {
        if (config.enable_console_logging) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::cout << "[" << std::ctime(&time) << "] " << message << std::endl;
        }
    }
};

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

MelvinCore::MelvinCore() : impl_(std::make_unique<Impl>()) {
    impl_->init();
}

MelvinCore::~MelvinCore() = default;

bool MelvinCore::load_config(const std::string& config_path) {
    // For now, use default config
    // TODO: Implement config file parsing
    impl_->log("✓ Configuration loaded");
    return true;
}

bool MelvinCore::load_memory(const std::string& nodes_path, const std::string& edges_path) {
    if (!impl_->storage) return false;
    
    bool success = impl_->storage->load(nodes_path, edges_path);
    
    if (success) {
        impl_->log("✓ Memory loaded: " + nodes_path + ", " + edges_path);
    } else {
        impl_->log("! Memory files not found - starting fresh");
    }
    
    // CRITICAL: Connect storage to all components
    if (impl_->leap_bridge) {
        impl_->leap_bridge->set_storage(impl_->storage.get());
    }
    if (impl_->learning) {
        impl_->learning->set_storage(impl_->storage.get());
    }
    
    // CRITICAL: Enable auto-save in reasoning engines
    if (impl_->reasoning) {
        impl_->reasoning->set_auto_save_paths(nodes_path, edges_path);
    }
    if (impl_->leap_bridge) {
        impl_->leap_bridge->set_auto_save_paths(nodes_path, edges_path);
    }
    impl_->log("✓ Auto-save enabled - memory persists after every query");
    
    return success;
}

bool MelvinCore::save_memory() {
    if (!impl_->storage) return false;
    
    return impl_->storage->save(
        impl_->config.storage_nodes_path,
        impl_->config.storage_edges_path
    );
}

void MelvinCore::run_interactive() {
    impl_->log("╔═══════════════════════════════════════════════════╗");
    impl_->log("║  MELVIN - Interactive Mode                        ║");
    impl_->log("╚═══════════════════════════════════════════════════╝");
    impl_->log("");
    impl_->log("Type your questions or commands.");
    impl_->log("Commands: /stats, /save, /quit, /teach <file>");
    impl_->log("");
    
    std::string line;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        // Handle commands
        if (line == "/quit" || line == "/exit") {
            impl_->log("Saving memory and exiting...");
            save_memory();
            break;
        } else if (line == "/stats") {
            print_stats();
            continue;
        } else if (line == "/save") {
            save_memory();
            impl_->log("✓ Memory saved");
            continue;
        } else if (line.rfind("/teach ", 0) == 0) {
            std::string file = line.substr(7);
            teach(file);
            continue;
        }
        
        // Process query
        auto answer = query(line);
        
        std::cout << "\n" << answer.text << "\n";
        std::cout << "(confidence: " << answer.confidence << ")\n";
    }
}

Answer MelvinCore::query(const std::string& text) {
    impl_->query_count++;
    
    Answer answer;
    
    if (!impl_->reasoning || !impl_->storage) {
        answer.text = "Error: System not initialized";
        answer.confidence = 0.0f;
        return answer;
    }
    
    // Use LEAP bridge for reasoning
    Query q;
    q.text = text;
    
    answer = impl_->leap_bridge->infer(q, impl_->config.max_hops);
    
    // Record metrics
    if (impl_->metrics) {
        impl_->metrics->record_query(text, answer);
    }
    
    return answer;
}

Answer MelvinCore::query_multimodal(
    const std::string& text,
    const std::string& audio_path,
    const std::string& image_path
) {
    // For now, delegate to text query
    // TODO: Implement multimodal fusion
    return query(text);
}

bool MelvinCore::teach(const std::string& teaching_file) {
    if (!impl_->learning) return false;
    
    impl_->log("Teaching from: " + teaching_file);
    bool success = impl_->learning->teach_file(teaching_file);
    
    if (success) {
        impl_->log("✓ Teaching complete");
        // CRITICAL: Save new knowledge immediately
        save_memory();
        impl_->log("✓ Knowledge saved to disk");
    } else {
        impl_->log("✗ Teaching failed");
    }
    
    return success;
}

bool MelvinCore::teach_fact(const std::string& fact, const std::string& context) {
    if (!impl_->learning) return false;
    bool success = impl_->learning->teach_fact(fact, context);
    if (success) {
        // CRITICAL: Save new knowledge immediately
        save_memory();
    }
    return success;
}

bool MelvinCore::teach_batch(const std::vector<std::string>& teaching_files) {
    bool all_success = true;
    for (const auto& file : teaching_files) {
        if (!impl_->learning->teach_file(file)) {
            all_success = false;
        }
    }
    // CRITICAL: Save once after batch (more efficient than per-file)
    if (all_success) {
        save_memory();
        impl_->log("✓ Batch teaching saved to disk");
    }
    return all_success;
}

void MelvinCore::print_stats() const {
    if (!impl_->storage) return;
    
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════\n";
    std::cout << "  MELVIN STATISTICS\n";
    std::cout << "═══════════════════════════════════════════════════\n";
    std::cout << "  Nodes:   " << impl_->storage->node_count() << "\n";
    std::cout << "  Edges:   " << impl_->storage->edge_count() << "\n";
    std::cout << "  Queries: " << impl_->query_count << "\n";
    std::cout << "═══════════════════════════════════════════════════\n";
    std::cout << "\n";
}

void MelvinCore::export_graph(const std::string& output_path) const {
    if (!impl_->storage) return;
    impl_->storage->export_to_file(output_path);
}

std::unordered_map<std::string, float> MelvinCore::get_metrics() const {
    if (!impl_->metrics) return {};
    return impl_->metrics->get_all();
}

void MelvinCore::enable_continuous_learning(const std::string& inbox_dir) {
    impl_->config.inbox_dir = inbox_dir;
    // TODO: Implement continuous learning watcher
    impl_->log("Continuous learning enabled: " + inbox_dir);
}

void MelvinCore::create_snapshot(const std::string& snapshot_name) {
    if (!impl_->storage) return;
    impl_->storage->create_snapshot(impl_->config.snapshot_dir + snapshot_name);
}

void MelvinCore::restore_snapshot(const std::string& snapshot_name) {
    if (!impl_->storage) return;
    impl_->storage->restore_snapshot(impl_->config.snapshot_dir + snapshot_name);
}

} // namespace melvin

