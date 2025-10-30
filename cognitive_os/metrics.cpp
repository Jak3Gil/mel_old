/**
 * @file metrics.cpp
 * @brief Implementation of metrics logger
 */

#include "metrics.h"
#include <sstream>
#include <iomanip>
#include <sys/stat.h>

namespace melvin {
namespace cognitive_os {

MetricsLogger::MetricsLogger(const std::string& filepath) {
    // Create logs directory if needed
    mkdir("logs", 0755);
    
    file_.open(filepath, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        // Fallback to current directory
        file_.open("kpis.jsonl", std::ios::out | std::ios::app);
    }
}

MetricsLogger::~MetricsLogger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void MetricsLogger::log(const SystemKPIs& kpis) {
    if (!file_.is_open()) return;
    
    // Write as JSON line
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{";
    oss << "\"t\":" << kpis.timestamp << ",";
    oss << "\"nodes\":" << kpis.active_nodes << ",";
    oss << "\"var\":" << kpis.energy_variance << ",";
    oss << "\"sparsity\":" << kpis.sparsity << ",";
    oss << "\"entropy\":" << kpis.entropy << ",";
    oss << "\"coherence\":" << kpis.coherence << ",";
    oss << "\"confidence\":" << kpis.confidence << ",";
    oss << "\"fps\":" << kpis.fps << ",";
    oss << "\"cpu\":" << kpis.cpu_usage << ",";
    oss << "\"gpu\":" << kpis.gpu_usage << ",";
    oss << "\"dropped\":" << kpis.dropped_msgs << ",";
    oss << "\"services\":" << kpis.services_active;
    oss << "}\n";
    
    file_ << oss.str();
    logs_written_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsLogger::flush() {
    if (file_.is_open()) {
        file_.flush();
    }
}

} // namespace cognitive_os
} // namespace melvin

