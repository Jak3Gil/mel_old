#pragma once

#include "types.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace melvin {

/**
 * 📊 MetricsCollector - Performance Tracking & Diagnostics
 * 
 * Tracks:
 * - Query performance
 * - Path statistics
 * - Memory growth
 * - Learning effectiveness
 * - LEAP metrics
 */
class MetricsCollector {
public:
    MetricsCollector();
    ~MetricsCollector();
    
    // ========================================================================
    // RECORDING
    // ========================================================================
    
    void record_query(const std::string& query, const Answer& answer);
    void record_path(const ReasoningPath& path);
    void record_learning_event(const std::string& event_type, float value);
    void record_custom(const std::string& key, float value);
    
    // ========================================================================
    // RETRIEVAL
    // ========================================================================
    
    std::unordered_map<std::string, float> get_all() const;
    float get(const std::string& key, float default_value = 0.0f) const;
    
    std::vector<std::string> get_recent_queries(int n = 10) const;
    
    // ========================================================================
    // REPORTING
    // ========================================================================
    
    void print_summary() const;
    void export_csv(const std::string& path) const;
    void export_json(const std::string& path) const;
    
    // ========================================================================
    // MANAGEMENT
    // ========================================================================
    
    void reset();
    void clear();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace melvin

