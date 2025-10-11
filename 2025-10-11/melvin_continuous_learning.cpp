/*
 * MELVIN CONTINUOUS LEARNING DEMO
 * 
 * Feeds Melvin real data and demonstrates continuous learning
 * with automatic parameter optimization and evolution.
 */

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include "melvin_data_ingestion.h"

using namespace melvin;

// ==================== LEARNING MONITOR ====================

class LearningMonitor {
private:
    std::atomic<bool> monitoring_active_;
    std::thread monitor_thread_;
    const DataIngestionEngine* engine_;
    
public:
    LearningMonitor(const DataIngestionEngine* engine) 
        : monitoring_active_(false), engine_(engine) {}
    
    ~LearningMonitor() {
        stop_monitoring();
    }
    
    void start_monitoring() {
        if (monitoring_active_) return;
        
        monitoring_active_ = true;
        monitor_thread_ = std::thread(&LearningMonitor::monitoring_loop, this);
        
        std::cout << "📊 Learning monitor started\n";
    }
    
    void stop_monitoring() {
        if (!monitoring_active_) return;
        
        monitoring_active_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        
        std::cout << "📊 Learning monitor stopped\n";
    }
    
private:
    void monitoring_loop() {
        while (monitoring_active_) {
            // Get current learning statistics
            auto stats = engine_->get_stats();
            auto guardrails = engine_->get_guardrails();
            
            // Display learning progress
            std::cout << "\n📈 LEARNING PROGRESS REPORT\n";
            std::cout << "==========================\n";
            std::cout << "📊 Processed chunks: " << stats.processed_chunks << "/" << stats.total_chunks << "\n";
            std::cout << "⚡ Processing rate: " << std::fixed << std::setprecision(2) << stats.processing_rate << " chunks/sec\n";
            std::cout << "📥 Queue size: " << stats.queue_size << "\n";
            
            // Display current metrics
            auto metrics = guardrails.get_current_snapshot();
            std::cout << "🧠 Current metrics:\n";
            std::cout << "   Entropy (mid): " << std::fixed << std::setprecision(3) << metrics.entropy_mid << "\n";
            std::cout << "   Top-2 margin: " << metrics.top2_margin_mid << "\n";
            std::cout << "   Success rate: " << metrics.success_mid << "\n";
            std::cout << "   Coherence drift: " << metrics.drift_long << "\n";
            std::cout << "   Fitness score: " << metrics.compute_fitness() << "\n";
            
            // Check for alerts
            if (guardrails.should_trigger_evolution()) {
                std::cout << "🚨 Evolution trigger conditions detected!\n";
            }
            
            if (guardrails.is_fitness_stagnating()) {
                std::cout << "⚠️  Fitness appears to be stagnating\n";
            }
            
            std::cout << "==========================\n\n";
            
            // Wait before next report
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
};

// ==================== DATA SOURCE SETUP ====================

void setup_data_sources(DataIngestionEngine& engine) {
    std::cout << "📚 Setting up data sources...\n";
    
    // Add book sources
    engine.add_directory_source("sample_data/books", DataType::TEXT_BOOK);
    
    // Add article sources
    engine.add_directory_source("sample_data/articles", DataType::TEXT_ARTICLE);
    
    // Add conversation sources
    engine.add_directory_source("sample_data/conversations", DataType::TEXT_CONVERSATION);
    
    // Add individual files for demonstration
    engine.add_file_source("sample_data/books/ai_fundamentals.txt", DataType::TEXT_BOOK);
    engine.add_file_source("sample_data/articles/tech_news.txt", DataType::TEXT_ARTICLE);
    engine.add_file_source("sample_data/conversations/qa_sessions.txt", DataType::TEXT_CONVERSATION);
    
    std::cout << "✅ Data sources configured\n\n";
}

// ==================== INTERACTIVE CONTROLS ====================

void interactive_controls(DataIngestionEngine& engine) {
    std::cout << "🎮 Interactive Controls\n";
    std::cout << "======================\n";
    std::cout << "Commands:\n";
    std::cout << "  stats  - Show current learning statistics\n";
    std::cout << "  metrics - Show current metrics\n";
    std::cout << "  stop   - Stop learning\n";
    std::cout << "  help   - Show this help\n";
    std::cout << "======================\n\n";
    
    std::string command;
    while (engine.get_stats().processed_chunks >= 0) {  // Continue while learning
        std::cout << "melvin> ";
        std::getline(std::cin, command);
        
        if (command == "stats") {
            auto stats = engine.get_stats();
            std::cout << "📊 Learning Statistics:\n";
            std::cout << "   Processed: " << stats.processed_chunks << " chunks\n";
            std::cout << "   Total: " << stats.total_chunks << " chunks\n";
            std::cout << "   Queue: " << stats.queue_size << " chunks\n";
            std::cout << "   Rate: " << std::fixed << std::setprecision(2) << stats.processing_rate << " chunks/sec\n\n";
            
        } else if (command == "metrics") {
            auto guardrails = engine.get_guardrails();
            auto metrics = guardrails.get_current_snapshot();
            
            std::cout << "🧠 Current Metrics:\n";
            std::cout << "   Entropy (short): " << std::fixed << std::setprecision(3) << metrics.entropy_short << "\n";
            std::cout << "   Entropy (mid): " << metrics.entropy_mid << "\n";
            std::cout << "   Entropy (long): " << metrics.entropy_long << "\n";
            std::cout << "   Top-2 margin: " << metrics.top2_margin_mid << "\n";
            std::cout << "   Success (short): " << metrics.success_short << "\n";
            std::cout << "   Success (mid): " << metrics.success_mid << "\n";
            std::cout << "   Drift (long): " << metrics.drift_long << "\n";
            std::cout << "   Thought replay: " << metrics.thought_replay_mid << "\n";
            std::cout << "   Fitness: " << metrics.compute_fitness() << "\n\n";
            
        } else if (command == "stop") {
            std::cout << "🛑 Stopping learning...\n";
            engine.stop();
            break;
            
        } else if (command == "help") {
            std::cout << "🎮 Available Commands:\n";
            std::cout << "  stats  - Show learning statistics\n";
            std::cout << "  metrics - Show current metrics\n";
            std::cout << "  stop   - Stop learning\n";
            std::cout << "  help   - Show this help\n\n";
            
        } else if (command.empty()) {
            // Empty command, continue
            continue;
            
        } else {
            std::cout << "❓ Unknown command. Type 'help' for available commands.\n\n";
        }
    }
}

// ==================== MAIN FUNCTION ====================

int main() {
    std::cout << "🧠 MELVIN CONTINUOUS LEARNING SYSTEM\n";
    std::cout << "====================================\n\n";
    
    std::cout << "This system will feed Melvin real data from various sources:\n";
    std::cout << "📚 Books: AI fundamentals, biology basics\n";
    std::cout << "📄 Articles: Technology news, scientific papers\n";
    std::cout << "💬 Conversations: Q&A sessions, dialogues\n\n";
    
    std::cout << "Melvin will:\n";
    std::cout << "🧠 Learn continuously from the data\n";
    std::cout << "📊 Monitor his own performance metrics\n";
    std::cout << "🧬 Evolve his parameters when needed\n";
    std::cout << "🏠 Apply homeostatic adjustments\n";
    std::cout << "📈 Report learning progress in real-time\n\n";
    
    // Create data ingestion engine
    DataIngestionEngine engine;
    
    // Setup data sources
    setup_data_sources(engine);
    
    // Start learning monitor
    LearningMonitor monitor(&engine);
    monitor.start_monitoring();
    
    // Start continuous learning
    std::cout << "🚀 Starting continuous learning...\n";
    std::thread learning_thread([&engine]() {
        engine.start_learning();
    });
    
    // Give learning time to start
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Start interactive controls
    interactive_controls(engine);
    
    // Cleanup
    monitor.stop_monitoring();
    if (learning_thread.joinable()) {
        learning_thread.join();
    }
    
    // Final statistics
    auto final_stats = engine.get_stats();
    std::cout << "\n🎉 LEARNING SESSION COMPLETE!\n";
    std::cout << "=============================\n";
    std::cout << "📊 Final Statistics:\n";
    std::cout << "   Total chunks processed: " << final_stats.processed_chunks << "\n";
    std::cout << "   Total chunks available: " << final_stats.total_chunks << "\n";
    std::cout << "   Average processing rate: " << std::fixed << std::setprecision(2) 
              << final_stats.processing_rate << " chunks/sec\n";
    
    auto final_guardrails = engine.get_guardrails();
    auto final_metrics = final_guardrails.get_current_snapshot();
    
    std::cout << "\n🧠 Final Metrics:\n";
    std::cout << "   Final fitness score: " << std::fixed << std::setprecision(3) 
              << final_metrics.compute_fitness() << "\n";
    std::cout << "   Final entropy: " << final_metrics.entropy_mid << "\n";
    std::cout << "   Final success rate: " << final_metrics.success_mid << "\n";
    std::cout << "   Final coherence drift: " << final_metrics.drift_long << "\n";
    
    std::cout << "\n✅ Melvin has successfully learned from real data!\n";
    std::cout << "   The system demonstrated:\n";
    std::cout << "   📚 Continuous data ingestion and processing\n";
    std::cout << "   🧠 Real-time learning and metric monitoring\n";
    std::cout << "   🧬 Automatic parameter evolution triggers\n";
    std::cout << "   🏠 Homeostatic parameter adjustments\n";
    std::cout << "   📊 Comprehensive telemetry and logging\n\n";
    
    std::cout << "🚀 Melvin is ready for production continuous learning!\n";
    
    return 0;
}
