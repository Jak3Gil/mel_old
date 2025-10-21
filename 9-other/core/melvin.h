#pragma once

#include "types.h"
#include <string>
#include <memory>

namespace melvin {

// Forward declarations
class Storage;
class ReasoningEngine;
class LearningSystem;
class MetricsCollector;
struct UCAConfig;

/**
 * 🧠 MelvinCore - Unified Entry Point
 * 
 * Single brain instance that handles all modalities:
 * - Text input/output
 * - Audio (Whisper integration)
 * - Images (CLIP integration)
 * - Motor control & actions
 * 
 * All inputs flow through the same reasoning pipeline.
 * All outputs are generated through the same field-based system.
 */
class MelvinCore {
public:
    MelvinCore();
    ~MelvinCore();
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    bool load_config(const std::string& config_path);
    bool load_memory(const std::string& nodes_path, const std::string& edges_path);
    bool save_memory();
    
    // ========================================================================
    // MAIN ENTRY POINTS
    // ========================================================================
    
    // Interactive mode - runs forever, processing inputs
    void run_interactive();
    
    // Single query mode
    Answer query(const std::string& text);
    
    // Multimodal query
    Answer query_multimodal(
        const std::string& text,
        const std::string& audio_path = "",
        const std::string& image_path = ""
    );
    
    // ========================================================================
    // LEARNING & TEACHING
    // ========================================================================
    
    bool teach(const std::string& teaching_file);
    bool teach_fact(const std::string& fact, const std::string& context = "");
    bool teach_batch(const std::vector<std::string>& teaching_files);
    
    // ========================================================================
    // MONITORING & DIAGNOSTICS
    // ========================================================================
    
    void print_stats() const;
    void export_graph(const std::string& output_path) const;
    std::unordered_map<std::string, float> get_metrics() const;
    
    // ========================================================================
    // ADVANCED
    // ========================================================================
    
    void enable_continuous_learning(const std::string& inbox_dir);
    void create_snapshot(const std::string& snapshot_name);
    void restore_snapshot(const std::string& snapshot_name);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace melvin

