#pragma once

#include "types.h"
#include "storage.h"
#include "episodic_memory.h"
#include "tokenizer.h"
#include <string>
#include <vector>
#include <memory>

namespace melvin {
namespace dataset {

/**
 * 📚 Dataset Loader - HuggingFace Integration & Curriculum Learning
 * 
 * Loads datasets and converts them into EXACT node chains for Melvin.
 * Implements curriculum learning: start simple, increase complexity.
 * 
 * Key Features:
 * - HuggingFace Datasets API integration (via Python bridge)
 * - Text corpus ingestion as temporal EXACT sequences
 * - Curriculum stages with difficulty progression
 * - Batch processing for efficient learning
 * - Episode creation for each document/sample
 */
class DatasetLoader {
public:
    struct Config {
        size_t batch_size = 32;               // Samples per batch
        size_t max_samples_per_dataset = 10000; // Limit per dataset
        bool create_episodes = true;          // Create episode per sample
        bool verbose = true;                  // Progress logging
        std::string cache_dir = "data/cache/"; // Cache directory
        
        Config();
    };
    
    /**
     * Curriculum: ordered sequence of datasets with sample counts
     */
    struct Curriculum {
        struct Stage {
            std::string name;                 // Stage name
            std::vector<std::string> datasets;  // Dataset names
            size_t samples_per_dataset;       // Samples to use
            std::string description;          // What this stage teaches
        };
        
        std::vector<Stage> stages;
        
        // Pre-defined curricula
        static Curriculum factual_curriculum();     // Facts first
        static Curriculum linguistic_curriculum();  // Language patterns first
        static Curriculum balanced_curriculum();    // Mixed approach
    };
    
    explicit DatasetLoader(
        Storage* storage,
        episodic::EpisodicMemory* episodes,
        tokenizer::Tokenizer* tokenizer,
        const Config& config = Config()
    );
    ~DatasetLoader();
    
    // ========================================================================
    // DATASET LOADING
    // ========================================================================
    
    /**
     * Load a dataset from HuggingFace (simulated - reads local files)
     * Returns number of samples loaded
     */
    size_t load_dataset(
        const std::string& dataset_name,
        const std::string& split = "train",
        size_t max_samples = 1000
    );
    
    /**
     * Load from local text file
     * One document per line or separated by blank lines
     */
    size_t load_from_file(const std::string& path);
    
    /**
     * Load multiple datasets
     */
    size_t load_datasets(const std::vector<std::string>& dataset_names);
    
    // ========================================================================
    // INGESTION
    // ========================================================================
    
    /**
     * Ingest texts into graph as EXACT node chains
     * Creates episodes for temporal structure
     */
    void ingest_batch(const std::vector<std::string>& texts);
    
    /**
     * Ingest single text
     */
    void ingest_text(const std::string& text, const std::string& context = "");
    
    // ========================================================================
    // CURRICULUM LEARNING
    // ========================================================================
    
    /**
     * Train through a curriculum
     * Returns final statistics
     */
    struct TrainingStats {
        size_t total_texts = 0;
        size_t total_tokens = 0;
        size_t nodes_created = 0;
        size_t edges_created = 0;
        size_t episodes_created = 0;
        float avg_text_length = 0.0f;
    };
    
    TrainingStats train_curriculum(const Curriculum& curriculum);
    
    /**
     * Train single stage
     */
    TrainingStats train_stage(const Curriculum::Stage& stage);
    
    // ========================================================================
    // UTILITIES
    // ========================================================================
    
    /**
     * Get available datasets (from cache or predefined list)
     */
    std::vector<std::string> get_available_datasets() const;
    
    /**
     * Estimate dataset difficulty (for curriculum ordering)
     * Based on vocab complexity, sentence length, etc.
     */
    float estimate_difficulty(const std::string& dataset_name) const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    TrainingStats get_stats() const;
    void print_stats() const;
    void reset_stats();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dataset
} // namespace melvin

