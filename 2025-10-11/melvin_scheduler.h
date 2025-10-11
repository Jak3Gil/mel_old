/*
 * MELVIN CONTINUOUS LEARNING + EVOLUTION PIPELINE
 * 
 * Main scheduler that orchestrates continuous learning, memory consolidation,
 * metrics monitoring, and evolution cycles.
 */

#pragma once

#include <thread>
#include <atomic>
#include <chrono>
#include <queue>
#include <mutex>
#include "melvin_types.h"
#include "melvin_metrics.h"
#include "melvin_evolution.h"
#include "melvin_memory.h"
#include "predictive_sampler.h"

namespace melvin {

// ==================== SCHEDULER CONFIGURATION ====================

struct SchedulerConfig {
    // Timing parameters
    std::chrono::milliseconds metrics_check_interval{1000};    // Check metrics every 1s
    std::chrono::minutes consolidation_interval{30};          // Consolidate every 30min
    std::chrono::minutes evolution_check_interval{60};        // Check evolution every 1hr
    
    // Evolution thresholds
    float entropy_threshold = 1.5f;          // Trigger if entropy > 1.5
    float success_rate_threshold = 0.6f;     // Trigger if success < 60%
    float drift_threshold = 0.25f;           // Trigger if drift > 0.25
    int stagnation_steps = 1000;             // Trigger if no improvement in 1k steps
    
    // Learning parameters
    size_t max_data_buffer_size = 10000;     // Max items in data buffer
    bool enable_continuous_learning = true;  // Enable data stream processing
    bool enable_memory_consolidation = true; // Enable memory consolidation
    bool enable_evolution = true;            // Enable evolution cycles
};

// ==================== DATA STREAM ITEM ====================

struct DataStreamItem {
    enum Type { TEXT, AUDIO, IMAGE, SENSOR };
    
    Type type;
    std::string data;
    std::vector<float> embeddings;
    std::chrono::system_clock::time_point timestamp;
    
    DataStreamItem(Type t, const std::string& d) 
        : type(t), data(d), timestamp(std::chrono::system_clock::now()) {}
};

// ==================== MAIN SCHEDULER ====================

class MelvinScheduler {
private:
    // Core components
    MetricsCollector metrics_collector_;
    EvolutionEngine evolution_engine_;
    ContinuousLearningPipeline learning_pipeline_;
    
    // Configuration
    SchedulerConfig config_;
    
    // Threading and synchronization
    std::atomic<bool> running_{false};
    std::thread scheduler_thread_;
    std::mutex data_mutex_;
    std::queue<DataStreamItem> data_queue_;
    
    // State tracking
    std::chrono::system_clock::time_point last_metrics_check_;
    std::chrono::system_clock::time_point last_consolidation_;
    std::chrono::system_clock::time_point last_evolution_check_;
    int steps_since_last_evolution_ = 0;
    
public:
    MelvinScheduler(const SchedulerConfig& config = SchedulerConfig{}) 
        : config_(config),
          last_metrics_check_(std::chrono::system_clock::now()),
          last_consolidation_(std::chrono::system_clock::now()),
          last_evolution_check_(std::chrono::system_clock::now()) {}
    
    ~MelvinScheduler() {
        stop();
    }
    
    // Start the continuous learning pipeline
    void start() {
        if (running_) return;
        
        running_ = true;
        scheduler_thread_ = std::thread(&MelvinScheduler::scheduler_loop, this);
        
        std::cout << "🚀 Melvin Continuous Learning Pipeline started\n";
        std::cout << "   - Metrics check: every " << config_.metrics_check_interval.count() << "ms\n";
        std::cout << "   - Consolidation: every " << config_.consolidation_interval.count() << "min\n";
        std::cout << "   - Evolution check: every " << config_.evolution_check_interval.count() << "min\n";
    }
    
    // Stop the pipeline
    void stop() {
        if (!running_) return;
        
        running_ = false;
        if (scheduler_thread_.joinable()) {
            scheduler_thread_.join();
        }
        
        std::cout << "🛑 Melvin Continuous Learning Pipeline stopped\n";
    }
    
    // Add data to the processing stream
    void add_data(const DataStreamItem& item) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        data_queue_.push(item);
    }
    
    void add_text_data(const std::string& text) {
        add_data(DataStreamItem(DataStreamItem::TEXT, text));
    }
    
    void add_audio_data(const std::vector<float>& audio_features) {
        // Convert audio features to string representation (simplified)
        std::string audio_str = "audio:" + std::to_string(audio_features.size()) + "features";
        add_data(DataStreamItem(DataStreamItem::AUDIO, audio_str));
    }
    
    void add_image_data(const std::vector<float>& image_embeddings) {
        // Convert image embeddings to string representation (simplified)
        std::string image_str = "image:" + std::to_string(image_embeddings.size()) + "embeddings";
        add_data(DataStreamItem(DataStreamItem::IMAGE, image_str));
    }
    
    // Manual evolution trigger
    void trigger_evolution() {
        std::cout << "🧬 Manual evolution trigger\n";
        perform_evolution_cycle();
    }
    
    // Manual memory consolidation
    void trigger_consolidation() {
        std::cout << "🧠 Manual memory consolidation\n";
        learning_pipeline_.get_consolidator().consolidate();
    }
    
    // Get current status
    void get_status() {
        const auto& metrics = metrics_collector_.get_current_metrics();
        const auto& genome = evolution_engine_.get_current_genome();
        const auto& stats = evolution_engine_.get_stats();
        
        std::cout << "\n📊 MELVIN STATUS REPORT\n";
        std::cout << "=======================\n";
        std::cout << "🧠 Metrics: " << metrics.get_summary() << "\n";
        std::cout << "🧬 Genome: " << genome.get_summary() << "\n";
        std::cout << "📈 Evolution: " << stats.total_generations << " generations, "
                  << "best fitness: " << std::fixed << std::setprecision(3) << stats.best_fitness << "\n";
        
        std::lock_guard<std::mutex> lock(data_mutex_);
        std::cout << "📥 Data queue: " << data_queue_.size() << " items pending\n";
        std::cout << "⏱️  Steps since evolution: " << steps_since_last_evolution_ << "\n";
        std::cout << "🔄 Pipeline running: " << (running_ ? "YES" : "NO") << "\n\n";
    }
    
    // Update configuration
    void update_config(const SchedulerConfig& new_config) {
        config_ = new_config;
        std::cout << "⚙️  Scheduler configuration updated\n";
    }
    
    // Get components for external access
    MetricsCollector& get_metrics_collector() { return metrics_collector_; }
    EvolutionEngine& get_evolution_engine() { return evolution_engine_; }
    ContinuousLearningPipeline& get_learning_pipeline() { return learning_pipeline_; }
    
private:
    // Main scheduler loop
    void scheduler_loop() {
        std::cout << "🔄 Starting scheduler loop...\n";
        
        while (running_) {
            auto now = std::chrono::system_clock::now();
            
            // Process data stream
            process_data_stream();
            
            // Check metrics periodically
            if (now - last_metrics_check_ >= config_.metrics_check_interval) {
                check_metrics();
                last_metrics_check_ = now;
            }
            
            // Trigger memory consolidation
            if (config_.enable_memory_consolidation && 
                now - last_consolidation_ >= config_.consolidation_interval) {
                learning_pipeline_.get_consolidator().consolidate();
                last_consolidation_ = now;
            }
            
            // Check evolution triggers
            if (config_.enable_evolution && 
                now - last_evolution_check_ >= config_.evolution_check_interval) {
                check_evolution_triggers();
                last_evolution_check_ = now;
            }
            
            // Small sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "🔄 Scheduler loop ended\n";
    }
    
    // Process incoming data stream
    void process_data_stream() {
        if (!config_.enable_continuous_learning) return;
        
        std::vector<std::string> batch_data;
        
        // Collect batch of data items
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            size_t batch_size = std::min(size_t(100), data_queue_.size());
            
            for (size_t i = 0; i < batch_size; ++i) {
                if (data_queue_.empty()) break;
                
                const auto& item = data_queue_.front();
                batch_data.push_back(item.data);
                data_queue_.pop();
            }
        }
        
        if (!batch_data.empty()) {
            learning_pipeline_.process_data_stream(batch_data);
        }
    }
    
    // Check current metrics and update tracking
    void check_metrics() {
        const auto& metrics = metrics_collector_.get_current_metrics();
        
        // Record metrics for evolution tracking
        steps_since_last_evolution_++;
        
        // Log metrics periodically
        static int log_counter = 0;
        if (++log_counter % 60 == 0) {  // Log every 60 checks (1 minute)
            metrics_collector_.log_status();
        }
    }
    
    // Check if evolution should be triggered
    void check_evolution_triggers() {
        const auto& metrics = metrics_collector_.get_current_metrics();
        
        bool should_evolve = false;
        std::string trigger_reason;
        
        // Check individual thresholds
        if (metrics.predictive.avg_traversal_entropy > config_.entropy_threshold) {
            should_evolve = true;
            trigger_reason = "high entropy (" + std::to_string(metrics.predictive.avg_traversal_entropy) + ")";
        }
        
        if (metrics.predictive.success_rate < config_.success_rate_threshold) {
            should_evolve = true;
            trigger_reason = "low success rate (" + std::to_string(metrics.predictive.success_rate) + ")";
        }
        
        if (metrics.cognitive.coherence_drift > config_.drift_threshold) {
            should_evolve = true;
            trigger_reason = "high drift (" + std::to_string(metrics.cognitive.coherence_drift) + ")";
        }
        
        if (steps_since_last_evolution_ > config_.stagnation_steps) {
            should_evolve = true;
            trigger_reason = "stagnation (" + std::to_string(steps_since_last_evolution_) + " steps)";
        }
        
        if (should_evolve) {
            std::cout << "🚨 Evolution trigger: " << trigger_reason << "\n";
            perform_evolution_cycle();
        }
    }
    
    // Perform evolution cycle
    void perform_evolution_cycle() {
        const auto& metrics = metrics_collector_.get_current_metrics();
        
        // Run evolution
        auto new_genome = evolution_engine_.evolve(metrics);
        
        // Reset tracking
        steps_since_last_evolution_ = 0;
        metrics_collector_.reset();
        
        // Save evolution history
        evolution_engine_.save_history("melvin_evolution_history.csv");
        
        std::cout << "🧬 Evolution cycle complete\n";
    }
};

// ==================== GLOBAL SCHEDULER INSTANCE ====================

// Global scheduler instance for easy access
extern std::unique_ptr<MelvinScheduler> g_scheduler;

// Initialize global scheduler
void initialize_scheduler(const SchedulerConfig& config = SchedulerConfig{});

// Shutdown global scheduler
void shutdown_scheduler();

// Get global scheduler instance
MelvinScheduler& get_scheduler();

} // namespace melvin
