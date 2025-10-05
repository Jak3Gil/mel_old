#include "OutputManager.hpp"
#include <algorithm>
#include <iostream>

namespace melvin {

OutputManager::OutputManager() 
    : default_output_type_(OutputType::TEXT), output_enabled_(true), 
      confidence_threshold_(0.1f), output_count_(0), successful_outputs_(0) {
}

bool OutputManager::dispatch(const OutputIntent& intent) {
    if (!output_enabled_ || !should_output(intent)) {
        return false;
    }
    
    output_count_++;
    
    // Find appropriate driver
    std::shared_ptr<OutputDriver> driver = find_driver(intent.output_type);
    if (!driver) {
        // Fallback to default driver
        driver = find_driver(default_output_type_);
    }
    
    if (!driver) {
        update_statistics(false);
        return false;
    }
    
    // Dispatch to driver
    bool success = driver->output(intent);
    update_statistics(success);
    
    return success;
}

bool OutputManager::dispatch(const Thought& thought) {
    // Convert thought to output intent
    OutputIntent intent(thought.text, thought.confidence, thought.output_type);
    return dispatch(intent);
}

void OutputManager::register_driver(std::shared_ptr<OutputDriver> driver) {
    if (driver) {
        drivers_.push_back(driver);
    }
}

void OutputManager::unregister_driver(const std::string& driver_name) {
    drivers_.erase(
        std::remove_if(drivers_.begin(), drivers_.end(),
            [&driver_name](const std::shared_ptr<OutputDriver>& driver) {
                return driver->get_driver_name() == driver_name;
            }),
        drivers_.end()
    );
}

std::vector<std::string> OutputManager::get_available_drivers() const {
    std::vector<std::string> names;
    for (const auto& driver : drivers_) {
        names.push_back(driver->get_driver_name());
    }
    return names;
}

void OutputManager::set_default_output_type(OutputType type) {
    default_output_type_ = type;
}

void OutputManager::set_output_enabled(bool enabled) {
    output_enabled_ = enabled;
}

void OutputManager::set_confidence_threshold(float threshold) {
    confidence_threshold_ = std::max(0.0f, std::min(1.0f, threshold));
}

size_t OutputManager::get_output_count() const {
    return output_count_.load();
}

size_t OutputManager::get_successful_outputs() const {
    return successful_outputs_.load();
}

float OutputManager::get_success_rate() const {
    size_t count = output_count_.load();
    if (count == 0) return 0.0f;
    return static_cast<float>(successful_outputs_.load()) / count;
}

void OutputManager::reset_statistics() {
    output_count_.store(0);
    successful_outputs_.store(0);
}

std::shared_ptr<OutputDriver> OutputManager::find_driver(OutputType type) {
    for (const auto& driver : drivers_) {
        if (driver->can_handle(type)) {
            return driver;
        }
    }
    return nullptr;
}

bool OutputManager::should_output(const OutputIntent& intent) {
    return intent.confidence >= confidence_threshold_;
}

void OutputManager::update_statistics(bool success) {
    if (success) {
        successful_outputs_++;
    }
}

} // namespace melvin
