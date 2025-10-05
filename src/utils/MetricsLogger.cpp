#include "MetricsLogger.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <fstream>
#include <algorithm>
#include <cmath>

namespace Melvin::Utils {

// Global metrics logger instance
std::unique_ptr<MetricsLogger> g_metrics_logger = nullptr;

// Global composition counters
static CompositionCounters g_counters;
static uint64_t g_last_nodes = 0;
static uint64_t g_last_edges = 0;
static uint64_t g_last_paths = 0;
static std::chrono::steady_clock::time_point g_last_time;

void initMetricsLogger(const std::string& log_path) {
    g_metrics_logger = std::make_unique<MetricsLogger>(log_path);
    g_last_time = std::chrono::steady_clock::now();
    std::cout << "📊 Initialized enhanced metrics logger: " << log_path << std::endl;
}

MetricsLogger& getMetricsLogger() {
    if (!g_metrics_logger) {
        // Default to metrics_live.csv in current directory
        initMetricsLogger("out/metrics_live.csv");
    }
    return *g_metrics_logger;
}

void logCurrentState(uint64_t nodes, uint64_t edges, uint64_t paths,
                    double attention_entropy, double output_diversity,
                    double top2_margin, double health_score, double latency_ms,
                    uint64_t fanout, const std::string& profile,
                    const std::string& dataset) {
    
    auto& logger = getMetricsLogger();
    logger.logWithAutoStep(nodes, edges, paths, attention_entropy, output_diversity,
                          top2_margin, health_score, latency_ms, fanout, profile, dataset);
}

// Composition tracking functions
void onNodeCreated(NodeKind kind) {
    if (kind == TAUGHT) {
        g_counters.taught_nodes++;
    } else if (kind == THOUGHT) {
        g_counters.thought_nodes++;
    }
}

void onEdgeCreated(Rel2Bit rel, uint32_t weight_bits) {
    switch (rel) {
        case REL_EXACT:
            g_counters.rel_exact++;
            break;
        case REL_TEMPORAL:
            g_counters.rel_temporal++;
            break;
        case REL_LEAP:
            g_counters.rel_leap++;
            break;
    }
    
    // Update weight statistics
    g_counters.max_weight_bits = std::max(g_counters.max_weight_bits, (double)weight_bits);
    // Simple running average for weight bits
    double total_edges = g_counters.rel_exact + g_counters.rel_temporal + g_counters.rel_leap;
    if (total_edges > 0) {
        g_counters.avg_weight_bits = (g_counters.avg_weight_bits * (total_edges - 1) + weight_bits) / total_edges;
    }
}

void onPathCreated(size_t path_length) {
    g_counters.max_path_len = std::max(g_counters.max_path_len, (double)path_length);
    g_counters.avg_path_len = rollingMean(g_counters.avg_path_len, path_length);
}

// Stall detection
StallDetectors detectStalls(uint64_t delta_nodes, uint64_t delta_edges, uint64_t delta_paths,
                           uint64_t delta_rel_temporal, uint64_t delta_rel_leap,
                           double attn_entropy, double diversity, double health) {
    StallDetectors detectors;
    
    // Stalled: no growth in any dimension
    detectors.stalled = (delta_nodes == 0 && delta_edges == 0 && delta_paths == 0);
    
    // Runaway: excessive edge growth with high entropy
    detectors.runaway = (delta_edges > 50 && attn_entropy > 0.40);
    
    // Locked: very low entropy and diversity
    detectors.locked = (attn_entropy < 0.06 && diversity < 0.30);
    
    // Spiky leap: more leap edges than temporal
    detectors.spiky_leap = (delta_rel_leap > (delta_rel_temporal * 2));
    
    // Generate alert message
    if (detectors.runaway) {
        detectors.alert_message = "⚠️ runaway edges (entropy " + std::to_string(attn_entropy).substr(0, 4) + 
                                 ") — consider lowering fanout_k or raising leap_threshold";
    } else if (detectors.locked) {
        detectors.alert_message = "🔒 mode lock detected — consider increasing temperature or top_p";
    } else if (detectors.spiky_leap) {
        detectors.alert_message = "🎯 excessive leap edges — consider raising leap_threshold";
    } else if (detectors.stalled) {
        detectors.alert_message = "⏸️ stalled growth — check input or increase exploration";
    }
    
    return detectors;
}

// Auto-tuning
void autoTuneSettings(bool runaway, bool locked, bool spiky_leap) {
    // This would need access to Melvin's UCA config
    // For now, log the recommendations
    std::ofstream autotune_log("out/autotune.log", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    if (runaway) {
        autotune_log << "[" << time_t << "] RECOMMEND: fanout_k: -2, leap_threshold: +0.03 (runaway edges)\n";
    }
    if (locked) {
        autotune_log << "[" << time_t << "] RECOMMEND: temperature: +0.1, top_p: +0.02 (mode lock)\n";
    }
    if (spiky_leap) {
        autotune_log << "[" << time_t << "] RECOMMEND: leap_threshold: +0.05 (excessive leaps)\n";
    }
}

// Log rotation
void rotateLog(const std::string& timestamp_suffix) {
    if (g_metrics_logger) {
        std::string new_path = "out/metrics_" + timestamp_suffix + ".csv";
        g_metrics_logger = std::make_unique<MetricsLogger>(new_path);
        std::cout << "📊 Rotated metrics log to: " << new_path << std::endl;
    }
}

// Rolling mean calculation
double rollingMean(double current_mean, double new_value, double alpha) {
    return alpha * new_value + (1.0 - alpha) * current_mean;
}

// Health band classification
const char* getHealthBand(double health) {
    if (health >= 0.80) return "GREEN";
    if (health >= 0.60) return "YELLOW";
    return "RED";
}

} // namespace Melvin::Utils
