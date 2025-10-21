#include "dataset_loader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace melvin {
namespace dataset {

// ============================================================================
// Config & Curriculum Implementation
// ============================================================================

DatasetLoader::Config::Config() {}

DatasetLoader::Curriculum DatasetLoader::Curriculum::factual_curriculum() {
    Curriculum curr;
    
    curr.stages.push_back({
        "simple_facts",
        {"simple_wiki", "basic_facts"},
        1000,
        "Simple factual knowledge"
    });
    
    curr.stages.push_back({
        "complex_facts",
        {"wikipedia", "scientific_texts"},
        5000,
        "Complex factual knowledge"
    });
    
    curr.stages.push_back({
        "reasoning",
        {"logical_texts", "explanations"},
        5000,
        "Reasoning and explanations"
    });
    
    return curr;
}

DatasetLoader::Curriculum DatasetLoader::Curriculum::linguistic_curriculum() {
    Curriculum curr;
    
    curr.stages.push_back({
        "basic_language",
        {"children_books", "simple_sentences"},
        2000,
        "Basic linguistic patterns"
    });
    
    curr.stages.push_back({
        "literature",
        {"books", "articles"},
        5000,
        "Complex language patterns"
    });
    
    curr.stages.push_back({
        "dialogue",
        {"conversations", "dialogue_texts"},
        3000,
        "Conversational patterns"
    });
    
    return curr;
}

DatasetLoader::Curriculum DatasetLoader::Curriculum::balanced_curriculum() {
    Curriculum curr;
    
    curr.stages.push_back({
        "foundation",
        {"wikipedia", "books"},
        3000,
        "Balanced foundation"
    });
    
    curr.stages.push_back({
        "specialization",
        {"scientific", "dialogue", "literature"},
        5000,
        "Specialized knowledge"
    });
    
    return curr;
}

// ============================================================================
// Implementation Class
// ============================================================================

class DatasetLoader::Impl {
public:
    Config config_;
    Storage* storage_;
    episodic::EpisodicMemory* episodes_;
    tokenizer::Tokenizer* tokenizer_;
    
    TrainingStats stats_;
    
    Impl(Storage* storage, episodic::EpisodicMemory* episodes, 
         tokenizer::Tokenizer* tokenizer, const Config& config)
        : config_(config), storage_(storage), episodes_(episodes), tokenizer_(tokenizer) {}
    
    size_t load_dataset(
        const std::string& dataset_name,
        const std::string& split,
        size_t max_samples
    ) {
        if (config_.verbose) {
            std::cout << "[DatasetLoader] Loading " << dataset_name 
                      << " (" << split << ")..." << std::endl;
        }
        
        // In a full implementation, this would call Python HuggingFace API
        // For now, look for local cache files
        std::string cache_path = config_.cache_dir + dataset_name + "_" + split + ".txt";
        
        if (std::filesystem::exists(cache_path)) {
            return load_from_file(cache_path);
        } else {
            if (config_.verbose) {
                std::cout << "[DatasetLoader] Dataset not found in cache: " << cache_path << std::endl;
                std::cout << "[DatasetLoader] Creating synthetic data..." << std::endl;
            }
            return create_synthetic_data(dataset_name, max_samples);
        }
    }
    
    size_t create_synthetic_data(const std::string& dataset_name, size_t count) {
        // Create some synthetic training data based on dataset name
        std::vector<std::string> texts;
        
        if (dataset_name.find("wiki") != std::string::npos) {
            texts.push_back("Fire produces heat and light.");
            texts.push_back("Heat causes things to warm up.");
            texts.push_back("Water flows downhill due to gravity.");
            texts.push_back("The sun is a star that provides energy.");
            texts.push_back("Plants use sunlight to make food through photosynthesis.");
        } else if (dataset_name.find("book") != std::string::npos) {
            texts.push_back("Once upon a time, there was a wise old owl.");
            texts.push_back("The quick brown fox jumps over the lazy dog.");
            texts.push_back("In the beginning, there was nothing but darkness.");
            texts.push_back("Stories teach us about life and wisdom.");
        } else {
            // Generic texts
            texts.push_back("Knowledge is power.");
            texts.push_back("Learning never stops.");
            texts.push_back("Understanding comes from experience.");
            texts.push_back("Words carry meaning and emotion.");
        }
        
        // Duplicate to reach count
        while (texts.size() < count) {
            for (size_t i = 0; i < texts.size() && texts.size() < count; i++) {
                texts.push_back(texts[i]);
            }
        }
        
        if (texts.size() > count) {
            texts.resize(count);
        }
        
        ingest_batch(texts);
        return texts.size();
    }
    
    size_t load_from_file(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            std::cerr << "[DatasetLoader] Failed to open: " << path << std::endl;
            return 0;
        }
        
        std::vector<std::string> texts;
        std::string line;
        std::string current_doc;
        
        while (std::getline(file, line)) {
            if (line.empty()) {
                if (!current_doc.empty()) {
                    texts.push_back(current_doc);
                    current_doc.clear();
                }
            } else {
                if (!current_doc.empty()) current_doc += " ";
                current_doc += line;
            }
            
            if (texts.size() >= config_.max_samples_per_dataset) {
                break;
            }
        }
        
        if (!current_doc.empty()) {
            texts.push_back(current_doc);
        }
        
        if (config_.verbose) {
            std::cout << "[DatasetLoader] Loaded " << texts.size() 
                      << " texts from " << path << std::endl;
        }
        
        ingest_batch(texts);
        return texts.size();
    }
    
    size_t load_datasets(const std::vector<std::string>& dataset_names) {
        size_t total = 0;
        for (const auto& name : dataset_names) {
            total += load_dataset(name, "train", config_.max_samples_per_dataset);
        }
        return total;
    }
    
    void ingest_batch(const std::vector<std::string>& texts) {
        for (const auto& text : texts) {
            ingest_text(text, "dataset_ingestion");
        }
    }
    
    void ingest_text(const std::string& text, const std::string& context) {
        if (!storage_ || !tokenizer_) return;
        
        // Create episode if enabled
        uint32_t episode_id = 0;
        if (config_.create_episodes && episodes_) {
            episode_id = episodes_->create_episode(context);
        }
        
        // Tokenize
        auto tokens = tokenizer_->encode(text);
        
        // Create EXACT node chain
        std::vector<NodeID> node_sequence;
        NodeID prev_node = 0;
        
        uint64_t timestamp = episodic::get_timestamp_ms();
        
        for (size_t i = 0; i < tokens.size(); i++) {
            uint32_t token_id = tokens[i];
            
            // Get or create node for this token
            NodeID node_id = tokenizer_->token_to_node(token_id);
            
            // Create node if it doesn't exist
            Node node;
            if (!storage_->get_node(node_id, node)) {
                std::string token_str = tokenizer_->get_token(token_id);
                node_id = storage_->create_node(token_str, NodeType::CONCEPT);
                stats_.nodes_created++;
                
                // Set temporal info
                if (storage_->get_node(node_id, node)) {
                    node.creation_timestamp = timestamp;
                    node.episode_id = episode_id;
                    storage_->update_node(node);
                }
            }
            
            node_sequence.push_back(node_id);
            
            // Add to episode
            if (episode_id > 0 && episodes_) {
                episodes_->add_node(episode_id, node_id);
            }
            
            // Create EXACT edge from previous token
            if (prev_node != 0) {
                EdgeID edge_id = storage_->create_edge(
                    prev_node, node_id, RelationType::EXACT, 1.0f
                );
                stats_.edges_created++;
                
                // Set temporal info
                Edge edge;
                if (storage_->get_edge(edge_id, edge)) {
                    edge.creation_timestamp = timestamp;
                    edge.episode_id = episode_id;
                    storage_->update_edge(edge);
                }
                
                // Add to episode
                if (episode_id > 0 && episodes_) {
                    episodes_->add_edge(episode_id, edge_id);
                }
            }
            
            prev_node = node_id;
        }
        
        // End episode
        if (episode_id > 0 && episodes_) {
            episodes_->end_episode(episode_id);
            stats_.episodes_created++;
        }
        
        stats_.total_texts++;
        stats_.total_tokens += tokens.size();
        stats_.avg_text_length = stats_.avg_text_length * 0.99f + tokens.size() * 0.01f;
    }
    
    TrainingStats train_curriculum(const Curriculum& curriculum) {
        if (config_.verbose) {
            std::cout << "\n[DatasetLoader] Starting curriculum training..." << std::endl;
            std::cout << "[DatasetLoader] Stages: " << curriculum.stages.size() << "\n" << std::endl;
        }
        
        for (size_t i = 0; i < curriculum.stages.size(); i++) {
            const auto& stage = curriculum.stages[i];
            
            if (config_.verbose) {
                std::cout << "[DatasetLoader] Stage " << (i+1) << "/" 
                          << curriculum.stages.size() << ": " << stage.name << std::endl;
                std::cout << "[DatasetLoader] Description: " << stage.description << "\n" << std::endl;
            }
            
            train_stage(stage);
            
            if (config_.verbose) {
                print_stats();
            }
        }
        
        if (config_.verbose) {
            std::cout << "\n[DatasetLoader] Curriculum training complete!\n" << std::endl;
        }
        
        return stats_;
    }
    
    TrainingStats train_stage(const Curriculum::Stage& stage) {
        TrainingStats stage_stats;
        
        for (const auto& dataset : stage.datasets) {
            size_t loaded = load_dataset(dataset, "train", stage.samples_per_dataset);
            stage_stats.total_texts += loaded;
        }
        
        return stage_stats;
    }
    
    std::vector<std::string> get_available_datasets() const {
        // Return predefined list + any cached datasets
        std::vector<std::string> datasets = {
            "wikipedia", "books", "simple_wiki", "articles",
            "scientific", "dialogue", "children_books"
        };
        
        // Check cache directory
        if (std::filesystem::exists(config_.cache_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(config_.cache_dir)) {
                if (entry.path().extension() == ".txt") {
                    datasets.push_back(entry.path().stem().string());
                }
            }
        }
        
        return datasets;
    }
    
    float estimate_difficulty(const std::string& dataset_name) const {
        // Simple heuristic based on name
        if (dataset_name.find("simple") != std::string::npos ||
            dataset_name.find("children") != std::string::npos) {
            return 1.0f;  // Easy
        }
        if (dataset_name.find("scientific") != std::string::npos ||
            dataset_name.find("technical") != std::string::npos) {
            return 5.0f;  // Hard
        }
        return 3.0f;  // Medium
    }
    
    void print_stats() const {
        std::cout << "\n=== Dataset Loader Statistics ===\n";
        std::cout << "Total texts: " << stats_.total_texts << "\n";
        std::cout << "Total tokens: " << stats_.total_tokens << "\n";
        std::cout << "Nodes created: " << stats_.nodes_created << "\n";
        std::cout << "Edges created: " << stats_.edges_created << "\n";
        std::cout << "Episodes created: " << stats_.episodes_created << "\n";
        std::cout << "Avg text length: " << stats_.avg_text_length << " tokens\n";
        std::cout << "==================================\n\n";
    }
};

// ============================================================================
// Public Interface
// ============================================================================

DatasetLoader::DatasetLoader(
    Storage* storage,
    episodic::EpisodicMemory* episodes,
    tokenizer::Tokenizer* tokenizer,
    const Config& config
) : impl_(std::make_unique<Impl>(storage, episodes, tokenizer, config)) {}

DatasetLoader::~DatasetLoader() = default;

size_t DatasetLoader::load_dataset(
    const std::string& dataset_name,
    const std::string& split,
    size_t max_samples
) {
    return impl_->load_dataset(dataset_name, split, max_samples);
}

size_t DatasetLoader::load_from_file(const std::string& path) {
    return impl_->load_from_file(path);
}

size_t DatasetLoader::load_datasets(const std::vector<std::string>& dataset_names) {
    return impl_->load_datasets(dataset_names);
}

void DatasetLoader::ingest_batch(const std::vector<std::string>& texts) {
    impl_->ingest_batch(texts);
}

void DatasetLoader::ingest_text(const std::string& text, const std::string& context) {
    impl_->ingest_text(text, context);
}

DatasetLoader::TrainingStats DatasetLoader::train_curriculum(const Curriculum& curriculum) {
    return impl_->train_curriculum(curriculum);
}

DatasetLoader::TrainingStats DatasetLoader::train_stage(const Curriculum::Stage& stage) {
    return impl_->train_stage(stage);
}

std::vector<std::string> DatasetLoader::get_available_datasets() const {
    return impl_->get_available_datasets();
}

float DatasetLoader::estimate_difficulty(const std::string& dataset_name) const {
    return impl_->estimate_difficulty(dataset_name);
}

DatasetLoader::TrainingStats DatasetLoader::get_stats() const {
    return impl_->stats_;
}

void DatasetLoader::print_stats() const {
    impl_->print_stats();
}

void DatasetLoader::reset_stats() {
    impl_->stats_ = TrainingStats();
}

} // namespace dataset
} // namespace melvin

