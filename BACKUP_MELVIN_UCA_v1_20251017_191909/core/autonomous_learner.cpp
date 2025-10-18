#include "autonomous_learner.h"
#include "leap_inference.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <algorithm>

namespace melvin {
namespace autonomous {

// ============================================================================
// Config Implementation
// ============================================================================

AutonomousLearner::Config::Config() {}

// ============================================================================
// Implementation Class
// ============================================================================

class AutonomousLearner::Impl {
public:
    Config config_;
    Storage* storage_;
    episodic::EpisodicMemory* episodes_;
    hybrid::HybridPredictor* predictor_;
    dataset::DatasetLoader* loader_;
    gnn::GNNPredictor* gnn_;
    
    LearningMetrics metrics_;
    int current_epoch_ = 0;
    
    Impl(Storage* storage, episodic::EpisodicMemory* episodes,
         hybrid::HybridPredictor* predictor, dataset::DatasetLoader* loader,
         gnn::GNNPredictor* gnn, const Config& config)
        : config_(config), storage_(storage), episodes_(episodes)
        , predictor_(predictor), loader_(loader), gnn_(gnn) {}
    
    LearningMetrics run_training(const dataset::DatasetLoader::Curriculum& curriculum) {
        if (config_.verbose) {
            std::cout << "\n╔═══════════════════════════════════════════════╗\n";
            std::cout << "║  MELVIN AUTONOMOUS LEARNING                   ║\n";
            std::cout << "╚═══════════════════════════════════════════════╝\n\n";
            std::cout << "Configuration:\n";
            std::cout << "  Epochs: " << config_.training_epochs << "\n";
            std::cout << "  Batch size: " << config_.batch_size << "\n";
            std::cout << "  Learning rate: " << config_.learning_rate << "\n";
            std::cout << "  Self-exploration: " << (config_.enable_self_exploration ? "enabled" : "disabled") << "\n\n";
        }
        
        // Train through curriculum
        if (loader_) {
            loader_->train_curriculum(curriculum);
        }
        
        // Main training loop
        for (int epoch = 1; epoch <= config_.training_epochs; epoch++) {
            current_epoch_ = epoch;
            
            if (config_.verbose) {
                std::cout << "\n=== Epoch " << epoch << "/" << config_.training_epochs << " ===\n";
            }
            
            auto epoch_metrics = run_epoch(epoch);
            
            // Update running metrics
            metrics_ = epoch_metrics;
            
            // Self-exploration
            if (config_.enable_self_exploration && epoch % config_.exploration_interval == 0) {
                if (config_.verbose) {
                    std::cout << "\n[Autonomous] Self-exploration phase...\n";
                }
                
                auto gaps = generate_knowledge_gaps();
                for (const auto& gap : gaps) {
                    explore_gap(gap);
                }
            }
            
            // Create LEAP connections
            int leaps_created = create_leaps();
            metrics_.leap_formation_rate = leaps_created;
            
            // Save checkpoint
            if (epoch % config_.save_interval == 0) {
                save_checkpoint(epoch);
            }
            
            // Print progress
            if (config_.verbose) {
                print_metrics();
            }
        }
        
        if (config_.verbose) {
            std::cout << "\n╔═══════════════════════════════════════════════╗\n";
            std::cout << "║  TRAINING COMPLETE                            ║\n";
            std::cout << "╚═══════════════════════════════════════════════╝\n\n";
            print_metrics();
        }
        
        return metrics_;
    }
    
    LearningMetrics run_epoch(int epoch_num) {
        LearningMetrics epoch_metrics;
        
        // Get all nodes for training
        auto nodes = storage_->get_all_nodes();
        
        if (nodes.empty()) {
            return epoch_metrics;
        }
        
        // Train GNN if available
        if (gnn_) {
            // Initialize embeddings if first epoch
            if (epoch_num == 1) {
                gnn_->initialize_embeddings(nodes);
            }
            
            // Message passing
            auto edges = storage_->get_all_edges();
            gnn_->message_pass(nodes, edges, storage_);
            
            // Predict and train
            std::vector<float> actual_activations;
            for (const auto& node : nodes) {
                actual_activations.push_back(node.activation);
            }
            
            auto predicted = gnn_->predict_activations(nodes);
            gnn_->train(nodes, actual_activations);
            
            // Create LEAPs from prediction errors
            int leaps_from_errors = gnn_->create_leaps_from_errors(
                nodes, predicted, actual_activations, storage_
            );
            
            if (config_.verbose && leaps_from_errors > 0) {
                std::cout << "[Autonomous] Created " << leaps_from_errors 
                          << " LEAPs from GNN prediction errors\n";
            }
        }
        
        // Compute metrics
        compute_graph_metrics(epoch_metrics);
        
        return epoch_metrics;
    }
    
    float train_batch(const std::vector<std::string>& texts) {
        if (!predictor_) return 0.0f;
        
        // Extract graph contexts
        std::vector<std::vector<NodeID>> graph_contexts;
        for (const auto& text : texts) {
            // Get recent nodes as context
            auto recent_episodes = episodes_->recall_recent(5);
            std::vector<NodeID> context;
            for (const auto& ep : recent_episodes) {
                context.insert(context.end(), ep.node_sequence.begin(), ep.node_sequence.end());
            }
            graph_contexts.push_back(context);
        }
        
        // Train hybrid predictor
        return predictor_->train_batch(texts, graph_contexts);
    }
    
    std::vector<std::string> generate_knowledge_gaps() {
        std::vector<std::string> gaps;
        
        if (!storage_) return gaps;
        
        // Strategy 1: Find highly connected nodes without certain relations
        auto nodes = storage_->get_all_nodes();
        for (const auto& node : nodes) {
            auto outgoing = storage_->get_edges_from(node.id);
            auto incoming = storage_->get_edges_to(node.id);
            
            // If many connections but low LEAP ratio, might be missing patterns
            size_t exact_count = 0, leap_count = 0;
            for (const auto& edge : outgoing) {
                if (edge.relation == RelationType::EXACT) exact_count++;
                else leap_count++;
            }
            
            if (exact_count > 5 && leap_count == 0) {
                // This concept might have undiscovered patterns
                std::string content = storage_->get_node_content(node.id);
                gaps.push_back("Explore patterns related to: " + content);
            }
        }
        
        // Strategy 2: Find sparse regions in the graph
        // (Areas with few connections)
        
        // Strategy 3: Identify contradictions or ambiguities
        
        // Limit to reasonable number
        if (gaps.size() > 10) {
            gaps.resize(10);
        }
        
        if (config_.verbose && !gaps.empty()) {
            std::cout << "[Autonomous] Identified " << gaps.size() << " knowledge gaps\n";
        }
        
        return gaps;
    }
    
    void explore_gap(const std::string& topic) {
        // In a full implementation, this would:
        // 1. Generate queries related to the topic
        // 2. Use the hybrid predictor to explore possible connections
        // 3. Create tentative LEAP connections
        // 4. Validate through reasoning
        
        if (config_.verbose) {
            std::cout << "[Autonomous] Exploring: " << topic << "\n";
        }
        
        // For now, just trigger LEAP creation
        create_leaps();
    }
    
    int create_leaps() {
        if (!storage_) return 0;
        
        // Use LEAP inference system
        leap::LEAPInference::Config leap_config;
        leap_config.max_transitive_hops = 3;
        leap_config.min_shared_neighbors = 2;
        leap_config.leap_confidence = 0.7f;
        leap_config.verbose = config_.verbose;
        
        leap::LEAPInference leap_system(leap_config);
        int leaps_created = leap_system.create_leap_connections(storage_);
        
        if (config_.verbose && leaps_created > 0) {
            std::cout << "[Autonomous] Created " << leaps_created << " new LEAP connections\n";
        }
        
        return leaps_created;
    }
    
    LearningMetrics evaluate(const std::vector<std::string>& validation_texts) {
        LearningMetrics eval_metrics;
        
        // Compute metrics on validation set
        compute_graph_metrics(eval_metrics);
        
        // Additional validation: prediction accuracy
        // (Would compare predictions vs actual next tokens)
        
        return eval_metrics;
    }
    
    void compute_graph_metrics(LearningMetrics& metrics) {
        if (!storage_) return;
        
        metrics.nodes_count = storage_->node_count();
        metrics.edges_count = storage_->edge_count();
        metrics.exact_edges = storage_->edge_count_by_type(RelationType::EXACT);
        metrics.leap_edges = storage_->edge_count_by_type(RelationType::LEAP);
        
        // Graph density: edges / (nodes * (nodes-1))
        if (metrics.nodes_count > 1) {
            float max_edges = metrics.nodes_count * (metrics.nodes_count - 1);
            metrics.graph_density = metrics.edges_count / max_edges;
        }
        
        // Episode coherence
        if (episodes_) {
            auto stats = episodes_->get_stats();
            metrics.episode_coherence = stats.avg_episode_size > 0 ? 1.0f : 0.0f;
        }
        
        // Get predictor stats if available
        if (predictor_) {
            auto pred_stats = predictor_->get_stats();
            metrics.prediction_accuracy = pred_stats.avg_combined_confidence;
        }
    }
    
    bool save_checkpoint(int epoch) {
        std::filesystem::create_directories(config_.checkpoint_dir);
        
        std::string checkpoint_path = config_.checkpoint_dir + "epoch_" + std::to_string(epoch);
        
        if (config_.verbose) {
            std::cout << "[Autonomous] Saving checkpoint: " << checkpoint_path << "\n";
        }
        
        // Save graph state
        if (storage_) {
            storage_->create_snapshot(checkpoint_path + "_graph");
        }
        
        // Save episodes
        if (episodes_) {
            episodes_->save(checkpoint_path + "_episodes.bin");
        }
        
        // Save metrics
        std::ofstream metrics_file(checkpoint_path + "_metrics.txt");
        if (metrics_file) {
            metrics_file << "Epoch: " << epoch << "\n";
            metrics_file << "Nodes: " << metrics_.nodes_count << "\n";
            metrics_file << "Edges: " << metrics_.edges_count << "\n";
            metrics_file << "EXACT: " << metrics_.exact_edges << "\n";
            metrics_file << "LEAP: " << metrics_.leap_edges << "\n";
            metrics_file << "Density: " << metrics_.graph_density << "\n";
            metrics_file << "Perplexity: " << metrics_.perplexity << "\n";
        }
        
        return true;
    }
    
    bool load_checkpoint(int epoch) {
        std::string checkpoint_path = config_.checkpoint_dir + "epoch_" + std::to_string(epoch);
        
        if (config_.verbose) {
            std::cout << "[Autonomous] Loading checkpoint: " << checkpoint_path << "\n";
        }
        
        // Load graph state
        if (storage_) {
            if (!storage_->restore_snapshot(checkpoint_path + "_graph")) {
                return false;
            }
        }
        
        // Load episodes
        if (episodes_) {
            if (!episodes_->load(checkpoint_path + "_episodes.bin")) {
                return false;
            }
        }
        
        current_epoch_ = epoch;
        return true;
    }
    
    void print_metrics() const {
        std::cout << "\n╔═══════════════════════════════════════════════╗\n";
        std::cout << "║  LEARNING METRICS                             ║\n";
        std::cout << "╚═══════════════════════════════════════════════╝\n";
        std::cout << "  Nodes: " << metrics_.nodes_count << "\n";
        std::cout << "  Edges: " << metrics_.edges_count 
                  << " (EXACT: " << metrics_.exact_edges 
                  << ", LEAP: " << metrics_.leap_edges << ")\n";
        std::cout << "  Graph density: " << (metrics_.graph_density * 100.0f) << "%\n";
        std::cout << "  Perplexity: " << metrics_.perplexity << "\n";
        std::cout << "  LEAP formation rate: " << metrics_.leap_formation_rate << "/epoch\n";
        std::cout << "  Prediction accuracy: " << metrics_.prediction_accuracy << "\n";
        std::cout << "═══════════════════════════════════════════════\n\n";
    }
};

// ============================================================================
// Public Interface
// ============================================================================

AutonomousLearner::AutonomousLearner(
    Storage* storage,
    episodic::EpisodicMemory* episodes,
    hybrid::HybridPredictor* predictor,
    dataset::DatasetLoader* loader,
    gnn::GNNPredictor* gnn,
    const Config& config
) : impl_(std::make_unique<Impl>(storage, episodes, predictor, loader, gnn, config)) {}

AutonomousLearner::~AutonomousLearner() = default;

AutonomousLearner::LearningMetrics AutonomousLearner::run_training(
    const dataset::DatasetLoader::Curriculum& curriculum
) {
    return impl_->run_training(curriculum);
}

AutonomousLearner::LearningMetrics AutonomousLearner::run_epoch(int epoch_num) {
    return impl_->run_epoch(epoch_num);
}

float AutonomousLearner::train_batch(const std::vector<std::string>& texts) {
    return impl_->train_batch(texts);
}

std::vector<std::string> AutonomousLearner::generate_knowledge_gaps() {
    return impl_->generate_knowledge_gaps();
}

void AutonomousLearner::explore_gap(const std::string& topic) {
    impl_->explore_gap(topic);
}

int AutonomousLearner::create_leaps() {
    return impl_->create_leaps();
}

AutonomousLearner::LearningMetrics AutonomousLearner::evaluate(
    const std::vector<std::string>& validation_texts
) {
    return impl_->evaluate(validation_texts);
}

void AutonomousLearner::compute_graph_metrics(LearningMetrics& metrics) {
    impl_->compute_graph_metrics(metrics);
}

bool AutonomousLearner::save_checkpoint(int epoch) {
    return impl_->save_checkpoint(epoch);
}

bool AutonomousLearner::load_checkpoint(int epoch) {
    return impl_->load_checkpoint(epoch);
}

void AutonomousLearner::set_config(const Config& config) {
    impl_->config_ = config;
}

const AutonomousLearner::Config& AutonomousLearner::get_config() const {
    return impl_->config_;
}

AutonomousLearner::LearningMetrics AutonomousLearner::get_metrics() const {
    return impl_->metrics_;
}

void AutonomousLearner::print_metrics() const {
    impl_->print_metrics();
}

void AutonomousLearner::reset_metrics() {
    impl_->metrics_ = LearningMetrics();
}

} // namespace autonomous
} // namespace melvin

