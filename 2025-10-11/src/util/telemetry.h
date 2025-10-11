/*
 * MELVIN TELEMETRY - Event Logging & Performance Tracking
 * 
 * Provides:
 * - Event logging with structured key-value pairs
 * - Scoped timers for performance measurement
 * - JSON output for analysis
 */

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

namespace melvin_telemetry {

using EventMap = std::unordered_map<std::string, std::string>;
using TimePoint = std::chrono::high_resolution_clock::time_point;

// ==================== EVENT LOGGING ====================

struct Event {
    std::string tag;
    EventMap data;
    TimePoint timestamp;
    
    Event(const std::string& tag, const EventMap& data)
        : tag(tag), data(data), timestamp(std::chrono::high_resolution_clock::now()) {}
    
    std::string to_json() const {
        std::stringstream ss;
        ss << "{\"tag\":\"" << tag << "\",\"ts\":" 
           << std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count()
           << ",\"data\":{";
        
        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) ss << ",";
            ss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        
        ss << "}}";
        return ss.str();
    }
};

// Global event log
static std::vector<Event> event_log;
static bool telemetry_enabled = true;
static std::string telemetry_file = "melvin_telemetry.jsonl";

inline void enable_telemetry(bool enabled = true) {
    telemetry_enabled = enabled;
}

inline void set_telemetry_file(const std::string& file) {
    telemetry_file = file;
}

inline void log_event(const std::string& tag, const EventMap& data) {
    if (!telemetry_enabled) return;
    
    Event event(tag, data);
    event_log.push_back(event);
    
    // Also write to file immediately
    std::ofstream out(telemetry_file, std::ios::app);
    if (out) {
        out << event.to_json() << "\n";
    }
}

inline void flush_telemetry() {
    std::ofstream out(telemetry_file);
    if (out) {
        for (const auto& event : event_log) {
            out << event.to_json() << "\n";
        }
    }
}

inline void clear_telemetry() {
    event_log.clear();
}

inline const std::vector<Event>& get_events() {
    return event_log;
}

inline int count_events(const std::string& tag) {
    int count = 0;
    for (const auto& event : event_log) {
        if (event.tag == tag) count++;
    }
    return count;
}

// ==================== SCOPED TIMER ====================

class ScopedTimer {
private:
    std::string name_;
    TimePoint start_;
    bool logged_;
    
public:
    ScopedTimer(const std::string& name) 
        : name_(name), start_(std::chrono::high_resolution_clock::now()), logged_(false) {}
    
    ~ScopedTimer() {
        if (!logged_) {
            stop();
        }
    }
    
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
    void stop() {
        if (logged_) return;
        
        double ms = elapsed_ms();
        
        std::stringstream ss;
        ss << ms;
        
        log_event("timer", {
            {"name", name_},
            {"duration_ms", ss.str()}
        });
        
        logged_ = true;
    }
};

// ==================== CONVENIENCE MACROS ====================

#define LOG_LEAP_CREATE(subj, conn, pred, support, diversity) \
    melvin_telemetry::log_event("leap_create", { \
        {"subject", subj}, \
        {"connector", conn}, \
        {"predicate", pred}, \
        {"support", support}, \
        {"diversity", diversity} \
    })

#define LOG_LEAP_PROMOTE(subj, pred, weight, successes) \
    melvin_telemetry::log_event("leap_promote", { \
        {"subject", subj}, \
        {"predicate", pred}, \
        {"weight", weight}, \
        {"successes", successes} \
    })

#define LOG_LEAP_DECAY(subj, pred, reason) \
    melvin_telemetry::log_event("leap_decay", { \
        {"subject", subj}, \
        {"predicate", pred}, \
        {"reason", reason} \
    })

#define LOG_LEAP_REJECT(subj, conn, reason, support, diversity, margin) \
    melvin_telemetry::log_event("leap_reject", { \
        {"subject", subj}, \
        {"connector", conn}, \
        {"reason", reason}, \
        {"support", support}, \
        {"diversity", diversity}, \
        {"margin", margin} \
    })

#define LOG_CONFLICT_DETECTED(subj, conn, old_pred, new_pred) \
    melvin_telemetry::log_event("conflict_detected", { \
        {"subject", subj}, \
        {"connector", conn}, \
        {"old_predicate", old_pred}, \
        {"new_predicate", new_pred} \
    })

#define LOG_ABSTAIN(query, reason) \
    melvin_telemetry::log_event("abstain", { \
        {"query", query}, \
        {"reason", reason} \
    })

#define SCOPED_TIMER(name) melvin_telemetry::ScopedTimer _timer_##__LINE__(name)

} // namespace melvin_telemetry

