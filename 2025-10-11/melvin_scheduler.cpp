/*
 * MELVIN CONTINUOUS LEARNING + EVOLUTION PIPELINE
 * 
 * Implementation of the main scheduler that orchestrates the entire system.
 */

#include "melvin_scheduler.h"
#include <iostream>
#include <iomanip>

namespace melvin {

// ==================== GLOBAL SCHEDULER INSTANCE ====================

std::unique_ptr<MelvinScheduler> g_scheduler = nullptr;

void initialize_scheduler(const SchedulerConfig& config) {
    if (g_scheduler) {
        std::cout << "⚠️  Scheduler already initialized\n";
        return;
    }
    
    g_scheduler = std::make_unique<MelvinScheduler>(config);
    std::cout << "🚀 Global Melvin scheduler initialized\n";
}

void shutdown_scheduler() {
    if (g_scheduler) {
        g_scheduler->stop();
        g_scheduler.reset();
        std::cout << "🛑 Global Melvin scheduler shutdown\n";
    }
}

MelvinScheduler& get_scheduler() {
    if (!g_scheduler) {
        throw std::runtime_error("Scheduler not initialized. Call initialize_scheduler() first.");
    }
    return *g_scheduler;
}

} // namespace melvin