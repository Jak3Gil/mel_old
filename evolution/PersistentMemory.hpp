#pragma once

#include "../melvin.h"
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>

// Forward declarations
namespace melvin::evolution {
    struct Genome;
    struct TestCase;
    class MiniEvaluator;
}

namespace melvin::evolution {

/**
 * Persistent Memory Manager for Evolution System
 * 
 * Manages a shared knowledge base that persists across all generations.
 * All genomes share the same memory substrate but differ in how they use it.
 */

class PersistentMemoryManager {
public:
    struct Config {
        std::string memory_directory = "evolution_memory/";
        bool read_write_mode = true;           // Allow reinforcement/decay during evaluation
        bool enable_integrity_checks = true;   // Check for memory corruption
        bool enable_clutter_control = true;    // Penalize over-decay or clutter
        
        // Memory integrity thresholds
        double max_self_loop_ratio = 0.05;     // Max 5% self-loops
        double max_duplicate_edge_ratio = 0.10; // Max 10% duplicate edges
        double min_valid_node_ratio = 0.95;    // Min 95% valid nodes
        
        // Clutter control thresholds
        double max_edge_count = 100000;        // Hard limit on total edges
        double min_edge_weight = 0.001;        // Minimum edge weight before decay
    };
    
private:
    Config config_;
    std::string memory_dir_;
    mutable std::mutex memory_mutex_;
    std::atomic<bool> initialized_{false};
    
    // Memory statistics for integrity checking
public:
    struct MemoryStats {
        size_t total_nodes = 0;
        size_t total_edges = 0;
        size_t self_loops = 0;
        size_t duplicate_edges = 0;
        size_t invalid_nodes = 0;
        double avg_edge_weight = 0.0;
        double memory_fragmentation = 0.0;
    };

private:
    MemoryStats last_stats_;
    
public:
    explicit PersistentMemoryManager(const Config& config);
    ~PersistentMemoryManager();
    
    // Initialization and cleanup
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized_.load(); }
    
    // Memory access
    melvin_t* createMelvinInstance(const std::string& instance_id) const;
    void destroyMelvinInstance(melvin_t* melvin) const;
    
    // Memory integrity and health
    MemoryStats getMemoryStats() const;
    bool checkMemoryIntegrity(const MemoryStats& stats) const;
    double calculateMemoryHealthScore(const MemoryStats& stats) const;
    
    // Clutter control
    bool isMemoryTooCluttered(const MemoryStats& stats) const;
    void performMaintenanceCleanup();
    
    // Configuration
    void setConfig(const Config& config) { config_ = config; }
    const Config& getConfig() const { return config_; }
    
    // Thread safety
    void lockMemory() { memory_mutex_.lock(); }
    void unlockMemory() { memory_mutex_.unlock(); }
    
private:
    // Helper functions
    bool createMemoryDirectory();
    bool loadExistingMemory();
    void initializeDefaultKnowledge();
    
    // Integrity checking helpers
    size_t countSelfLoops() const;
    size_t countDuplicateEdges() const;
    size_t countInvalidNodes() const;
    double calculateFragmentation() const;
    
    // Maintenance helpers
    void decayWeakEdges(double threshold);
    void removeDuplicateEdges();
    void consolidateFragmentedNodes();
};

/**
 * Memory-Safe Melvin Instance Wrapper
 * 
 * Ensures proper cleanup and thread safety when using Melvin instances
 * with the persistent memory manager.
 */
class MemorySafeMelvin {
public:
    explicit MemorySafeMelvin(PersistentMemoryManager* manager, const std::string& instance_id);
    ~MemorySafeMelvin();
    
    // Non-copyable, movable
    MemorySafeMelvin(const MemorySafeMelvin&) = delete;
    MemorySafeMelvin& operator=(const MemorySafeMelvin&) = delete;
    MemorySafeMelvin(MemorySafeMelvin&& other) noexcept;
    MemorySafeMelvin& operator=(MemorySafeMelvin&& other) noexcept;
    
    // Melvin API wrapper
    const char* reason(const char* query);
    void learn(const char* text);
    void decayPass();
    
    // Memory management
    melvin_t* getHandle() const { return melvin_handle_; }
    bool isValid() const { return melvin_handle_ != nullptr; }
    
    // Statistics
    size_t getNodeCount() const;
    size_t getEdgeCount() const;
    float getHealthScore() const;
    
private:
    PersistentMemoryManager* manager_;
    melvin_t* melvin_handle_;
    std::string instance_id_;
    mutable std::mutex instance_mutex_;
};

/**
 * Memory Evaluation Context
 * 
 * Provides a safe context for evaluating genomes against the persistent memory.
 * Handles isolation, integrity checking, and cleanup.
 */
class MemoryEvaluationContext {
public:
    struct EvaluationResult {
        double correctness = 0.0;
        double speed = 0.0;
        double creativity = 0.0;
        double memory_health = 0.0;
        double integrity_penalty = 0.0;
        double clutter_penalty = 0.0;
        double total_fitness = 0.0;
        
        // Detailed metrics
        size_t queries_processed = 0;
        size_t successful_queries = 0;
        double avg_response_time = 0.0;
        size_t memory_corruption_count = 0;
        
        // Memory state before/after evaluation
        PersistentMemoryManager::MemoryStats memory_before;
        PersistentMemoryManager::MemoryStats memory_after;
    };
    
private:
    PersistentMemoryManager* memory_manager_;
    std::string context_id_;
    
public:
    explicit MemoryEvaluationContext(PersistentMemoryManager* manager, const std::string& context_id);
    
    // Evaluation methods
    EvaluationResult evaluateGenome(const Genome& genome, const std::vector<struct TestCase>& test_cases);
    EvaluationResult evaluateGenomeMini(const Genome& genome, const std::vector<struct MiniTestCase>& test_cases);
    
    // Memory state management
    void saveMemorySnapshot(const std::string& snapshot_name);
    void restoreMemorySnapshot(const std::string& snapshot_name);
    void clearMemorySnapshots();
    
    // Utility methods
    std::string getContextId() const { return context_id_; }
    PersistentMemoryManager* getMemoryManager() const { return memory_manager_; }
    
private:
    // Helper methods
    double measureResponseTime(std::function<void()> operation) const;
    bool checkMemoryIntegrityBeforeAfter(const PersistentMemoryManager::MemoryStats& before,
                                        const PersistentMemoryManager::MemoryStats& after) const;
    double calculateIntegrityPenalty(const PersistentMemoryManager::MemoryStats& stats) const;
    double calculateClutterPenalty(const PersistentMemoryManager::MemoryStats& stats) const;
    
    // Test execution
    std::string executeTestCase(MemorySafeMelvin& melvin, const struct TestCase& test_case);
    std::string executeMiniTestCase(MemorySafeMelvin& melvin, const struct MiniTestCase& test_case);
};

} // namespace melvin::evolution
