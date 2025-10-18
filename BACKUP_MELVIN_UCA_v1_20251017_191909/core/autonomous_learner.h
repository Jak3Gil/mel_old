#pragma once

#include "types.h"
#include "storage.h"
#include "episodic_memory.h"
#include "dataset_loader.h"
#include "hybrid_predictor.h"
#include "sequence_predictor.h"
#include "gnn_predictor.h"
#include <memory>
#include <vector>
#include <string>

namespace melvin {
namespace autonomous {

/**
 * 🤖 Autonomous Learner - Continuous Self-Improvement
 * 
 * Main training loop that combines all components for autonomous learning.
 * Key innovation: Self-directed exploration based on prediction errors.
 * 
 * Features:
 * - Continuous learning from datasets
 * - Self-directed knowledge gap exploration
 * - Curriculum-based progression
 * - Performance tracking and adaptation
 * - Graph growth and LEAP formation monitoring
 */
class AutonomousLearner {
public:
    struct Config {
        int training_epochs = 100;            // Total epochs
        int batch_size = 32;                  // Batch size
        int save_interval = 10;               // Save every N epochs
        float learning_rate = 0.0001f;        // Base learning rate
        bool enable_self_exploration = true;  // Generate knowledge gaps
        int exploration_interval = 50;        // Explore every N epochs
        bool verbose = true;                  // Progress logging
        std::string checkpoint_dir = "data/checkpoints/";
        
        Config();
    };
    
    struct LearningMetrics {
        float perplexity = 0.0f;              // How surprised by new data
        float graph_density = 0.0f;           // Knowledge connectivity
        float episode_coherence = 0.0f;       // Temporal consistency
        float prediction_accuracy = 0.0f;     // Next-token accuracy
        size_t nodes_count = 0;               // Total nodes
        size_t edges_count = 0;               // Total edges
        size_t exact_edges = 0;               // EXACT edges
        size_t leap_edges = 0;                // LEAP edges
        float leap_formation_rate = 0.0f;     // LEAPs created per epoch
    };
    
    explicit AutonomousLearner(
        Storage* storage,
        episodic::EpisodicMemory* episodes,
        hybrid::HybridPredictor* predictor,
        dataset::DatasetLoader* loader,
        gnn::GNNPredictor* gnn,
        const Config& config = Config()
    );
    ~AutonomousLearner();
    
    // ========================================================================
    // TRAINING LOOP
    // ========================================================================
    
    /**
     * Main training loop
     * Runs for configured number of epochs
     */
    LearningMetrics run_training(const dataset::DatasetLoader::Curriculum& curriculum);
    
    /**
     * Single training epoch
     */
    LearningMetrics run_epoch(int epoch_num);
    
    /**
     * Train on a batch of texts
     */
    float train_batch(const std::vector<std::string>& texts);
    
    // ========================================================================
    // SELF-DIRECTED EXPLORATION
    // ========================================================================
    
    /**
     * Identify knowledge gaps in the graph
     * Returns: questions/topics to explore
     */
    std::vector<std::string> generate_knowledge_gaps();
    
    /**
     * Explore a specific topic/gap
     * Creates hypothetical connections and tests them
     */
    void explore_gap(const std::string& topic);
    
    /**
     * Generate LEAP connections from patterns
     * Calls LEAP inference system
     */
    int create_leaps();
    
    // ========================================================================
    // EVALUATION
    // ========================================================================
    
    /**
     * Evaluate on validation set
     */
    LearningMetrics evaluate(const std::vector<std::string>& validation_texts);
    
    /**
     * Compute graph metrics
     */
    void compute_graph_metrics(LearningMetrics& metrics);
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    /**
     * Save checkpoint
     */
    bool save_checkpoint(int epoch);
    
    /**
     * Load checkpoint
     */
    bool load_checkpoint(int epoch);
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    LearningMetrics get_metrics() const;
    void print_metrics() const;
    void reset_metrics();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace autonomous
} // namespace melvin

