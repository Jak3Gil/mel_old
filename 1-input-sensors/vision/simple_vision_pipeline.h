#pragma once

#include "visual_tokenization.h"
#include "../core/types.h"
#include "../core/optimized_storage.h"
#include "../core/adaptive_window.h"
#include <memory>
#include <vector>
#include <string>

namespace melvin {
namespace vision {

/**
 * SimpleVisionPipeline - C++ Vision without OpenCV dependency
 * 
 * Reads frames from Python vision system via shared memory/files
 * Processes with tokenization + clustering + adaptive window
 * Saves to graph
 */
class SimpleVisionPipeline {
public:
    struct Config {
        std::string frame_source = "data/current_frame.raw";
        int frame_width = 640;
        int frame_height = 480;
        
        int patch_size = 32;
        float attention_threshold = 0.6f;
        
        // Adaptive window
        adaptive::AdaptiveWindowConfig adaptive_config;
        
        bool verbose = false;
    };
    
    SimpleVisionPipeline(optimized::OptimizedStorage* storage, const Config& config = Config());
    ~SimpleVisionPipeline();
    
    /**
     * Process frame data (raw RGB bytes)
     */
    NodeID process_frame_data(const uint8_t* data, const std::vector<float>& attention);
    
    /**
     * Process frame from file
     */
    NodeID process_frame_file(const std::string& frame_path, const std::string& attention_path);
    
    /**
     * Get statistics
     */
    struct Stats {
        uint64_t frames_processed = 0;
        uint64_t objects_created = 0;
        uint64_t scenes_created = 0;
    };
    
    Stats get_stats() const;
    void print_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}} // namespace melvin::vision

