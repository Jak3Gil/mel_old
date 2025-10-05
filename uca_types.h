#pragma once

#include "melvin_types.h"
#include <string>
#include <unordered_map>

namespace melvin {

// Forward declaration
enum class OutputType {
    TEXT = 0,
    SPEECH = 1,
    ACTION = 2,
    MULTI_MODAL = 3,
    ABSTRACTION = 4,
    QUESTION = 5,
    CLARIFICATION = 6
};

/**
 * MelvinParams - Parameters for Melvin's cognitive system
 */
struct MelvinParams {
    // Reasoning parameters
    float confidence_threshold = 0.45f;
    int max_depth = 6;
    int beam_width = 8;
    
    // Learning parameters
    float reinforcement_rate = 0.1f;
    float decay_rate = 0.01f;
    
    // Reflection parameters
    float stagnation_threshold = 0.2f;
    bool micro_evolution_enabled = true;
    
    // Output parameters
    OutputType default_output_type;
    float output_confidence_threshold = 0.1f;
    
    MelvinParams() : default_output_type(OutputType::TEXT) {}
};

/**
 * DynamicGenome - Simplified genome for micro-evolution
 */
struct DynamicGenome {
    std::string id;
    uint64_t seed{0};
    std::unordered_map<std::string, float> values;
    
    DynamicGenome() = default;
    DynamicGenome(const std::string& id) : id(id) {}
    
    std::string to_json() const {
        return "{\"id\":\"" + id + "\",\"seed\":" + std::to_string(seed) + "}";
    }
};

} // namespace melvin
