/**
 * @file service_base.h
 * @brief Base class for all cognitive services
 * 
 * Each service runs at a fixed frequency with a budget
 */

#ifndef MELVIN_SERVICE_BASE_H
#define MELVIN_SERVICE_BASE_H

#include "event_bus.h"
#include <string>
#include <atomic>
#include <chrono>

namespace melvin {
namespace cognitive_os {

/**
 * @brief Service statistics
 */
struct ServiceStats {
    uint64_t ticks_completed;
    uint64_t budget_overruns;
    double avg_tick_time_ms;
    double max_tick_time_ms;
    double cpu_usage;
    bool is_running;
};

/**
 * @brief Base class for cognitive services
 */
class ServiceBase {
public:
    ServiceBase(const std::string& name, float frequency_hz, EventBus* bus) :
        name_(name),
        frequency_hz_(frequency_hz),
        period_ms_(1000.0f / frequency_hz),
        budget_ms_(period_ms_ * 0.8f),  // 80% of period by default
        bus_(bus),
        running_(false),
        ticks_(0),
        overruns_(0)
    {}
    
    virtual ~ServiceBase() = default;
    
    /**
     * @brief Initialize service (called once)
     */
    virtual void initialize() = 0;
    
    /**
     * @brief Main service tick (called at frequency_hz)
     * 
     * @param budget_ms Allocated time budget in milliseconds
     * @return Actual time used in milliseconds
     */
    virtual double tick(float budget_ms) = 0;
    
    /**
     * @brief Shutdown service (called once)
     */
    virtual void shutdown() {}
    
    /**
     * @brief Get service name
     */
    const std::string& name() const { return name_; }
    
    /**
     * @brief Get target frequency
     */
    float frequency() const { return frequency_hz_; }
    
    /**
     * @brief Get period in ms
     */
    float period() const { return period_ms_; }
    
    /**
     * @brief Set/get budget
     */
    void set_budget(float budget_ms) { budget_ms_ = budget_ms; }
    float budget() const { return budget_ms_; }
    
    /**
     * @brief Control running state
     */
    void start() { running_ = true; }
    void stop() { running_ = false; }
    bool is_running() const { return running_.load(std::memory_order_relaxed); }
    
    /**
     * @brief Get statistics
     */
    ServiceStats stats() const {
        ServiceStats s;
        s.ticks_completed = ticks_.load(std::memory_order_relaxed);
        s.budget_overruns = overruns_.load(std::memory_order_relaxed);
        s.avg_tick_time_ms = avg_tick_time_;
        s.max_tick_time_ms = max_tick_time_;
        s.cpu_usage = (avg_tick_time_ / period_ms_) * 100.0f;
        s.is_running = running_.load(std::memory_order_relaxed);
        return s;
    }
    
protected:
    std::string name_;
    float frequency_hz_;
    float period_ms_;
    float budget_ms_;
    EventBus* bus_;
    
    std::atomic<bool> running_;
    std::atomic<uint64_t> ticks_;
    std::atomic<uint64_t> overruns_;
    
    double avg_tick_time_;
    double max_tick_time_;
    
    /**
     * @brief Update timing statistics
     */
    void update_stats(double tick_time_ms) {
        ticks_.fetch_add(1, std::memory_order_relaxed);
        
        if (tick_time_ms > budget_ms_) {
            overruns_.fetch_add(1, std::memory_order_relaxed);
        }
        
        // Exponential moving average
        const float alpha = 0.1f;
        avg_tick_time_ = alpha * tick_time_ms + (1.0f - alpha) * avg_tick_time_;
        
        if (tick_time_ms > max_tick_time_) {
            max_tick_time_ = tick_time_ms;
        }
    }
};

} // namespace cognitive_os
} // namespace melvin

#endif // MELVIN_SERVICE_BASE_H

