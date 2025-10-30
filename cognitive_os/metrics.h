/**
 * @file metrics.h
 * @brief KPI logging system
 * 
 * Logs per-tick metrics to JSONL
 */

#ifndef MELVIN_METRICS_H
#define MELVIN_METRICS_H

#include <string>
#include <fstream>
#include <atomic>

namespace melvin {
namespace cognitive_os {

/**
 * @brief System-wide KPIs
 */
struct SystemKPIs {
    double timestamp;
    
    // Field metrics
    int active_nodes;
    float energy_variance;
    float sparsity;
    float entropy;
    float coherence;
    float confidence;
    
    // Performance
    float fps;
    float cpu_usage;
    float gpu_usage;
    uint64_t dropped_msgs;
    
    // Services
    int services_active;
    float avg_service_load;
};

/**
 * @brief Metrics logger
 */
class MetricsLogger {
public:
    MetricsLogger(const std::string& filepath = "logs/kpis.jsonl");
    ~MetricsLogger();
    
    /**
     * @brief Log KPIs for this tick
     */
    void log(const SystemKPIs& kpis);
    
    /**
     * @brief Flush to disk
     */
    void flush();
    
    /**
     * @brief Get total logs written
     */
    uint64_t logs_written() const {
        return logs_written_.load(std::memory_order_relaxed);
    }
    
private:
    std::ofstream file_;
    std::atomic<uint64_t> logs_written_{0};
};

} // namespace cognitive_os
} // namespace melvin

#endif // MELVIN_METRICS_H

