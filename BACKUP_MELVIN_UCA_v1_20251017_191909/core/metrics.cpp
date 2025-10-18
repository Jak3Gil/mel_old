/*
 * Metrics Collector Implementation
 */

#include "metrics.h"
#include <fstream>
#include <iostream>
#include <deque>

namespace melvin {

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class MetricsCollector::Impl {
public:
    std::unordered_map<std::string, float> metrics;
    std::deque<std::string> recent_queries;
    
    size_t max_recent_queries = 100;
};

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

MetricsCollector::MetricsCollector() : impl_(std::make_unique<Impl>()) {}

MetricsCollector::~MetricsCollector() = default;

void MetricsCollector::record_query(const std::string& query, const Answer& answer) {
    impl_->metrics["total_queries"] += 1.0f;
    impl_->metrics["avg_confidence"] = 
        (impl_->metrics["avg_confidence"] * (impl_->metrics["total_queries"] - 1) + answer.confidence) 
        / impl_->metrics["total_queries"];
    
    impl_->recent_queries.push_back(query);
    if (impl_->recent_queries.size() > impl_->max_recent_queries) {
        impl_->recent_queries.pop_front();
    }
}

void MetricsCollector::record_path(const ReasoningPath& path) {
    impl_->metrics["total_paths"] += 1.0f;
    impl_->metrics["avg_path_length"] = 
        (impl_->metrics["avg_path_length"] * (impl_->metrics["total_paths"] - 1) + path.nodes.size())
        / impl_->metrics["total_paths"];
}

void MetricsCollector::record_learning_event(const std::string& event_type, float value) {
    impl_->metrics["learning_" + event_type] += value;
}

void MetricsCollector::record_custom(const std::string& key, float value) {
    impl_->metrics[key] += value;
}

std::unordered_map<std::string, float> MetricsCollector::get_all() const {
    return impl_->metrics;
}

float MetricsCollector::get(const std::string& key, float default_value) const {
    auto it = impl_->metrics.find(key);
    if (it != impl_->metrics.end()) {
        return it->second;
    }
    return default_value;
}

std::vector<std::string> MetricsCollector::get_recent_queries(int n) const {
    std::vector<std::string> result;
    int start = std::max(0, static_cast<int>(impl_->recent_queries.size()) - n);
    for (size_t i = start; i < impl_->recent_queries.size(); ++i) {
        result.push_back(impl_->recent_queries[i]);
    }
    return result;
}

void MetricsCollector::print_summary() const {
    std::cout << "\n═══════════════════════════════════════\n";
    std::cout << "  METRICS SUMMARY\n";
    std::cout << "═══════════════════════════════════════\n";
    
    for (const auto& [key, value] : impl_->metrics) {
        std::cout << "  " << key << ": " << value << "\n";
    }
    
    std::cout << "═══════════════════════════════════════\n\n";
}

void MetricsCollector::export_csv(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return;
    
    file << "metric,value\n";
    for (const auto& [key, value] : impl_->metrics) {
        file << key << "," << value << "\n";
    }
}

void MetricsCollector::export_json(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return;
    
    file << "{\n";
    bool first = true;
    for (const auto& [key, value] : impl_->metrics) {
        if (!first) file << ",\n";
        file << "  \"" << key << "\": " << value;
        first = false;
    }
    file << "\n}\n";
}

void MetricsCollector::reset() {
    impl_->metrics.clear();
    impl_->recent_queries.clear();
}

void MetricsCollector::clear() {
    reset();
}

} // namespace melvin

