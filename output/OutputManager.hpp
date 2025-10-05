#pragma once

#include "melvin_types.h"
#include "../reasoning/ReasoningEngine.hpp"
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>

namespace melvin {

/**
 * OutputIntent - Structured command for output generation
 */
struct OutputIntent {
    std::string content;           // The content to output
    float confidence;             // Confidence in the output
    OutputType output_type;       // Type of output (text, speech, etc.)
    std::vector<std::string> metadata; // Additional metadata
    uint64_t timestamp;           // When this intent was created
    
    OutputIntent() : confidence(0.0f), output_type(OutputType::TEXT), timestamp(0) {}
    OutputIntent(const std::string& c, float conf, OutputType type) 
        : content(c), confidence(conf), output_type(type) {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

/**
 * OutputDriver - Base class for output drivers
 */
class OutputDriver {
public:
    virtual ~OutputDriver() = default;
    virtual bool can_handle(OutputType type) const = 0;
    virtual bool output(const OutputIntent& intent) = 0;
    virtual std::string get_driver_name() const = 0;
};

/**
 * OutputManager - Dispatches output intents to appropriate drivers
 * 
 * Responsibilities:
 * - Route output intents to correct drivers
 * - Manage multiple output modalities
 * - Handle output failures gracefully
 * - Provide output statistics
 */
class OutputManager {
public:
    OutputManager();
    ~OutputManager() = default;

    // Core output methods
    bool dispatch(const OutputIntent& intent);
    bool dispatch(const Thought& thought);
    
    // Driver management
    void register_driver(std::shared_ptr<OutputDriver> driver);
    void unregister_driver(const std::string& driver_name);
    std::vector<std::string> get_available_drivers() const;
    
    // Output configuration
    void set_default_output_type(OutputType type);
    void set_output_enabled(bool enabled);
    void set_confidence_threshold(float threshold);
    
    // Statistics
    size_t get_output_count() const;
    size_t get_successful_outputs() const;
    float get_success_rate() const;
    void reset_statistics();

private:
    std::vector<std::shared_ptr<OutputDriver>> drivers_;
    OutputType default_output_type_;
    bool output_enabled_;
    float confidence_threshold_;
    
    // Statistics
    std::atomic<size_t> output_count_;
    std::atomic<size_t> successful_outputs_;
    
    // Internal methods
    std::shared_ptr<OutputDriver> find_driver(OutputType type);
    bool should_output(const OutputIntent& intent);
    void update_statistics(bool success);
};

} // namespace melvin
