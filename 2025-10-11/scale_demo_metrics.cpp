#include "scale_demo_metrics.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace melvin {
namespace scale_demo {

// ============================================================================
// FitnessMetrics
// ============================================================================

void FitnessMetrics::compute_composite() {
    // Composite fitness formula:
    // fitness = recall_weight * recall_at_k 
    //         - latency_weight * (median_latency_ms / 100.0)
    //         + stability_weight * (1.0 - stability_drift)
    //         + variance_impact_weight * variance_impact_corr
    
    constexpr double recall_weight = 0.50;
    constexpr double latency_weight = 0.15;
    constexpr double stability_weight = 0.20;
    constexpr double variance_weight = 0.15;
    
    composite_fitness = 
        recall_weight * recall_at_k
        - latency_weight * (median_latency_ms / 100.0)
        + stability_weight * (1.0 - stability_drift)
        + variance_weight * variance_impact_corr;
}

// ============================================================================
// CycleSnapshot
// ============================================================================

std::string CycleSnapshot::csv_header() {
    return "cycle,timestamp_ms,nodes,edges,temporal_edges,leap_edges,similar_edges,"
           "recall_at_k,correct_recalls,total_queries,median_latency_ms,p95_latency_ms,"
           "stability_drift,variance_impact_corr,temporal_usage,leap_usage,similar_usage,"
           "top2_margin,composite_fitness,promoted_dims,demoted_dims";
}

std::string CycleSnapshot::to_csv_row() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    oss << cycle << ","
        << timestamp_ms << ","
        << nodes << ","
        << edges << ","
        << temporal_edges << ","
        << leap_edges << ","
        << similar_edges << ","
        << metrics.recall_at_k << ","
        << metrics.correct_recalls << ","
        << metrics.total_queries << ","
        << metrics.median_latency_ms << ","
        << metrics.p95_latency_ms << ","
        << metrics.stability_drift << ","
        << metrics.variance_impact_corr << ","
        << metrics.temporal_edge_usage << ","
        << metrics.leap_edge_usage << ","
        << metrics.similar_edge_usage << ","
        << metrics.top2_margin << ","
        << metrics.composite_fitness << ","
        << promoted_dimensions << ","
        << demoted_dimensions;
    
    return oss.str();
}

// ============================================================================
// MetricsCollector
// ============================================================================

MetricsCollector::MetricsCollector(const std::string& output_csv, 
                                   const std::string& evolution_log) {
    metrics_csv_.open(output_csv, std::ios::out | std::ios::trunc);
    evolution_log_.open(evolution_log, std::ios::out | std::ios::trunc);
    
    if (!metrics_csv_.is_open()) {
        throw std::runtime_error("Failed to open metrics CSV: " + output_csv);
    }
    
    if (!evolution_log_.is_open()) {
        throw std::runtime_error("Failed to open evolution log: " + evolution_log);
    }
}

MetricsCollector::~MetricsCollector() {
    flush();
    metrics_csv_.close();
    evolution_log_.close();
}

void MetricsCollector::record_snapshot(const CycleSnapshot& snapshot) {
    if (!header_written_) {
        metrics_csv_ << CycleSnapshot::csv_header() << "\n";
        header_written_ = true;
    }
    
    metrics_csv_ << snapshot.to_csv_row() << "\n";
    
    // Write to evolution log as JSON
    evolution_log_ << "{\"cycle\":" << snapshot.cycle 
                   << ",\"fitness\":" << snapshot.metrics.composite_fitness
                   << ",\"nodes\":" << snapshot.nodes
                   << ",\"edges\":" << snapshot.edges
                   << ",\"recall\":" << snapshot.metrics.recall_at_k
                   << ",\"latency\":" << snapshot.metrics.median_latency_ms
                   << "}\n";
}

void MetricsCollector::flush() {
    metrics_csv_.flush();
    evolution_log_.flush();
}

// ============================================================================
// QueryEvaluator
// ============================================================================

QueryEvaluator::QueryEvaluator() {}

QueryEvaluator::EvaluationResult QueryEvaluator::evaluate_query(
    const std::string& query,
    const std::vector<std::string>& expected_path,
    const std::vector<std::string>& generated_path,
    double latency_ms) {
    
    EvaluationResult result;
    result.query = query;
    result.expected_path = expected_path;
    result.generated_path = generated_path;
    result.latency_ms = latency_ms;
    result.correct = paths_match(generated_path, expected_path);
    
    // Simple confidence: inverse of path length deviation
    if (!generated_path.empty() && !expected_path.empty()) {
        double length_diff = std::abs(static_cast<double>(generated_path.size()) - 
                                     static_cast<double>(expected_path.size()));
        result.confidence = 1.0 / (1.0 + length_diff);
    } else {
        result.confidence = 0.0;
    }
    
    return result;
}

FitnessMetrics QueryEvaluator::compute_metrics(const std::vector<EvaluationResult>& results) {
    FitnessMetrics metrics;
    
    if (results.empty()) return metrics;
    
    // Recall
    metrics.total_queries = static_cast<uint32_t>(results.size());
    for (const auto& result : results) {
        if (result.correct) {
            metrics.correct_recalls++;
        }
    }
    metrics.recall_at_k = static_cast<double>(metrics.correct_recalls) / 
                         static_cast<double>(metrics.total_queries);
    
    // Latency
    metrics.latency_samples.clear();
    for (const auto& result : results) {
        metrics.latency_samples.push_back(result.latency_ms);
    }
    
    std::sort(metrics.latency_samples.begin(), metrics.latency_samples.end());
    
    size_t median_idx = metrics.latency_samples.size() / 2;
    metrics.median_latency_ms = metrics.latency_samples[median_idx];
    
    size_t p95_idx = static_cast<size_t>(metrics.latency_samples.size() * 0.95);
    metrics.p95_latency_ms = metrics.latency_samples[std::min(p95_idx, metrics.latency_samples.size() - 1)];
    
    // Placeholder values for dimensional metrics (would need full system)
    metrics.stability_drift = 0.05;  // Low drift is good
    metrics.variance_impact_corr = 0.6;  // Positive correlation is good
    
    // Edge usage (placeholder - would need traversal tracking)
    metrics.temporal_edge_usage = 0.70;
    metrics.leap_edge_usage = 0.20;
    metrics.similar_edge_usage = 0.10;
    metrics.top2_margin = 0.25;
    
    metrics.compute_composite();
    
    return metrics;
}

bool QueryEvaluator::paths_match(const std::vector<std::string>& generated,
                                 const std::vector<std::string>& expected) {
    if (generated.size() != expected.size()) return false;
    
    // Allow partial match (at least 70% tokens correct in sequence)
    size_t matches = 0;
    size_t min_size = std::min(generated.size(), expected.size());
    
    for (size_t i = 0; i < min_size; ++i) {
        if (generated[i] == expected[i]) {
            matches++;
        }
    }
    
    double match_ratio = static_cast<double>(matches) / static_cast<double>(expected.size());
    return match_ratio >= 0.7;
}

// ============================================================================
// LiveTelemetry
// ============================================================================

LiveTelemetry::LiveTelemetry() {}

void LiveTelemetry::record_event() {
    stats_.events_processed++;
}

void LiveTelemetry::record_node() {
    stats_.nodes_appended++;
}

void LiveTelemetry::record_edge() {
    stats_.edges_appended++;
}

void LiveTelemetry::record_flush(double time_ms) {
    stats_.flush_time_ms = time_ms;
}

void LiveTelemetry::record_chemistry_value(double value) {
    chemistry_values_.push_back(value);
    
    // Compute running statistics
    if (!chemistry_values_.empty()) {
        double sum = std::accumulate(chemistry_values_.begin(), chemistry_values_.end(), 0.0);
        stats_.mean_chemistry = sum / chemistry_values_.size();
        
        double sq_sum = 0.0;
        for (double v : chemistry_values_) {
            sq_sum += (v - stats_.mean_chemistry) * (v - stats_.mean_chemistry);
        }
        stats_.variance_chemistry = sq_sum / chemistry_values_.size();
    }
}

void LiveTelemetry::record_promotion() {
    stats_.promotions++;
}

void LiveTelemetry::record_demotion() {
    stats_.demotions++;
}

void LiveTelemetry::reset() {
    stats_ = LiveStats{};
    chemistry_values_.clear();
}

// ============================================================================
// CheckpointManager
// ============================================================================

CheckpointManager::CheckpointManager(const std::string& checkpoint_dir) 
    : checkpoint_dir_(checkpoint_dir) {
    std::filesystem::create_directories(checkpoint_dir);
}

void CheckpointManager::save_checkpoint(const std::string& name,
                                       const std::string& memory_file,
                                       const CycleSnapshot& snapshot) {
    std::string checkpoint_path = checkpoint_dir_ + "/" + name;
    std::filesystem::create_directories(checkpoint_path);
    
    // Copy memory file
    std::string dest_memory = checkpoint_path + "/memory.bin";
    std::filesystem::copy_file(memory_file, dest_memory, 
                              std::filesystem::copy_options::overwrite_existing);
    
    // Write snapshot metadata
    std::string meta_path = checkpoint_path + "/metadata.csv";
    std::ofstream meta(meta_path);
    meta << CycleSnapshot::csv_header() << "\n";
    meta << snapshot.to_csv_row() << "\n";
    meta.close();
}

bool CheckpointManager::load_checkpoint(const std::string& name,
                                       std::string& memory_file,
                                       CycleSnapshot& snapshot) {
    std::string checkpoint_path = checkpoint_dir_ + "/" + name;
    
    if (!std::filesystem::exists(checkpoint_path)) {
        return false;
    }
    
    memory_file = checkpoint_path + "/memory.bin";
    
    // Load metadata (simplified - would parse CSV properly)
    std::string meta_path = checkpoint_path + "/metadata.csv";
    std::ifstream meta(meta_path);
    if (!meta.is_open()) return false;
    
    std::string header;
    std::getline(meta, header);  // Skip header
    
    // Would parse snapshot data here
    
    return true;
}

std::vector<std::string> CheckpointManager::list_checkpoints() const {
    std::vector<std::string> checkpoints;
    
    if (!std::filesystem::exists(checkpoint_dir_)) {
        return checkpoints;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(checkpoint_dir_)) {
        if (entry.is_directory()) {
            checkpoints.push_back(entry.path().filename().string());
        }
    }
    
    std::sort(checkpoints.begin(), checkpoints.end());
    
    return checkpoints;
}

} // namespace scale_demo
} // namespace melvin

