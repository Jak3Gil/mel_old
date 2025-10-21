/*
 * LEAP Bridge Implementation
 */

#include "leap_bridge.h"
#include "../core/reasoning.h"
#include <iostream>

namespace melvin {

// ============================================================================
// CONFIG
// ============================================================================

LEAPBridge::Config::Config() {}

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class LEAPBridge::Impl {
public:
    Config config;
    Storage* storage = nullptr;
    std::unique_ptr<ReasoningEngine> reasoning_engine;
    
    Impl() {
        reasoning_engine = std::make_unique<ReasoningEngine>();
    }
};

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

LEAPBridge::LEAPBridge(const Config& config) : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

LEAPBridge::~LEAPBridge() = default;

Answer LEAPBridge::infer(const Query& query, int k_hops) {
    if (!impl_->storage) {
        Answer answer;
        answer.text = "Error: No storage configured";
        answer.confidence = 0.0f;
        return answer;
    }
    
    // Use reasoning engine for inference
    Answer answer = impl_->reasoning_engine->infer(query, impl_->storage);
    
    // Log path with energy metrics if enabled
    if (impl_->config.enable_path_logging && !answer.paths.empty()) {
        const auto& path = answer.paths[0];
        float actual_hops = answer.metrics.count("actual_hops") ? 
            answer.metrics.at("actual_hops") : 0.0f;
        float final_energy = answer.metrics.count("final_energy") ?
            answer.metrics.at("final_energy") : 0.0f;
        
        std::cout << "[LEAP] requested=" << k_hops 
                  << " actual=" << static_cast<int>(actual_hops)
                  << " energy=" << final_energy
                  << " confidence=" << answer.confidence
                  << " path_len=" << path.nodes.size() << "\n";
    }
    
    return answer;
}

void LEAPBridge::set_storage(Storage* storage) {
    impl_->storage = storage;
}

void LEAPBridge::set_auto_save_paths(const std::string& nodes_path, const std::string& edges_path) {
    if (impl_->reasoning_engine) {
        impl_->reasoning_engine->set_auto_save_paths(nodes_path, edges_path);
    }
}

void LEAPBridge::set_config(const Config& config) {
    impl_->config = config;
}

const LEAPBridge::Config& LEAPBridge::get_config() const {
    return impl_->config;
}

} // namespace melvin

