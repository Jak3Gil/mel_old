#pragma once

#include "Genome.hpp"
#include "PersistentMemory.hpp"
#include <string>
#include <vector>
#include <chrono>

namespace melvin::evolution {

/**
 * Fitness Evaluator for Melvin Evolution
 * 
 * Evaluates genomes using correctness, speed, and creativity metrics
 */

struct TestCase {
    std::string teach_phrase;
    int repeat_count;
    std::string question;
    std::string expected_keywords;
    std::string query_type;
    
    TestCase(const std::string& teach, int repeat, const std::string& ask, 
             const std::string& expected, const std::string& type)
        : teach_phrase(teach), repeat_count(repeat), question(ask), 
          expected_keywords(expected), query_type(type) {}
};

class FitnessEvaluator {
public:
    // Evaluation configuration
    struct Config {
        bool use_mini_eval = true;           // Use mini eval harness (fast)
        bool use_agi_eval = false;           // Use full AGI test (slow)
        int agi_eval_frequency = 5;          // Run AGI eval every N generations
        
        // Test case weights
        double correctness_weight = 0.6;     // Primary weight for correctness
        double speed_weight = 0.25;          // Weight for response speed
        double creativity_weight = 0.15;     // Weight for response diversity
        double memory_health_weight = 0.1;   // Weight for memory health
        
        // Speed evaluation
        double max_response_time = 5.0;      // Max time per query (seconds)
        
        // Creativity evaluation
        size_t novelty_window = 10;          // Window for novelty calculation
        double diversity_threshold = 0.1;    // Threshold for unique responses
        
        // Safety checks
        bool enable_safety_checks = true;
        double min_correctness_threshold = 0.01;  // Minimum correctness to pass
        double max_abstain_rate = 0.8;       // Maximum abstain rate allowed
        
        // Memory management
        bool use_persistent_memory = true;   // Use shared memory across generations
        std::string memory_directory = "evolution_memory/";  // Directory for persistent memory
    };
    
private:
    Config config_;
    std::vector<TestCase> test_cases_;
    
    // Persistent memory manager
    std::unique_ptr<PersistentMemoryManager> memory_manager_;
    std::unique_ptr<MemoryEvaluationContext> evaluation_context_;
    
    // Evaluation statistics
    struct EvaluationStats {
        double correctness = 0.0;
        double speed = 0.0;
        double creativity = 0.0;
        double memory_health = 0.0;
        double integrity_penalty = 0.0;
        double clutter_penalty = 0.0;
        double total_fitness = 0.0;
        
        // Detailed metrics
        int total_tests = 0;
        int correct_tests = 0;
        double total_time = 0.0;
        double avg_response_time = 0.0;
        
        // Creativity metrics
        std::vector<std::string> responses;
        double uniqueness_ratio = 0.0;
        double response_entropy = 0.0;
        
        // Safety metrics
        int abstain_count = 0;
        double abstain_rate = 0.0;
        bool passed_safety_checks = true;
        
        // Memory metrics
        PersistentMemoryManager::MemoryStats memory_before;
        PersistentMemoryManager::MemoryStats memory_after;
    };
    
    // Response history for creativity evaluation
    std::vector<std::string> recent_responses_;
    
public:
    explicit FitnessEvaluator(const Config& config);
    
    // Main evaluation function
    void evaluate(Genome& genome);
    
    // Individual evaluation components
    double evaluateCorrectness(const Genome& genome);
    double evaluateSpeed(const Genome& genome);
    double evaluateCreativity(const Genome& genome);
    
    // Test case execution
    std::string executeTestCase(const Genome& genome, const TestCase& test_case);
    bool checkResponseCorrectness(const std::string& response, const std::string& expected_keywords);
    
    // Creativity and diversity metrics
    double calculateUniquenessRatio(const std::vector<std::string>& responses);
    double calculateResponseEntropy(const std::vector<std::string>& responses);
    
    // Safety checks
    bool performSafetyChecks(const EvaluationStats& stats);
    
    // Utility functions
    void initializeTestCases();
    void updateResponseHistory(const std::string& response);
    void clearResponseHistory();
    
    // Statistics and reporting
    void printEvaluationSummary(const Genome& genome, const EvaluationStats& stats);
    std::string generateEvaluationReport(const Genome& genome, const EvaluationStats& stats);
    
    // Configuration
    void setConfig(const Config& config) { config_ = config; }
    const Config& getConfig() const { return config_; }
    
    // Test case management
    void addTestCase(const TestCase& test_case);
    void clearTestCases() { test_cases_.clear(); }
    size_t getTestCaseCount() const { return test_cases_.size(); }
    
private:
    // Helper functions
    double calculateKeywordMatch(const std::string& response, const std::string& expected_keywords);
    std::vector<std::string> extractKeywords(const std::string& text);
    std::string toLower(const std::string& str);
    
    // Time measurement
    class Timer {
    public:
        Timer() : start_(std::chrono::high_resolution_clock::now()) {}
        
        double elapsed() const {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
            return duration.count() / 1000.0; // Convert to seconds
        }
        
    private:
        std::chrono::high_resolution_clock::time_point start_;
    };
    
    // Melvin interface (would need to be implemented)
    std::string queryMelvin(const Genome& genome, const std::string& query);
    void teachMelvin(const Genome& genome, const std::string& phrase);
    void configureMelvin(const Genome& genome);
    void configureMelvin(void* melvin, const Genome& genome); // Overload for direct Melvin instance (C API)
};

/**
 * Mini evaluation harness for fast fitness evaluation
 */
class MiniEvaluator {
public:
    struct MiniTestCase {
        std::string teach;
        int repeat;
        std::string ask;
        std::string expect_substr;
        std::string kind;
        
        MiniTestCase(const std::string& t, int r, const std::string& a, 
                    const std::string& e, const std::string& k)
            : teach(t), repeat(r), ask(a), expect_substr(e), kind(k) {}
    };
    
private:
    std::vector<MiniTestCase> mini_cases_;
    
public:
    MiniEvaluator();
    
    double evaluate(const Genome& genome);
    
    void addTestCase(const MiniTestCase& test_case);
    void initializeDefaultCases();
    
private:
    std::string executeMiniTestCase(const Genome& genome, const MiniTestCase& test_case);
};

/**
 * AGI test harness integration for periodic validation
 */
class AGIEvaluator {
public:
    struct AGIResult {
        int total_tests = 0;
        int passed_tests = 0;
        double pass_rate = 0.0;
        double avg_score = 0.0;
        double total_time = 0.0;
    };
    
    AGIResult evaluate(const Genome& genome);
    
private:
    std::string runAGITest(const Genome& genome);
    AGIResult parseAGIResults(const std::string& output);
};

} // namespace melvin::evolution
