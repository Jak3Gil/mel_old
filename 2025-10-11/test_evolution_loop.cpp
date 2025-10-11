/*
 * MELVIN CONTINUOUS LEARNING + EVOLUTION PIPELINE
 * 
 * Test system to benchmark adaptive evolution on sample data.
 * Demonstrates the complete pipeline in action.
 */

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include "melvin_scheduler.h"
#include "melvin_metrics.h"
#include "melvin_evolution.h"
#include "predictive_sampler.h"

using namespace melvin;

// ==================== TEST DATA GENERATION ====================

class TestDataGenerator {
private:
    std::mt19937 rng_;
    std::vector<std::string> vocabulary_;
    std::vector<std::vector<std::string>> sentence_templates_;
    
public:
    TestDataGenerator() : rng_(std::random_device{}()) {
        initialize_vocabulary();
        initialize_sentence_templates();
    }
    
    std::vector<std::string> generate_text_batch(size_t count) {
        std::vector<std::string> batch;
        
        for (size_t i = 0; i < count; ++i) {
            batch.push_back(generate_sentence());
        }
        
        return batch;
    }
    
    std::vector<std::string> generate_learning_sequence() {
        // Generate a sequence that should trigger learning
        return {
            "cats are mammals",
            "mammals drink water",
            "cats drink water",
            "dogs are mammals",
            "dogs drink water",
            "animals need water",
            "cats are animals",
            "water is essential"
        };
    }
    
private:
    void initialize_vocabulary() {
        vocabulary_ = {
            "cats", "dogs", "birds", "fish", "mammals", "animals",
            "water", "food", "sleep", "play", "run", "eat", "drink",
            "are", "is", "have", "can", "will", "should", "must",
            "the", "a", "an", "and", "or", "but", "because", "if"
        };
    }
    
    void initialize_sentence_templates() {
        sentence_templates_ = {
            {"the", "animal", "drinks", "water"},
            {"mammals", "are", "animals"},
            {"cats", "are", "mammals"},
            {"dogs", "are", "mammals"},
            {"animals", "need", "water"},
            {"water", "is", "essential"},
            {"mammals", "have", "fur"},
            {"cats", "like", "to", "play"}
        };
    }
    
    std::string generate_sentence() {
        const auto& template_words = sentence_templates_[rng_() % sentence_templates_.size()];
        std::string sentence;
        
        for (size_t i = 0; i < template_words.size(); ++i) {
            if (i > 0) sentence += " ";
            
            if (template_words[i] == "animal") {
                sentence += vocabulary_[rng_() % 6];  // Random animal
            } else if (template_words[i] == "mammals") {
                sentence += "mammals";
            } else {
                sentence += template_words[i];
            }
        }
        
        return sentence;
    }
};

// ==================== EVOLUTION BENCHMARK ====================

class EvolutionBenchmark {
private:
    MelvinScheduler scheduler_;
    TestDataGenerator data_generator_;
    
public:
    EvolutionBenchmark() {
        // Configure scheduler for testing
        SchedulerConfig config;
        config.metrics_check_interval = std::chrono::milliseconds(500);
        config.consolidation_interval = std::chrono::minutes(5);
        config.evolution_check_interval = std::chrono::minutes(2);
        config.enable_continuous_learning = true;
        config.enable_memory_consolidation = true;
        config.enable_evolution = true;
        
        scheduler_.update_config(config);
    }
    
    void run_benchmark() {
        std::cout << "🧪 Starting Melvin Evolution Benchmark\n";
        std::cout << "=====================================\n\n";
        
        // Start the scheduler
        scheduler_.start();
        
        // Phase 1: Initial Learning (5 minutes)
        std::cout << "📚 Phase 1: Initial Learning (5 minutes)\n";
        run_learning_phase(300);  // 5 minutes
        
        // Phase 2: Adaptation Test (3 minutes)
        std::cout << "\n🧬 Phase 2: Adaptation Test (3 minutes)\n";
        run_adaptation_phase(180);  // 3 minutes
        
        // Phase 3: Evolution Stress Test (5 minutes)
        std::cout << "\n🚀 Phase 3: Evolution Stress Test (5 minutes)\n";
        run_stress_test_phase(300);  // 5 minutes
        
        // Final report
        std::cout << "\n📊 Final Status Report:\n";
        scheduler_.get_status();
        
        // Stop scheduler
        scheduler_.stop();
        
        std::cout << "\n🎉 Benchmark complete!\n";
    }
    
private:
    void run_learning_phase(int duration_seconds) {
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration_seconds);
        
        while (std::chrono::steady_clock::now() < end_time) {
            // Generate learning sequence
            auto learning_data = data_generator_.generate_learning_sequence();
            
            for (const auto& sentence : learning_data) {
                scheduler_.add_text_data(sentence);
            }
            
            // Small delay between batches
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Show progress
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed.count() % 30000000000 == 0) {  // Every 30 seconds
                std::cout << "   Learning progress: " 
                          << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
                          << "s / " << duration_seconds << "s\n";
            }
        }
        
        std::cout << "   ✅ Initial learning phase complete\n";
    }
    
    void run_adaptation_phase(int duration_seconds) {
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration_seconds);
        
        // Trigger manual evolution to test adaptation
        scheduler_.trigger_evolution();
        
        while (std::chrono::steady_clock::now() < end_time) {
            // Generate varied data to test adaptation
            auto batch = data_generator_.generate_text_batch(10);
            
            for (const auto& text : batch) {
                scheduler_.add_text_data(text);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed.count() % 60000000000 == 0) {  // Every 60 seconds
                std::cout << "   Adaptation progress: " 
                          << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
                          << "s / " << duration_seconds << "s\n";
                scheduler_.get_status();
            }
        }
        
        std::cout << "   ✅ Adaptation phase complete\n";
    }
    
    void run_stress_test_phase(int duration_seconds) {
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration_seconds);
        
        // Rapid data injection to stress the system
        while (std::chrono::steady_clock::now() < end_time) {
            // Large batch of diverse data
            auto batch = data_generator_.generate_text_batch(50);
            
            for (const auto& text : batch) {
                scheduler_.add_text_data(text);
            }
            
            // Trigger consolidation frequently
            scheduler_.trigger_consolidation();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed.count() % 60000000000 == 0) {  // Every 60 seconds
                std::cout << "   Stress test progress: " 
                          << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
                          << "s / " << duration_seconds << "s\n";
            }
        }
        
        std::cout << "   ✅ Stress test phase complete\n";
    }
};

// ==================== SIMPLE EVOLUTION DEMO ====================

void run_simple_evolution_demo() {
    std::cout << "🧬 Simple Evolution Demo\n";
    std::cout << "=======================\n\n";
    
    // Create evolution engine
    EvolutionEngine evolution_engine;
    
    // Create metrics collector
    MetricsCollector metrics_collector;
    
    // Simulate some reasoning steps
    std::cout << "📊 Simulating reasoning steps...\n";
    
    for (int step = 0; step < 100; ++step) {
        // Create fake candidates
        std::vector<Candidate> candidates;
        candidates.emplace_back(1, 0.6f, Rel::CONSUMES, 0.8f);
        candidates.emplace_back(2, 0.3f, Rel::ISA, 0.6f);
        candidates.emplace_back(3, 0.1f, Rel::TEMPORAL, 0.3f);
        
        // Record metrics
        bool success = (step % 10 != 0);  // 90% success rate
        bool reused_edge = (step % 3 == 0);
        bool thought_replay = (step % 20 == 0);
        
        metrics_collector.record_reasoning_step(candidates, success, reused_edge, thought_replay);
        
        // Show progress
        if (step % 20 == 0) {
            std::cout << "   Step " << step << ": " << metrics_collector.get_current_metrics().get_summary() << "\n";
        }
    }
    
    // Check if evolution should be triggered
    const auto& metrics = metrics_collector.get_current_metrics();
    
    std::cout << "\n📊 Final metrics: " << metrics.get_summary() << "\n";
    
    if (metrics_collector.should_trigger_evolution()) {
        std::cout << "🚨 Evolution trigger conditions met!\n";
        
        // Run evolution
        auto new_genome = evolution_engine.evolve(metrics);
        std::cout << "🧬 New genome: " << new_genome.get_summary() << "\n";
        
        // Show evolution stats
        auto stats = evolution_engine.get_stats();
        std::cout << "📈 Evolution stats: " << stats.total_generations 
                  << " generations, best fitness: " << stats.best_fitness << "\n";
    } else {
        std::cout << "✅ No evolution needed - parameters are optimal\n";
    }
}

// ==================== MAIN FUNCTION ====================

int main() {
    std::cout << "🧠 MELVIN CONTINUOUS LEARNING + EVOLUTION PIPELINE\n";
    std::cout << "=================================================\n\n";
    
    std::cout << "This system demonstrates:\n";
    std::cout << "🧩 Continuous Learning - Real-time data stream processing\n";
    std::cout << "🧠 Memory Consolidation - Thought node replay and edge decay\n";
    std::cout << "📊 Metrics Monitoring - Entropy, success rate, coherence drift\n";
    std::cout << "🧬 Evolution Cycles - Parameter genome mutation and selection\n";
    std::cout << "⚙️  Adaptive Tuning - Homeostatic parameter adjustment\n\n";
    
    std::cout << "Choose demo mode:\n";
    std::cout << "1. Simple Evolution Demo (quick)\n";
    std::cout << "2. Full Benchmark Test (13 minutes)\n";
    std::cout << "Enter choice (1 or 2): ";
    
    int choice;
    std::cin >> choice;
    
    if (choice == 1) {
        run_simple_evolution_demo();
    } else if (choice == 2) {
        EvolutionBenchmark benchmark;
        benchmark.run_benchmark();
    } else {
        std::cout << "Invalid choice. Running simple demo...\n\n";
        run_simple_evolution_demo();
    }
    
    std::cout << "\n🎉 Demo complete! Melvin now has a continuously learning and evolving brain!\n";
    
    return 0;
}
