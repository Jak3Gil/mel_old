#pragma once

#include "../core/types.h"
#include "../core/storage.h"
#include <memory>

namespace melvin {

/**
 * 🌉 LEAP Bridge - Unified Reasoning Interface
 * 
 * Bridges the LEAP (Large-scale Emergent Abstraction Processing) 
 * reasoning system with Melvin's core.
 * 
 * This is the main reasoning interface that wraps:
 * - Multi-hop search
 * - Context field dynamics
 * - Emergent dimensional influences
 */
class LEAPBridge {
public:
    struct Config {
        int max_hops = 5;
        int beam_width = 8;
        float leap_threshold = 0.7f;
        bool enable_path_logging = true;
        
        Config();
    };
    
    LEAPBridge(const Config& config = Config());
    ~LEAPBridge();
    
    // ========================================================================
    // MAIN INFERENCE
    // ========================================================================
    
    /**
     * Perform multi-hop reasoning on query
     * Returns answer with confidence and reasoning paths
     */
    Answer infer(const Query& query, int k_hops);
    
    /**
     * Set storage backend
     */
    void set_storage(Storage* storage);
    
    /**
     * Enable auto-save (persistence after every query)
     */
    void set_auto_save_paths(const std::string& nodes_path, const std::string& edges_path);
    
    /**
     * Update configuration
     */
    void set_config(const Config& config);
    
    /**
     * Get configuration
     */
    const Config& get_config() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace melvin

