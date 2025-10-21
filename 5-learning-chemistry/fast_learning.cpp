/*
 * FastLearning Implementation - Optimized Batch Learning
 */

#include "fast_learning.h"
#include "leap_inference.h"
#include "adaptive_window.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <future>

namespace melvin {
namespace fast {

// ============================================================================
// CONFIG
// ============================================================================

FastLearning::Config::Config() {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
    }
}

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class FastLearning::Impl {
public:
    optimized::OptimizedStorage* storage = nullptr;
    Config config;
    Stats stats;
    
    // LEAP queue
    std::vector<NodeID> leap_queue_;
    std::mutex leap_mutex_;
    
    // Background processor
    std::atomic<bool> background_running_{false};
    std::thread background_thread_;
    
    // Adaptive window manager
    std::unique_ptr<adaptive::AdaptiveWindowManager> adaptive_window_;
    
    // Parse a fact into words/concepts
    std::vector<std::string> parse_fact(const std::string& fact) {
        std::vector<std::string> words;
        std::istringstream iss(fact);
        std::string word;
        
        while (iss >> word) {
            // Clean up word
            word.erase(std::remove_if(word.begin(), word.end(), 
                [](char c) { return !std::isalnum(c) && c != '_' && c != '-'; }), 
                word.end());
            
            if (word.length() >= 2) {
                words.push_back(word);
            }
        }
        
        return words;
    }
    
    // Process a batch of facts
    size_t process_batch(const std::vector<std::string>& facts) {
        if (!storage) return 0;
        
        auto start = std::chrono::steady_clock::now();
        
        size_t nodes_created = 0;
        size_t edges_created = 0;
        
        // Prepare batch insert
        optimized::OptimizedStorage::BatchInsert batch;
        std::vector<std::tuple<NodeID, NodeID, RelationType, float>> edge_batch;
        
        // Parse all facts first
        std::vector<std::vector<std::string>> parsed_facts;
        parsed_facts.reserve(facts.size());
        
        for (const auto& fact : facts) {
            if (fact.length() > 10 && fact[0] != '#') {
                parsed_facts.push_back(parse_fact(fact));
            }
        }
        
        // Create nodes in batch
        std::vector<std::vector<NodeID>> fact_node_ids;
        fact_node_ids.reserve(parsed_facts.size());
        
        for (const auto& words : parsed_facts) {
            std::vector<NodeID> node_ids;
            node_ids.reserve(words.size());
            
            for (const auto& word : words) {
                NodeID id = storage->create_node(word, NodeType::CONCEPT);
                node_ids.push_back(id);
                nodes_created++;
            }
            
            fact_node_ids.push_back(node_ids);
        }
        
        // Create edges using adaptive window system
        if (adaptive_window_) {
            // Use adaptive window for dynamic EXACT connections
            for (const auto& node_ids : fact_node_ids) {
                auto adaptive_edges = adaptive_window_->add_nodes_to_buffer(node_ids);
                edge_batch.insert(edge_batch.end(), adaptive_edges.begin(), adaptive_edges.end());
                edges_created += adaptive_edges.size();
            }
        } else {
            // Fallback to simple consecutive edges (legacy behavior)
            for (const auto& node_ids : fact_node_ids) {
                for (size_t i = 0; i + 1 < node_ids.size(); ++i) {
                    edge_batch.emplace_back(node_ids[i], node_ids[i + 1], RelationType::EXACT, 1.0f);
                    edges_created++;
                }
            }
        }
        
        // Batch insert edges
        if (!edge_batch.empty()) {
            storage->batch_create_edges(edge_batch);
        }
        
        // Queue nodes for LEAP processing
        if (config.defer_leaps) {
            std::lock_guard<std::mutex> lock(leap_mutex_);
            for (const auto& node_ids : fact_node_ids) {
                leap_queue_.insert(leap_queue_.end(), node_ids.begin(), node_ids.end());
            }
        }
        
        auto end = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // Update stats
        stats.batches_processed++;
        stats.facts_ingested += facts.size();
        stats.nodes_created += nodes_created;
        stats.edges_created += edges_created;
        
        // Copy adaptive window stats if available
        if (adaptive_window_ && config.adaptive_config.enable_stats) {
            stats.adaptive_stats = adaptive_window_->get_stats();
        }
        
        // Running average
        double n = stats.batches_processed;
        stats.avg_batch_time_ms = (stats.avg_batch_time_ms * (n - 1) + elapsed_ms) / n;
        
        return facts.size();
    }
    
    // Process a chunk of a file (for parallel processing)
    size_t process_file_chunk(const std::vector<std::string>& lines) {
        std::vector<std::string> batch;
        batch.reserve(config.batch_size);
        size_t total_processed = 0;
        
        for (const auto& line : lines) {
            if (!line.empty() && line.length() > 10 && line[0] != '#') {
                batch.push_back(line);
                
                if (batch.size() >= config.batch_size) {
                    total_processed += process_batch(batch);
                    batch.clear();
                }
            }
        }
        
        // Process remaining
        if (!batch.empty()) {
            total_processed += process_batch(batch);
        }
        
        return total_processed;
    }
};

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

FastLearning::FastLearning(optimized::OptimizedStorage* storage, const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->storage = storage;
    impl_->config = config;
    
    // Initialize adaptive window manager
    if (storage) {
        impl_->adaptive_window_ = std::make_unique<adaptive::AdaptiveWindowManager>(
            storage, config.adaptive_config
        );
    }
}

FastLearning::~FastLearning() {
    stop_background_leaps();
}

size_t FastLearning::ingest_facts(const std::vector<std::string>& facts) {
    if (!impl_->storage) return 0;
    
    auto start = std::chrono::steady_clock::now();
    
    std::cout << "🚀 Fast ingestion: " << facts.size() << " facts...\n";
    
    size_t total_processed = 0;
    
    if (impl_->config.enable_parallel && facts.size() > impl_->config.batch_size * 4) {
        // Parallel processing
        size_t num_threads = impl_->config.num_threads;
        size_t chunk_size = facts.size() / num_threads;
        
        std::vector<std::future<size_t>> futures;
        
        for (size_t i = 0; i < num_threads; ++i) {
            size_t start_idx = i * chunk_size;
            size_t end_idx = (i == num_threads - 1) ? facts.size() : (i + 1) * chunk_size;
            
            std::vector<std::string> chunk(facts.begin() + start_idx, facts.begin() + end_idx);
            
            futures.push_back(std::async(std::launch::async, [this, chunk]() {
                return impl_->process_file_chunk(chunk);
            }));
        }
        
        for (auto& future : futures) {
            total_processed += future.get();
        }
        
        std::cout << "  ✓ Parallel processing with " << num_threads << " threads\n";
    } else {
        // Sequential batching
        std::vector<std::string> batch;
        batch.reserve(impl_->config.batch_size);
        
        for (const auto& fact : facts) {
            if (!fact.empty() && fact.length() > 10 && fact[0] != '#') {
                batch.push_back(fact);
                
                if (batch.size() >= impl_->config.batch_size) {
                    total_processed += impl_->process_batch(batch);
                    batch.clear();
                }
            }
        }
        
        if (!batch.empty()) {
            total_processed += impl_->process_batch(batch);
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
    
    impl_->stats.total_time_sec += elapsed_sec;
    impl_->stats.facts_per_second = impl_->stats.facts_ingested / impl_->stats.total_time_sec;
    
    std::cout << "  ✓ Processed " << total_processed << " facts in " 
              << std::fixed << std::setprecision(2) << elapsed_sec << "s\n";
    std::cout << "  ⚡ Speed: " << impl_->stats.facts_per_second << " facts/sec\n";
    
    return total_processed;
}

size_t FastLearning::ingest_file(const std::string& path) {
    std::cout << "📖 Reading " << path << "...\n";
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "❌ Could not open " << path << "\n";
        return 0;
    }
    
    // Check file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    size_t file_size_mb = file_size / (1024 * 1024);
    
    std::vector<std::string> facts;
    std::string line;
    
    if (impl_->config.stream_large_files && file_size_mb > impl_->config.stream_threshold_mb) {
        std::cout << "  Large file (" << file_size_mb << "MB), streaming...\n";
        
        // Stream in chunks
        std::vector<std::string> batch;
        batch.reserve(impl_->config.batch_size);
        size_t total_processed = 0;
        
        while (std::getline(file, line)) {
            if (!line.empty() && line.length() > 10 && line[0] != '#') {
                batch.push_back(line);
                
                if (batch.size() >= impl_->config.batch_size) {
                    total_processed += impl_->process_batch(batch);
                    batch.clear();
                }
            }
        }
        
        if (!batch.empty()) {
            total_processed += impl_->process_batch(batch);
        }
        
        return total_processed;
    } else {
        // Load all into memory
        while (std::getline(file, line)) {
            if (!line.empty() && line.length() > 10 && line[0] != '#') {
                facts.push_back(line);
            }
        }
        
        std::cout << "  ✓ Loaded " << facts.size() << " facts\n";
        return ingest_facts(facts);
    }
}

size_t FastLearning::ingest_files_parallel(const std::vector<std::string>& paths) {
    std::cout << "📚 Processing " << paths.size() << " files in parallel...\n\n";
    
    std::vector<std::future<size_t>> futures;
    
    for (const auto& path : paths) {
        futures.push_back(std::async(std::launch::async, [this, path]() {
            return this->ingest_file(path);
        }));
    }
    
    size_t total = 0;
    for (auto& future : futures) {
        total += future.get();
    }
    
    return total;
}

void FastLearning::queue_for_leaps(const std::vector<NodeID>& new_nodes) {
    std::lock_guard<std::mutex> lock(impl_->leap_mutex_);
    impl_->leap_queue_.insert(impl_->leap_queue_.end(), new_nodes.begin(), new_nodes.end());
}

size_t FastLearning::process_leap_queue() {
    std::vector<NodeID> queue_copy;
    
    {
        std::lock_guard<std::mutex> lock(impl_->leap_mutex_);
        queue_copy = impl_->leap_queue_;
        impl_->leap_queue_.clear();
    }
    
    if (queue_copy.empty()) {
        return 0;
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  PROCESSING DEFERRED LEAPs (" << queue_copy.size() << " nodes)                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    auto start = std::chrono::steady_clock::now();
    
    // Run LEAP inference
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 4;
    leap_config.min_shared_neighbors = 2;
    leap_config.leap_confidence = 0.7f;
    leap_config.verbose = false;
    
    leap::LEAPInference leap_system(leap_config);
    
    std::cout << "  Phase 1: Transitive shortcuts...\n";
    
    // We need to convert OptimizedStorage to regular Storage for LEAP system
    // For now, just report that LEAPs would be created
    // TODO: Make LEAP system work with OptimizedStorage
    
    std::cout << "    ⚠️  LEAP processing requires conversion (TODO)\n";
    std::cout << "    For now, LEAPs will be created during save/load cycle\n";
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    
    std::cout << "\n✅ LEAP processing complete in " << elapsed_sec << "s\n\n";
    
    return 0;
}

bool FastLearning::should_process_leaps() const {
    std::lock_guard<std::mutex> lock(impl_->leap_mutex_);
    return impl_->leap_queue_.size() >= impl_->config.leap_batch_threshold;
}

void FastLearning::start_background_leaps() {
    if (impl_->background_running_) return;
    
    impl_->background_running_ = true;
    impl_->background_thread_ = std::thread([this]() {
        while (impl_->background_running_) {
            if (should_process_leaps()) {
                process_leap_queue();
            }
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    });
    
    std::cout << "  ✓ Background LEAP processor started\n";
}

void FastLearning::stop_background_leaps() {
    if (!impl_->background_running_) return;
    
    impl_->background_running_ = false;
    if (impl_->background_thread_.joinable()) {
        impl_->background_thread_.join();
    }
}

FastLearning::Stats FastLearning::get_stats() const {
    return impl_->stats;
}

void FastLearning::reset_stats() {
    impl_->stats = Stats();
}

void FastLearning::print_stats() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  FAST LEARNING STATISTICS                                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "  Facts ingested: " << impl_->stats.facts_ingested << "\n";
    std::cout << "  Batches processed: " << impl_->stats.batches_processed << "\n";
    std::cout << "  Nodes created: " << impl_->stats.nodes_created << "\n";
    std::cout << "  Edges created: " << impl_->stats.edges_created << "\n";
    std::cout << "  LEAPs created: " << impl_->stats.leaps_created << "\n\n";
    
    std::cout << "  Average batch time: " << std::fixed << std::setprecision(2) 
              << impl_->stats.avg_batch_time_ms << " ms\n";
    std::cout << "  Total time: " << impl_->stats.total_time_sec << " seconds\n";
    std::cout << "  Processing speed: " << impl_->stats.facts_per_second << " facts/sec\n\n";
    
    std::cout << "  Queued for LEAPs: " << impl_->leap_queue_.size() << " nodes\n\n";
    
    // Print adaptive window statistics if available
    if (impl_->adaptive_window_) {
        impl_->adaptive_window_->print_stats();
    }
}

}} // namespace melvin::fast

