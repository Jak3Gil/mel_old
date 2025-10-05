#pragma once

#include <fstream>
#include <string>
#include <iomanip>

namespace Melvin::Utils {

/**
 * Node and Edge composition tracking
 */
enum NodeKind { 
    TAUGHT = 1, 
    THOUGHT = 2 
};

enum Rel2Bit { 
    REL_EXACT = 0, 
    REL_TEMPORAL = 1, 
    REL_LEAP = 2, 
    REL_RESERVED = 3 
};

struct CompositionCounters {
    uint64_t taught_nodes = 0;
    uint64_t thought_nodes = 0;
    uint64_t rel_exact = 0;
    uint64_t rel_temporal = 0;
    uint64_t rel_leap = 0;
    double avg_weight_bits = 0.0;
    double max_weight_bits = 0.0;
    double avg_path_len = 0.0;
    double max_path_len = 0.0;
    double edge_rate = 0.0;
    double e2n_ratio = 0.0;
};

struct StallDetectors {
    bool stalled = false;
    bool runaway = false;
    bool locked = false;
    bool spiky_leap = false;
    std::string alert_message;
};

/**
 * Real-time metrics logger for live monitoring of Melvin's growth
 * Logs nodes, edges, paths, attention entropy, and other key metrics
 * to CSV for real-time visualization and monitoring
 */
struct MetricsLogger {
    std::ofstream out;
    bool wrote_header = false;
    std::string log_path;

    explicit MetricsLogger(const std::string& path) : log_path(path) {
        out.open(path, std::ios::out | std::ios::app);
        if (!out.is_open()) {
            std::cerr << "⚠️  Warning: Could not open metrics log file: " << path << std::endl;
        }
    }
    
    ~MetricsLogger() {
        if (out.is_open()) {
            out.close();
        }
    }

    void header() {
        if (wrote_header || !out.is_open()) return;
        out << "timestamp,step,nodes,edges,paths,"
            << "taught_nodes,thought_nodes,"
            << "rel_exact,rel_temporal,rel_leap,"
            << "avg_weight_bits,max_weight_bits,"
            << "avg_path_len,max_path_len,"
            << "edge_rate,e2n_ratio,"
            << "attn_entropy,diversity,top2_margin,health,latency_ms,fanout,profile,dataset\n";
        wrote_header = true;
        out.flush();
    }
    
    void log(uint64_t step,
             uint64_t nodes,
             uint64_t edges,
             uint64_t paths,
             double attn_entropy,
             double diversity,
             double top2_margin,
             double health,
             double latency_ms,
             uint64_t fanout = 0,
             const std::string& profile = "",
             const std::string& dataset = "") {
        
        if (!out.is_open()) return;
        
        if (!wrote_header) header();
        
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        out << time_t << "." << std::setfill('0') << std::setw(3) << ms.count() << ","
            << step << "," << nodes << "," << edges << "," << paths << ","
            << "0,0,0,0,0,0,0,0,0,0,0,0,"  // Placeholder for composition metrics
            << std::fixed << std::setprecision(4)
            << attn_entropy << "," << diversity << "," << top2_margin << ","
            << health << "," << std::setprecision(2) << latency_ms << ","
            << fanout << "," << profile << "," << dataset << "\n";
        out.flush();
    }
    
    // Enhanced logging with composition tracking
    void logComposition(uint64_t step,
                       uint64_t nodes, uint64_t edges, uint64_t paths,
                       uint64_t taught_nodes, uint64_t thought_nodes,
                       uint64_t rel_exact, uint64_t rel_temporal, uint64_t rel_leap,
                       double avg_weight_bits, double max_weight_bits,
                       double avg_path_len, double max_path_len,
                       double edge_rate, double e2n_ratio,
                       double attn_entropy, double diversity, double top2_margin,
                       double health, double latency_ms, uint64_t fanout,
                       const std::string& profile = "", const std::string& dataset = "") {
        
        if (!out.is_open()) return;
        
        if (!wrote_header) header();
        
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        out << time_t << "." << std::setfill('0') << std::setw(3) << ms.count() << ","
            << step << "," << nodes << "," << edges << "," << paths << ","
            << taught_nodes << "," << thought_nodes << ","
            << rel_exact << "," << rel_temporal << "," << rel_leap << ","
            << std::fixed << std::setprecision(2)
            << avg_weight_bits << "," << max_weight_bits << ","
            << avg_path_len << "," << max_path_len << ","
            << edge_rate << "," << e2n_ratio << ","
            << std::setprecision(4)
            << attn_entropy << "," << diversity << "," << top2_margin << ","
            << health << "," << std::setprecision(2) << latency_ms << ","
            << fanout << "," << profile << "," << dataset << "\n";
        out.flush();
    }
    
    // Convenience method for Melvin-specific metrics
    void logMelvinMetrics(uint64_t step,
                         uint64_t nodes,
                         uint64_t edges,
                         uint64_t paths,
                         double attention_entropy,
                         double output_diversity,
                         double top2_margin,
                         double health_score,
                         double latency_ms,
                         uint64_t fanout,
                         const std::string& profile = "",
                         const std::string& dataset = "") {
        
        log(step, nodes, edges, paths, attention_entropy, output_diversity, 
            top2_margin, health_score, latency_ms, fanout, profile, dataset);
    }
    
    // Log with automatic step counting
    void logWithAutoStep(uint64_t nodes,
                        uint64_t edges,
                        uint64_t paths,
                        double attention_entropy,
                        double output_diversity,
                        double top2_margin,
                        double health_score,
                        double latency_ms,
                        uint64_t fanout,
                        const std::string& profile = "",
                        const std::string& dataset = "") {
        
        static uint64_t auto_step = 0;
        logMelvinMetrics(++auto_step, nodes, edges, paths, attention_entropy, 
                        output_diversity, top2_margin, health_score, latency_ms, 
                        fanout, profile, dataset);
    }
    
    // Check if logger is ready
    bool isReady() const {
        return out.is_open();
    }
    
    // Get log path
    const std::string& getLogPath() const {
        return log_path;
    }
};

/**
 * Global metrics logger instance
 * Can be used throughout the codebase for consistent logging
 */
extern std::unique_ptr<MetricsLogger> g_metrics_logger;

/**
 * Initialize global metrics logger
 */
void initMetricsLogger(const std::string& log_path);

/**
 * Get global metrics logger (creates default if not initialized)
 */
MetricsLogger& getMetricsLogger();

/**
 * Log current Melvin state to metrics
 */
void logCurrentState(uint64_t nodes, uint64_t edges, uint64_t paths,
                    double attention_entropy, double output_diversity,
                    double top2_margin, double health_score, double latency_ms,
                    uint64_t fanout, const std::string& profile = "",
                    const std::string& dataset = "");

/**
 * Composition tracking functions - call these immediately after node/edge creation
 */
void onNodeCreated(NodeKind kind);
void onEdgeCreated(Rel2Bit rel, uint32_t weight_bits);
void onPathCreated(size_t path_length);

/**
 * Stall and runaway detection
 */
StallDetectors detectStalls(uint64_t delta_nodes, uint64_t delta_edges, uint64_t delta_paths,
                           uint64_t delta_rel_temporal, uint64_t delta_rel_leap,
                           double attn_entropy, double diversity, double health);

/**
 * Auto-tuning based on monitor alerts
 */
void autoTuneSettings(bool runaway, bool locked, bool spiky_leap);

/**
 * Log rotation support
 */
void rotateLog(const std::string& timestamp_suffix = "");

/**
 * Rolling mean calculation for path length tracking
 */
double rollingMean(double current_mean, double new_value, double alpha = 0.05);

/**
 * Health band classification
 */
const char* getHealthBand(double health);

} // namespace Melvin::Utils
