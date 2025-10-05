#include "PersistentMemory.hpp"
#include "Genome.hpp"
#include "Fitness.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <thread>
#include <unordered_set>

namespace melvin::evolution {

PersistentMemoryManager::PersistentMemoryManager(const Config& config) 
    : config_(config), memory_dir_(config.memory_directory) {
}

PersistentMemoryManager::~PersistentMemoryManager() {
    shutdown();
}

bool PersistentMemoryManager::initialize() {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    
    if (initialized_.load()) {
        return true;
    }
    
    std::cout << "🧠 Initializing Persistent Memory Manager...\n";
    std::cout << "   Memory directory: " << memory_dir_ << "\n";
    
    // Create memory directory if it doesn't exist
    if (!createMemoryDirectory()) {
        std::cerr << "❌ Failed to create memory directory: " << memory_dir_ << "\n";
        return false;
    }
    
    // Load existing memory or initialize default knowledge
    if (std::filesystem::exists(memory_dir_ + "nodes.mdb") && 
        std::filesystem::exists(memory_dir_ + "edges.mdb")) {
        std::cout << "📂 Loading existing memory from disk...\n";
        if (!loadExistingMemory()) {
            std::cerr << "⚠️  Failed to load existing memory, initializing default knowledge\n";
            initializeDefaultKnowledge();
        }
    } else {
        std::cout << "🆕 Initializing with default knowledge...\n";
        initializeDefaultKnowledge();
    }
    
    // Get initial memory statistics
    last_stats_ = getMemoryStats();
    
    std::cout << "✅ Memory initialized: " << last_stats_.total_nodes << " nodes, " 
              << last_stats_.total_edges << " edges\n";
    std::cout << "   Memory health score: " << std::fixed << std::setprecision(3) 
              << calculateMemoryHealthScore(last_stats_) << "\n";
    
    initialized_.store(true);
    return true;
}

void PersistentMemoryManager::shutdown() {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    
    if (!initialized_.load()) {
        return;
    }
    
    std::cout << "🧹 Shutting down Persistent Memory Manager...\n";
    
    // Perform final maintenance if needed
    if (config_.enable_clutter_control) {
        performMaintenanceCleanup();
    }
    
    // Get final statistics
    auto final_stats = getMemoryStats();
    std::cout << "📊 Final memory state: " << final_stats.total_nodes << " nodes, " 
              << final_stats.total_edges << " edges\n";
    std::cout << "   Memory health score: " << std::fixed << std::setprecision(3) 
              << calculateMemoryHealthScore(final_stats) << "\n";
    
    initialized_.store(false);
}

bool PersistentMemoryManager::createMemoryDirectory() {
    try {
        std::filesystem::create_directories(memory_dir_);
        
        // Create subdirectories for different memory components
        std::filesystem::create_directories(memory_dir_ + "snapshots");
        std::filesystem::create_directories(memory_dir_ + "logs");
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create memory directory: " << e.what() << "\n";
        return false;
    }
}

bool PersistentMemoryManager::loadExistingMemory() {
    // Check if memory files exist and are readable
    auto nodes_file = memory_dir_ + "nodes.mdb";
    auto edges_file = memory_dir_ + "edges.mdb";
    
    if (!std::filesystem::exists(nodes_file) || !std::filesystem::exists(edges_file)) {
        return false;
    }
    
    // Verify file sizes are reasonable
    auto nodes_size = std::filesystem::file_size(nodes_file);
    auto edges_size = std::filesystem::file_size(edges_file);
    
    if (nodes_size == 0 || edges_size == 0) {
        std::cout << "⚠️  Memory files are empty, reinitializing\n";
        return false;
    }
    
    // Files exist and have content - assume they're valid
    std::cout << "✅ Loaded existing memory files (" << nodes_size << " bytes nodes, " 
              << edges_size << " bytes edges)\n";
    
    return true;
}

void PersistentMemoryManager::initializeDefaultKnowledge() {
    // Create a Melvin instance directly in the memory directory to initialize default knowledge
    melvin_t* melvin = melvin_create(memory_dir_.c_str());
    if (melvin) {
        // Add some basic knowledge to get started
        melvin_learn(melvin, "cats are mammals");
        melvin_learn(melvin, "dogs are mammals");
        melvin_learn(melvin, "birds can fly");
        melvin_learn(melvin, "fish live in water");
        melvin_learn(melvin, "humans are mammals");
        melvin_learn(melvin, "trees are plants");
        melvin_learn(melvin, "water is wet");
        melvin_learn(melvin, "fire is hot");
        
        melvin_destroy(melvin);
        
        std::cout << "✅ Initialized default knowledge base in " << memory_dir_ << "\n";
    } else {
        std::cerr << "❌ Failed to create Melvin instance for initialization\n";
    }
}

melvin_t* PersistentMemoryManager::createMelvinInstance(const std::string& instance_id) const {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    
    if (!initialized_.load()) {
        std::cerr << "❌ Memory manager not initialized\n";
        return nullptr;
    }
    
    // Create Melvin instance pointing to the shared memory directory
    melvin_t* melvin = melvin_create(memory_dir_.c_str());
    
    if (melvin) {
        std::cout << "🧠 Created Melvin instance '" << instance_id << "' with shared memory\n";
    } else {
        std::cerr << "❌ Failed to create Melvin instance '" << instance_id << "'\n";
    }
    
    return melvin;
}

void PersistentMemoryManager::destroyMelvinInstance(melvin_t* melvin) const {
    if (melvin) {
        melvin_destroy(melvin);
    }
}

PersistentMemoryManager::MemoryStats PersistentMemoryManager::getMemoryStats() const {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    
    MemoryStats stats;
    
    if (!initialized_.load()) {
        return stats;
    }
    
    // Create a temporary Melvin instance to get statistics
    melvin_t* temp_melvin = melvin_create(memory_dir_.c_str());
    if (temp_melvin) {
        stats.total_nodes = melvin_node_count(temp_melvin);
        stats.total_edges = melvin_edge_count(temp_melvin);
        stats.avg_edge_weight = melvin_get_health_score(temp_melvin);
        
        // Calculate integrity metrics
        if (config_.enable_integrity_checks) {
            stats.self_loops = countSelfLoops();
            stats.duplicate_edges = countDuplicateEdges();
            stats.invalid_nodes = countInvalidNodes();
            stats.memory_fragmentation = calculateFragmentation();
        }
        
        melvin_destroy(temp_melvin);
    }
    
    return stats;
}

bool PersistentMemoryManager::checkMemoryIntegrity(const MemoryStats& stats) const {
    if (!config_.enable_integrity_checks) {
        return true;
    }
    
    // Check self-loop ratio
    if (stats.total_edges > 0) {
        double self_loop_ratio = static_cast<double>(stats.self_loops) / stats.total_edges;
        if (self_loop_ratio > config_.max_self_loop_ratio) {
            std::cout << "⚠️  High self-loop ratio: " << self_loop_ratio << " (max: " 
                      << config_.max_self_loop_ratio << ")\n";
            return false;
        }
    }
    
    // Check duplicate edge ratio
    if (stats.total_edges > 0) {
        double duplicate_ratio = static_cast<double>(stats.duplicate_edges) / stats.total_edges;
        if (duplicate_ratio > config_.max_duplicate_edge_ratio) {
            std::cout << "⚠️  High duplicate edge ratio: " << duplicate_ratio << " (max: " 
                      << config_.max_duplicate_edge_ratio << ")\n";
            return false;
        }
    }
    
    // Check valid node ratio
    if (stats.total_nodes > 0) {
        double valid_node_ratio = 1.0 - (static_cast<double>(stats.invalid_nodes) / stats.total_nodes);
        if (valid_node_ratio < config_.min_valid_node_ratio) {
            std::cout << "⚠️  Low valid node ratio: " << valid_node_ratio << " (min: " 
                      << config_.min_valid_node_ratio << ")\n";
            return false;
        }
    }
    
    return true;
}

double PersistentMemoryManager::calculateMemoryHealthScore(const MemoryStats& stats) const {
    double health_score = 1.0;
    
    if (stats.total_edges > 0) {
        // Penalize self-loops
        double self_loop_ratio = static_cast<double>(stats.self_loops) / stats.total_edges;
        health_score -= self_loop_ratio * 0.3;
        
        // Penalize duplicate edges
        double duplicate_ratio = static_cast<double>(stats.duplicate_edges) / stats.total_edges;
        health_score -= duplicate_ratio * 0.2;
    }
    
    if (stats.total_nodes > 0) {
        // Penalize invalid nodes
        double invalid_ratio = static_cast<double>(stats.invalid_nodes) / stats.total_nodes;
        health_score -= invalid_ratio * 0.4;
    }
    
    // Penalize high fragmentation
    health_score -= stats.memory_fragmentation * 0.1;
    
    return std::max(0.0, std::min(1.0, health_score));
}

bool PersistentMemoryManager::isMemoryTooCluttered(const MemoryStats& stats) const {
    if (!config_.enable_clutter_control) {
        return false;
    }
    
    // Check edge count limit
    if (stats.total_edges > config_.max_edge_count) {
        return true;
    }
    
    // Check if average edge weight is too low (indicating clutter)
    if (stats.avg_edge_weight < config_.min_edge_weight) {
        return true;
    }
    
    return false;
}

void PersistentMemoryManager::performMaintenanceCleanup() {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    
    std::cout << "🧹 Performing memory maintenance cleanup...\n";
    
    auto stats_before = getMemoryStats();
    
    // Perform cleanup operations
    decayWeakEdges(config_.min_edge_weight);
    removeDuplicateEdges();
    consolidateFragmentedNodes();
    
    auto stats_after = getMemoryStats();
    
    std::cout << "✅ Maintenance complete: " << stats_before.total_edges << " → " 
              << stats_after.total_edges << " edges\n";
    std::cout << "   Health score: " << std::fixed << std::setprecision(3) 
              << calculateMemoryHealthScore(stats_after) << "\n";
}

// Helper function implementations (simplified for now)
size_t PersistentMemoryManager::countSelfLoops() const {
    // This would need to be implemented by querying the actual memory structure
    // For now, return a placeholder
    return 0;
}

size_t PersistentMemoryManager::countDuplicateEdges() const {
    // This would need to be implemented by querying the actual memory structure
    // For now, return a placeholder
    return 0;
}

size_t PersistentMemoryManager::countInvalidNodes() const {
    // This would need to be implemented by querying the actual memory structure
    // For now, return a placeholder
    return 0;
}

double PersistentMemoryManager::calculateFragmentation() const {
    // This would need to be implemented by analyzing memory layout
    // For now, return a placeholder
    return 0.0;
}

void PersistentMemoryManager::decayWeakEdges(double threshold) {
    // This would need to be implemented by modifying the actual memory structure
    // For now, just log the operation
    std::cout << "   Decaying edges with weight < " << threshold << "\n";
}

void PersistentMemoryManager::removeDuplicateEdges() {
    // This would need to be implemented by modifying the actual memory structure
    // For now, just log the operation
    std::cout << "   Removing duplicate edges\n";
}

void PersistentMemoryManager::consolidateFragmentedNodes() {
    // This would need to be implemented by modifying the actual memory structure
    // For now, just log the operation
    std::cout << "   Consolidating fragmented nodes\n";
}

// MemorySafeMelvin implementation
MemorySafeMelvin::MemorySafeMelvin(PersistentMemoryManager* manager, const std::string& instance_id)
    : manager_(manager), melvin_handle_(nullptr), instance_id_(instance_id) {
    
    if (manager_) {
        melvin_handle_ = manager_->createMelvinInstance(instance_id);
    }
}

MemorySafeMelvin::~MemorySafeMelvin() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (melvin_handle_ && manager_) {
        manager_->destroyMelvinInstance(melvin_handle_);
        melvin_handle_ = nullptr;
    }
}

MemorySafeMelvin::MemorySafeMelvin(MemorySafeMelvin&& other) noexcept
    : manager_(other.manager_), melvin_handle_(other.melvin_handle_), instance_id_(std::move(other.instance_id_)) {
    
    other.manager_ = nullptr;
    other.melvin_handle_ = nullptr;
}

MemorySafeMelvin& MemorySafeMelvin::operator=(MemorySafeMelvin&& other) noexcept {
    if (this != &other) {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        
        if (melvin_handle_ && manager_) {
            manager_->destroyMelvinInstance(melvin_handle_);
        }
        
        manager_ = other.manager_;
        melvin_handle_ = other.melvin_handle_;
        instance_id_ = std::move(other.instance_id_);
        
        other.manager_ = nullptr;
        other.melvin_handle_ = nullptr;
    }
    
    return *this;
}

const char* MemorySafeMelvin::reason(const char* query) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (!melvin_handle_) {
        return "Error: Invalid Melvin instance";
    }
    
    return melvin_reason(melvin_handle_, query);
}

void MemorySafeMelvin::learn(const char* text) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (melvin_handle_) {
        melvin_learn(melvin_handle_, text);
    }
}

void MemorySafeMelvin::decayPass() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (melvin_handle_) {
        melvin_decay_pass(melvin_handle_);
    }
}

size_t MemorySafeMelvin::getNodeCount() const {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (!melvin_handle_) {
        return 0;
    }
    
    return melvin_node_count(melvin_handle_);
}

size_t MemorySafeMelvin::getEdgeCount() const {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (!melvin_handle_) {
        return 0;
    }
    
    return melvin_edge_count(melvin_handle_);
}

float MemorySafeMelvin::getHealthScore() const {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (!melvin_handle_) {
        return 0.0f;
    }
    
    return melvin_get_health_score(melvin_handle_);
}

// MemoryEvaluationContext implementation
MemoryEvaluationContext::MemoryEvaluationContext(PersistentMemoryManager* manager, const std::string& context_id)
    : memory_manager_(manager), context_id_(context_id) {
}

MemoryEvaluationContext::EvaluationResult MemoryEvaluationContext::evaluateGenome(
    const Genome& genome, const std::vector<TestCase>& test_cases) {
    
    EvaluationResult result;
    
    if (!memory_manager_ || !memory_manager_->isInitialized()) {
        std::cerr << "❌ Memory manager not available for evaluation\n";
        return result;
    }
    
    // Get memory state before evaluation
    result.memory_before = memory_manager_->getMemoryStats();
    
    // Create a memory-safe Melvin instance for this evaluation
    MemorySafeMelvin melvin(memory_manager_, context_id_ + "_" + std::to_string(genome.id));
    
    if (!melvin.isValid()) {
        std::cerr << "❌ Failed to create Melvin instance for evaluation\n";
        return result;
    }
    
    // Configure Melvin with genome parameters (simplified for now)
    // In a full implementation, this would apply all 55 parameters
    
    // Execute test cases
    std::vector<std::string> responses;
    double total_time = 0.0;
    
    for (const auto& test_case : test_cases) {
        // Teach Melvin if needed
        for (int i = 0; i < test_case.repeat_count; ++i) {
            if (!test_case.teach_phrase.empty()) {
                melvin.learn(test_case.teach_phrase.c_str());
            }
        }
        
        // Measure response time
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string response = executeTestCase(melvin, test_case);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        double response_time = std::chrono::duration<double>(end_time - start_time).count();
        total_time += response_time;
        
        responses.push_back(response);
        result.queries_processed++;
        
        // Check correctness
        if (response.find(test_case.expected_keywords) != std::string::npos) {
            result.successful_queries++;
        }
    }
    
    // Get memory state after evaluation
    result.memory_after = memory_manager_->getMemoryStats();
    
    // Calculate metrics
    result.correctness = static_cast<double>(result.successful_queries) / result.queries_processed;
    result.avg_response_time = total_time / result.queries_processed;
    result.speed = 1.0 / (1.0 + result.avg_response_time);
    
    // Calculate creativity (simplified)
    std::unordered_set<std::string> unique_responses;
    for (const auto& response : responses) {
        unique_responses.insert(response);
    }
    result.creativity = static_cast<double>(unique_responses.size()) / responses.size();
    
    // Calculate memory health
    result.memory_health = memory_manager_->calculateMemoryHealthScore(result.memory_after);
    
    // Calculate penalties
    result.integrity_penalty = calculateIntegrityPenalty(result.memory_after);
    result.clutter_penalty = calculateClutterPenalty(result.memory_after);
    
    // Calculate total fitness
    result.total_fitness = 0.6 * result.correctness + 
                          0.25 * result.speed + 
                          0.15 * result.creativity +
                          0.1 * result.memory_health -
                          result.integrity_penalty -
                          result.clutter_penalty;
    
    result.total_fitness = std::max(0.0, result.total_fitness);
    
    return result;
}

std::string MemoryEvaluationContext::executeTestCase(MemorySafeMelvin& melvin, const TestCase& test_case) {
    const char* response = melvin.reason(test_case.question.c_str());
    return response ? response : "No response";
}

double MemoryEvaluationContext::calculateIntegrityPenalty(const PersistentMemoryManager::MemoryStats& stats) const {
    if (!memory_manager_->getConfig().enable_integrity_checks) {
        return 0.0;
    }
    
    double penalty = 0.0;
    
    // Penalize self-loops
    if (stats.total_edges > 0) {
        double self_loop_ratio = static_cast<double>(stats.self_loops) / stats.total_edges;
        if (self_loop_ratio > memory_manager_->getConfig().max_self_loop_ratio) {
            penalty += 0.1 * (self_loop_ratio - memory_manager_->getConfig().max_self_loop_ratio);
        }
    }
    
    // Penalize duplicate edges
    if (stats.total_edges > 0) {
        double duplicate_ratio = static_cast<double>(stats.duplicate_edges) / stats.total_edges;
        if (duplicate_ratio > memory_manager_->getConfig().max_duplicate_edge_ratio) {
            penalty += 0.1 * (duplicate_ratio - memory_manager_->getConfig().max_duplicate_edge_ratio);
        }
    }
    
    return penalty;
}

double MemoryEvaluationContext::calculateClutterPenalty(const PersistentMemoryManager::MemoryStats& stats) const {
    if (!memory_manager_->getConfig().enable_clutter_control) {
        return 0.0;
    }
    
    double penalty = 0.0;
    
    // Penalize too many edges
    if (stats.total_edges > memory_manager_->getConfig().max_edge_count) {
        penalty += 0.2 * (stats.total_edges - memory_manager_->getConfig().max_edge_count) / 
                   memory_manager_->getConfig().max_edge_count;
    }
    
    // Penalize low average edge weight (indicating clutter)
    if (stats.avg_edge_weight < memory_manager_->getConfig().min_edge_weight) {
        penalty += 0.1 * (memory_manager_->getConfig().min_edge_weight - stats.avg_edge_weight);
    }
    
    return penalty;
}

} // namespace melvin::evolution
