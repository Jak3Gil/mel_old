#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <fstream>

namespace melvin {
namespace scale_demo {

// Fitness and telemetry system
struct FitnessMetrics {
    // Recall metrics
    double recall_at_k = 0.0;        // % of queries with correct top path
    uint32_t correct_recalls = 0;
    uint32_t total_queries = 0;
    
    // Latency metrics
    double median_latency_ms = 0.0;
    double p95_latency_ms = 0.0;
    std::vector<double> latency_samples;
    
    // Stability metrics
    double stability_drift = 0.0;     // Cosine distance of top dimensions
    double variance_impact_corr = 0.0; // Correlation of variance to fitness
    
    // Connection chemistry
    double temporal_edge_usage = 0.0;  // % of traversals using TEMPORAL
    double leap_edge_usage = 0.0;      // % of traversals using LEAP
    double similar_edge_usage = 0.0;   // % of traversals using SIMILAR
    double top2_margin = 0.0;          // Decision clarity
    
    // Composite fitness
    double composite_fitness = 0.0;
    
    // Compute composite from components
    void compute_composite();
};

struct CycleSnapshot {
    uint64_t cycle = 0;
    uint64_t timestamp_ms = 0;
    
    FitnessMetrics metrics;
    
    // Graph stats
    size_t nodes = 0;
    size_t edges = 0;
    size_t temporal_edges = 0;
    size_t leap_edges = 0;
    size_t similar_edges = 0;
    
    // Dimensional emergence (simplified tracking)
    size_t promoted_dimensions = 0;
    size_t demoted_dimensions = 0;
    
    std::string to_csv_row() const;
    static std::string csv_header();
};

class MetricsCollector {
public:
    MetricsCollector(const std::string& output_csv, const std::string& evolution_log);
    ~MetricsCollector();
    
    void record_snapshot(const CycleSnapshot& snapshot);
    
    void flush();
    
private:
    std::ofstream metrics_csv_;
    std::ofstream evolution_log_;
    bool header_written_ = false;
};

// Query evaluator for ground-truth validation
class QueryEvaluator {
public:
    struct EvaluationResult {
        std::string query;
        std::vector<std::string> generated_path;
        std::vector<std::string> expected_path;
        bool correct = false;
        double latency_ms = 0.0;
        double confidence = 0.0;
    };
    
    QueryEvaluator();
    
    // Evaluate a query against ground truth
    EvaluationResult evaluate_query(const std::string& query,
                                     const std::vector<std::string>& expected_path,
                                     const std::vector<std::string>& generated_path,
                                     double latency_ms);
    
    // Compute aggregate metrics from evaluations
    FitnessMetrics compute_metrics(const std::vector<EvaluationResult>& results);
    
private:
    bool paths_match(const std::vector<std::string>& generated,
                    const std::vector<std::string>& expected);
};

// Live telemetry (per 1000 events)
class LiveTelemetry {
public:
    struct LiveStats {
        uint64_t events_processed = 0;
        uint64_t nodes_appended = 0;
        uint64_t edges_appended = 0;
        double flush_time_ms = 0.0;
        size_t mmap_faults = 0;
        
        double mean_chemistry = 0.0;
        double variance_chemistry = 0.0;
        double traversal_entropy = 0.0;
        
        uint32_t promotions = 0;
        uint32_t demotions = 0;
    };
    
    LiveTelemetry();
    
    void record_event();
    void record_node();
    void record_edge();
    void record_flush(double time_ms);
    
    void record_chemistry_value(double value);
    void record_promotion();
    void record_demotion();
    
    LiveStats get_stats() const { return stats_; }
    void reset();
    
    bool should_report() const { return stats_.events_processed % 1000 == 0; }
    
private:
    LiveStats stats_;
    std::vector<double> chemistry_values_;
};

// Checkpoint manager
class CheckpointManager {
public:
    CheckpointManager(const std::string& checkpoint_dir);
    
    void save_checkpoint(const std::string& name, 
                        const std::string& memory_file,
                        const CycleSnapshot& snapshot);
    
    bool load_checkpoint(const std::string& name,
                        std::string& memory_file,
                        CycleSnapshot& snapshot);
    
    std::vector<std::string> list_checkpoints() const;
    
private:
    std::string checkpoint_dir_;
};

} // namespace scale_demo
} // namespace melvin

