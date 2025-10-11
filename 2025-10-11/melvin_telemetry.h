/*
 * MELVIN LLM TRAINING TELEMETRY SYSTEM
 * 
 * JSON telemetry logging for monitoring training progress,
 * genome evolution, and system health.
 */

#pragma once

#include "melvin_guardrails.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <queue>

namespace melvin {

// ==================== TELEMETRY EVENTS ====================

enum class TelemetryEventType {
    EVOLUTION_TRIGGERED,
    SLEEP_CYCLE,
    GUARDRAIL_ALERT,
    CHAMPION_PROMOTED,
    GENOME_BLACKLISTED,
    EVALUATION_COMPLETE,
    PARAMETER_ADJUSTMENT,
    TASK_MODE_SWITCH
};

struct TelemetryEvent {
    TelemetryEventType type;
    uint64_t timestamp;
    std::string message;
    std::unordered_map<std::string, std::string> metadata;
    
    TelemetryEvent(TelemetryEventType t, const std::string& msg) 
        : type(t), timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()), message(msg) {}
};

// ==================== TELEMETRY LOGGER ====================

class TelemetryLogger {
private:
    std::ofstream log_file_;
    std::queue<TelemetryEvent> event_buffer_;
    size_t max_buffer_size_ = 1000;
    uint64_t last_log_timestamp_ = 0;
    uint64_t log_interval_ms_ = 1000;  // Log every 1k steps (1 second)
    
    // Telemetry data
    struct TelemetryData {
        uint64_t t = 0;  // timestamp
        GenomeSnapshot genome;
        MetricSnapshot metrics;
        std::vector<std::string> events;
        
        std::string to_json() const {
            std::stringstream ss;
            ss << "{\n";
            ss << "  \"t\": " << t << ",\n";
            ss << "  \"genome\": {\n";
            ss << "    \"alpha\": " << std::fixed << std::setprecision(3) << genome.alpha << ",\n";
            ss << "    \"beta\": " << genome.beta << ",\n";
            ss << "    \"gamma\": " << genome.gamma << ",\n";
            ss << "    \"eta\": " << genome.eta << ",\n";
            ss << "    \"delta\": " << genome.delta << ",\n";
            ss << "    \"eps\": " << genome.epsilon << "\n";
            ss << "  },\n";
            ss << "  \"metrics\": {\n";
            ss << "    \"entropy\": {\n";
            ss << "      \"short\": " << metrics.entropy_short << ",\n";
            ss << "      \"mid\": " << metrics.entropy_mid << ",\n";
            ss << "      \"long\": " << metrics.entropy_long << "\n";
            ss << "    },\n";
            ss << "    \"top2_margin\": {\n";
            ss << "      \"mid\": " << metrics.top2_margin_mid << "\n";
            ss << "    },\n";
            ss << "    \"success\": {\n";
            ss << "      \"short\": " << metrics.success_short << ",\n";
            ss << "      \"mid\": " << metrics.success_mid << "\n";
            ss << "    },\n";
            ss << "    \"drift\": {\n";
            ss << "      \"long\": " << metrics.drift_long << "\n";
            ss << "    },\n";
            ss << "    \"thought_replay\": {\n";
            ss << "      \"mid\": " << metrics.thought_replay_mid << "\n";
            ss << "    }\n";
            ss << "  },\n";
            ss << "  \"events\": [";
            for (size_t i = 0; i < events.size(); ++i) {
                if (i > 0) ss << ",";
                ss << "\"" << events[i] << "\"";
            }
            ss << "]\n";
            ss << "}";
            return ss.str();
        }
    };
    
public:
    TelemetryLogger(const std::string& filename = "melvin_telemetry.jsonl") {
        log_file_.open(filename, std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open telemetry log file: " << filename << "\n";
        } else {
            std::cout << "📊 Telemetry logging to: " << filename << "\n";
        }
    }
    
    ~TelemetryLogger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }
    
    // Log telemetry data (called every 1k steps)
    void log_telemetry(const GenomeSnapshot& genome, const MetricSnapshot& metrics) {
        auto now = std::chrono::system_clock::now();
        uint64_t current_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        // Only log if enough time has passed
        if (current_timestamp - last_log_timestamp_ < log_interval_ms_) {
            return;
        }
        
        TelemetryData data;
        data.t = current_timestamp;
        data.genome = genome;
        data.metrics = metrics;
        
        // Add recent events to the log entry
        while (!event_buffer_.empty()) {
            const auto& event = event_buffer_.front();
            data.events.push_back(event.message);
            event_buffer_.pop();
        }
        
        // Write to log file
        if (log_file_.is_open()) {
            log_file_ << data.to_json() << "\n";
            log_file_.flush();
        }
        
        last_log_timestamp_ = current_timestamp;
        
        // Console output for monitoring
        std::cout << "📊 Telemetry: t=" << data.t 
                  << ", fitness=" << std::fixed << std::setprecision(3) << genome.fitness
                  << ", entropy=" << metrics.entropy_mid
                  << ", success=" << metrics.success_mid
                  << ", events=" << data.events.size() << "\n";
    }
    
    // Log events
    void log_event(TelemetryEventType type, const std::string& message, 
                   const std::unordered_map<std::string, std::string>& metadata = {}) {
        TelemetryEvent event(type, message);
        event.metadata = metadata;
        
        // Add to buffer
        event_buffer_.push(event);
        
        // Limit buffer size
        if (event_buffer_.size() > max_buffer_size_) {
            event_buffer_.pop();
        }
        
        // Console output for important events
        std::cout << "📊 Event: " << event_type_to_string(type) << " - " << message << "\n";
    }
    
    // Specific event loggers
    void log_evolution_triggered(const std::string& reason) {
        std::unordered_map<std::string, std::string> metadata;
        metadata["reason"] = reason;
        log_event(TelemetryEventType::EVOLUTION_TRIGGERED, "evolution_triggered:true", metadata);
    }
    
    void log_sleep_cycle() {
        log_event(TelemetryEventType::SLEEP_CYCLE, "sleep:true");
    }
    
    void log_guardrail_alert(const std::string& alert_type, const std::string& details) {
        std::unordered_map<std::string, std::string> metadata;
        metadata["alert_type"] = alert_type;
        metadata["details"] = details;
        log_event(TelemetryEventType::GUARDRAIL_ALERT, "guardrail_alert:" + alert_type, metadata);
    }
    
    void log_champion_promoted(const GenomeSnapshot& new_champion) {
        std::unordered_map<std::string, std::string> metadata;
        metadata["fitness"] = std::to_string(new_champion.fitness);
        metadata["generation"] = std::to_string(new_champion.generation);
        metadata["genome_hash"] = new_champion.hash;
        log_event(TelemetryEventType::CHAMPION_PROMOTED, "champion_promoted:true", metadata);
    }
    
    void log_genome_blacklisted(const std::string& genome_hash, const std::string& reason) {
        std::unordered_map<std::string, std::string> metadata;
        metadata["genome_hash"] = genome_hash;
        metadata["reason"] = reason;
        log_event(TelemetryEventType::GENOME_BLACKLISTED, "genome_blacklisted:true", metadata);
    }
    
    void log_evaluation_complete(const std::string& evaluation_type, float overall_score) {
        std::unordered_map<std::string, std::string> metadata;
        metadata["evaluation_type"] = evaluation_type;
        metadata["overall_score"] = std::to_string(overall_score);
        log_event(TelemetryEventType::EVALUATION_COMPLETE, "evaluation_complete:" + evaluation_type, metadata);
    }
    
    void log_parameter_adjustment(const std::string& parameter, float old_value, float new_value) {
        std::unordered_map<std::string, std::string> metadata;
        metadata["parameter"] = parameter;
        metadata["old_value"] = std::to_string(old_value);
        metadata["new_value"] = std::to_string(new_value);
        metadata["delta"] = std::to_string(new_value - old_value);
        log_event(TelemetryEventType::PARAMETER_ADJUSTMENT, "parameter_adjustment:" + parameter, metadata);
    }
    
    void log_task_mode_switch(const std::string& old_mode, const std::string& new_mode) {
        std::unordered_map<std::string, std::string> metadata;
        metadata["old_mode"] = old_mode;
        metadata["new_mode"] = new_mode;
        log_event(TelemetryEventType::TASK_MODE_SWITCH, "task_mode_switch:" + new_mode, metadata);
    }
    
    // Configuration
    void set_log_interval(uint64_t interval_ms) {
        log_interval_ms_ = interval_ms;
    }
    
    void set_max_buffer_size(size_t max_size) {
        max_buffer_size_ = max_size;
    }
    
    // Get current buffer size
    size_t get_buffer_size() const {
        return event_buffer_.size();
    }
    
    // Force flush events
    void flush_events() {
        // This would be called to force immediate logging of buffered events
        std::cout << "📊 Flushing " << event_buffer_.size() << " buffered events\n";
    }
    
private:
    std::string event_type_to_string(TelemetryEventType type) const {
        switch (type) {
            case TelemetryEventType::EVOLUTION_TRIGGERED: return "EVOLUTION_TRIGGERED";
            case TelemetryEventType::SLEEP_CYCLE: return "SLEEP_CYCLE";
            case TelemetryEventType::GUARDRAIL_ALERT: return "GUARDRAIL_ALERT";
            case TelemetryEventType::CHAMPION_PROMOTED: return "CHAMPION_PROMOTED";
            case TelemetryEventType::GENOME_BLACKLISTED: return "GENOME_BLACKLISTED";
            case TelemetryEventType::EVALUATION_COMPLETE: return "EVALUATION_COMPLETE";
            case TelemetryEventType::PARAMETER_ADJUSTMENT: return "PARAMETER_ADJUSTMENT";
            case TelemetryEventType::TASK_MODE_SWITCH: return "TASK_MODE_SWITCH";
            default: return "UNKNOWN";
        }
    }
};

// Forward declaration for TelemetryData
struct TelemetryData {
    uint64_t t = 0;
    GenomeSnapshot genome;
    MetricSnapshot metrics;
    std::vector<std::string> events;
};

// ==================== TELEMETRY ANALYZER ====================

class TelemetryAnalyzer {
private:
    std::string log_filename_;
    std::vector<TelemetryData> telemetry_history_;
    
public:
    TelemetryAnalyzer(const std::string& filename = "melvin_telemetry.jsonl") 
        : log_filename_(filename) {}
    
    // Load telemetry history from file
    bool load_telemetry_history() {
        std::ifstream file(log_filename_);
        if (!file.is_open()) {
            std::cerr << "Failed to open telemetry file: " << log_filename_ << "\n";
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Parse JSON line (simplified - in practice would use proper JSON parser)
            // For now, just store the raw data
            // TelemetryData data;
            // ... JSON parsing would go here ...
            // telemetry_history_.push_back(data);
        }
        
        std::cout << "📊 Loaded " << telemetry_history_.size() << " telemetry entries\n";
        return true;
    }
    
    // Analyze trends
    struct TrendAnalysis {
        float fitness_trend = 0.0f;
        float entropy_trend = 0.0f;
        float success_trend = 0.0f;
        int evolution_cycles = 0;
        int guardrail_alerts = 0;
        std::string summary;
    };
    
    TrendAnalysis analyze_trends(size_t window_size = 100) const {
        TrendAnalysis analysis;
        
        if (telemetry_history_.size() < window_size) {
            analysis.summary = "Insufficient data for trend analysis";
            return analysis;
        }
        
        // Analyze last window_size entries
        size_t start_idx = telemetry_history_.size() - window_size;
        
        // Calculate trends
        float fitness_sum = 0.0f, entropy_sum = 0.0f, success_sum = 0.0f;
        
        for (size_t i = start_idx; i < telemetry_history_.size(); ++i) {
            const auto& data = telemetry_history_[i];
            fitness_sum += data.genome.fitness;
            entropy_sum += data.metrics.entropy_mid;
            success_sum += data.metrics.success_mid;
            
            // Count events
            for (const auto& event : data.events) {
                if (event.find("evolution_triggered") != std::string::npos) {
                    analysis.evolution_cycles++;
                }
                if (event.find("guardrail_alert") != std::string::npos) {
                    analysis.guardrail_alerts++;
                }
            }
        }
        
        size_t count = telemetry_history_.size() - start_idx;
        analysis.fitness_trend = fitness_sum / count;
        analysis.entropy_trend = entropy_sum / count;
        analysis.success_trend = success_sum / count;
        
        // Generate summary
        std::stringstream ss;
        ss << "Trend Analysis (last " << window_size << " entries):\n";
        ss << "  Fitness: " << std::fixed << std::setprecision(3) << analysis.fitness_trend << "\n";
        ss << "  Entropy: " << analysis.entropy_trend << "\n";
        ss << "  Success: " << analysis.success_trend << "\n";
        ss << "  Evolution cycles: " << analysis.evolution_cycles << "\n";
        ss << "  Guardrail alerts: " << analysis.guardrail_alerts;
        analysis.summary = ss.str();
        
        return analysis;
    }
    
    // Get telemetry history
    const std::vector<TelemetryData>& get_telemetry_history() const {
        return telemetry_history_;
    }
};

} // namespace melvin
