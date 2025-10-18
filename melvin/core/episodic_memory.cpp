#include "episodic_memory.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <cstring>

namespace melvin {
namespace episodic {

// ============================================================================
// Config Implementation
// ============================================================================

EpisodicMemory::Config::Config() {}

// ============================================================================
// Implementation Class
// ============================================================================

class EpisodicMemory::Impl {
public:
    Config config_;
    Storage* storage_ = nullptr;
    
    // Episode storage
    std::vector<Episode> episodes_;
    uint32_t next_episode_id_ = 1;
    uint64_t last_consolidation_time_ = 0;
    
    // Statistics
    Stats stats_;
    
    Impl(Storage* storage, const Config& config)
        : config_(config), storage_(storage) {
        last_consolidation_time_ = get_timestamp_ms();
    }
    
    uint32_t create_episode(const std::string& context) {
        Episode ep;
        ep.id = next_episode_id_++;
        ep.start_time = get_timestamp_ms();
        ep.context = context;
        ep.is_active = true;
        
        episodes_.push_back(ep);
        stats_.total_episodes++;
        stats_.active_episodes++;
        
        if (config_.verbose) {
            std::cout << "[EpisodicMemory] Created episode " << ep.id 
                      << " with context: " << context << std::endl;
        }
        
        return ep.id;
    }
    
    void end_episode(uint32_t episode_id) {
        for (auto& ep : episodes_) {
            if (ep.id == episode_id && ep.is_active) {
                ep.end_time = get_timestamp_ms();
                ep.is_active = false;
                stats_.active_episodes--;
                
                if (config_.verbose) {
                    std::cout << "[EpisodicMemory] Ended episode " << episode_id 
                              << " (duration: " << ep.duration() << "ms)" << std::endl;
                }
                break;
            }
        }
        
        // Check if consolidation is needed
        if (config_.auto_consolidate) {
            uint64_t now = get_timestamp_ms();
            if (now - last_consolidation_time_ > config_.consolidation_interval_ms) {
                consolidate();
                last_consolidation_time_ = now;
            }
        }
    }
    
    void add_node(uint32_t episode_id, NodeID node_id) {
        for (auto& ep : episodes_) {
            if (ep.id == episode_id && ep.is_active) {
                ep.node_sequence.push_back(node_id);
                stats_.total_nodes_in_episodes++;
                break;
            }
        }
    }
    
    void add_edge(uint32_t episode_id, EdgeID edge_id) {
        for (auto& ep : episodes_) {
            if (ep.id == episode_id && ep.is_active) {
                ep.edge_sequence.push_back(edge_id);
                break;
            }
        }
    }
    
    bool get_episode(uint32_t episode_id, Episode& out) const {
        for (const auto& ep : episodes_) {
            if (ep.id == episode_id) {
                out = ep;
                return true;
            }
        }
        return false;
    }
    
    std::vector<Episode> get_all_episodes() const {
        return episodes_;
    }
    
    std::vector<Episode> recall_timerange(uint64_t start_time, uint64_t end_time) const {
        std::vector<Episode> result;
        for (const auto& ep : episodes_) {
            if (ep.start_time >= start_time && ep.start_time <= end_time) {
                result.push_back(ep);
            }
        }
        
        // Sort by start time
        std::sort(result.begin(), result.end(), 
                  [](const Episode& a, const Episode& b) {
                      return a.start_time < b.start_time;
                  });
        
        return result;
    }
    
    std::vector<Episode> recall_similar(const std::vector<NodeID>& context, size_t top_k) const {
        // Convert context to set for fast lookup
        std::unordered_set<NodeID> context_set(context.begin(), context.end());
        
        // Compute similarity scores for all episodes
        struct EpisodeScore {
            Episode episode;
            float score;
        };
        
        std::vector<EpisodeScore> scored_episodes;
        for (const auto& ep : episodes_) {
            // Count overlap
            size_t overlap = 0;
            for (NodeID node : ep.node_sequence) {
                if (context_set.count(node) > 0) {
                    overlap++;
                }
            }
            
            // Jaccard similarity: |A ∩ B| / |A ∪ B|
            size_t union_size = context.size() + ep.node_sequence.size() - overlap;
            float score = union_size > 0 ? (float)overlap / union_size : 0.0f;
            
            if (score >= config_.similarity_threshold) {
                scored_episodes.push_back({ep, score});
            }
        }
        
        // Sort by score descending
        std::sort(scored_episodes.begin(), scored_episodes.end(),
                  [](const EpisodeScore& a, const EpisodeScore& b) {
                      return a.score > b.score;
                  });
        
        // Return top-k
        std::vector<Episode> result;
        for (size_t i = 0; i < std::min(top_k, scored_episodes.size()); i++) {
            result.push_back(scored_episodes[i].episode);
        }
        
        return result;
    }
    
    std::vector<Episode> recall_recent(size_t count) const {
        std::vector<Episode> sorted = episodes_;
        
        // Sort by start time descending (most recent first)
        std::sort(sorted.begin(), sorted.end(),
                  [](const Episode& a, const Episode& b) {
                      return a.start_time > b.start_time;
                  });
        
        // Return first 'count' episodes
        if (sorted.size() > count) {
            sorted.resize(count);
        }
        
        return sorted;
    }
    
    ReasoningPath reconstruct_episode(uint32_t episode_id) const {
        ReasoningPath path;
        
        for (const auto& ep : episodes_) {
            if (ep.id == episode_id) {
                path.nodes = ep.node_sequence;
                path.edges = ep.edge_sequence;
                path.confidence = 1.0f;  // Episode is exact memory
                return path;
            }
        }
        
        return path;
    }
    
    void consolidate() {
        if (config_.verbose) {
            std::cout << "[EpisodicMemory] Starting consolidation..." << std::endl;
        }
        
        // Find similar episodes and merge them
        std::vector<bool> merged(episodes_.size(), false);
        std::vector<Episode> consolidated;
        
        for (size_t i = 0; i < episodes_.size(); i++) {
            if (merged[i] || episodes_[i].is_active) continue;
            
            Episode base = episodes_[i];
            std::unordered_set<NodeID> base_nodes(base.node_sequence.begin(), 
                                                   base.node_sequence.end());
            
            // Find episodes to merge with base
            for (size_t j = i + 1; j < episodes_.size(); j++) {
                if (merged[j] || episodes_[j].is_active) continue;
                
                // Check similarity
                size_t overlap = 0;
                for (NodeID node : episodes_[j].node_sequence) {
                    if (base_nodes.count(node) > 0) {
                        overlap++;
                    }
                }
                
                size_t union_size = base.node_sequence.size() + 
                                    episodes_[j].node_sequence.size() - overlap;
                float similarity = union_size > 0 ? (float)overlap / union_size : 0.0f;
                
                if (similarity >= config_.similarity_threshold) {
                    // Merge j into base
                    for (NodeID node : episodes_[j].node_sequence) {
                        if (base_nodes.count(node) == 0) {
                            base.node_sequence.push_back(node);
                            base_nodes.insert(node);
                        }
                    }
                    
                    for (EdgeID edge : episodes_[j].edge_sequence) {
                        base.edge_sequence.push_back(edge);
                    }
                    
                    // Update time range
                    base.start_time = std::min(base.start_time, episodes_[j].start_time);
                    base.end_time = std::max(base.end_time, episodes_[j].end_time);
                    
                    merged[j] = true;
                }
            }
            
            consolidated.push_back(base);
            merged[i] = true;
        }
        
        // Keep active episodes
        for (size_t i = 0; i < episodes_.size(); i++) {
            if (!merged[i] && episodes_[i].is_active) {
                consolidated.push_back(episodes_[i]);
            }
        }
        
        size_t before = episodes_.size();
        episodes_ = consolidated;
        size_t after = episodes_.size();
        
        stats_.consolidations_performed++;
        
        if (config_.verbose) {
            std::cout << "[EpisodicMemory] Consolidated " << before 
                      << " episodes into " << after << std::endl;
        }
        
        // Prune if needed
        if (episodes_.size() > config_.max_episodes) {
            prune_old_episodes();
        }
    }
    
    void prune_old_episodes() {
        if (episodes_.size() <= config_.max_episodes) return;
        
        // Sort by start time (oldest first)
        std::sort(episodes_.begin(), episodes_.end(),
                  [](const Episode& a, const Episode& b) {
                      return a.start_time < b.start_time;
                  });
        
        // Remove oldest episodes, keeping active ones
        size_t to_remove = episodes_.size() - config_.max_episodes;
        auto it = episodes_.begin();
        while (to_remove > 0 && it != episodes_.end()) {
            if (!it->is_active) {
                it = episodes_.erase(it);
                to_remove--;
                stats_.total_episodes--;
            } else {
                ++it;
            }
        }
        
        if (config_.verbose) {
            std::cout << "[EpisodicMemory] Pruned to " << episodes_.size() 
                      << " episodes" << std::endl;
        }
    }
    
    Stats get_stats() const {
        Stats s = stats_;
        
        if (!episodes_.empty()) {
            s.oldest_episode_time = episodes_.front().start_time;
            s.newest_episode_time = episodes_.back().start_time;
            
            for (const auto& ep : episodes_) {
                s.oldest_episode_time = std::min(s.oldest_episode_time, ep.start_time);
                s.newest_episode_time = std::max(s.newest_episode_time, ep.start_time);
            }
            
            size_t total_size = 0;
            for (const auto& ep : episodes_) {
                total_size += ep.size();
            }
            s.avg_episode_size = (float)total_size / episodes_.size();
        }
        
        return s;
    }
    
    bool save(const std::string& path) const {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        
        // Write header
        uint32_t version = 1;
        uint32_t num_episodes = episodes_.size();
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&num_episodes), sizeof(num_episodes));
        
        // Write episodes
        for (const auto& ep : episodes_) {
            file.write(reinterpret_cast<const char*>(&ep.id), sizeof(ep.id));
            file.write(reinterpret_cast<const char*>(&ep.start_time), sizeof(ep.start_time));
            file.write(reinterpret_cast<const char*>(&ep.end_time), sizeof(ep.end_time));
            file.write(reinterpret_cast<const char*>(&ep.is_active), sizeof(ep.is_active));
            
            // Write context string
            uint32_t context_len = ep.context.size();
            file.write(reinterpret_cast<const char*>(&context_len), sizeof(context_len));
            file.write(ep.context.data(), context_len);
            
            // Write node sequence
            uint32_t node_count = ep.node_sequence.size();
            file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
            file.write(reinterpret_cast<const char*>(ep.node_sequence.data()), 
                      node_count * sizeof(NodeID));
            
            // Write edge sequence
            uint32_t edge_count = ep.edge_sequence.size();
            file.write(reinterpret_cast<const char*>(&edge_count), sizeof(edge_count));
            file.write(reinterpret_cast<const char*>(ep.edge_sequence.data()), 
                      edge_count * sizeof(EdgeID));
        }
        
        return true;
    }
    
    bool load(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        
        // Read header
        uint32_t version, num_episodes;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        file.read(reinterpret_cast<char*>(&num_episodes), sizeof(num_episodes));
        
        if (version != 1) return false;
        
        episodes_.clear();
        episodes_.reserve(num_episodes);
        
        // Read episodes
        for (uint32_t i = 0; i < num_episodes; i++) {
            Episode ep;
            file.read(reinterpret_cast<char*>(&ep.id), sizeof(ep.id));
            file.read(reinterpret_cast<char*>(&ep.start_time), sizeof(ep.start_time));
            file.read(reinterpret_cast<char*>(&ep.end_time), sizeof(ep.end_time));
            file.read(reinterpret_cast<char*>(&ep.is_active), sizeof(ep.is_active));
            
            // Read context string
            uint32_t context_len;
            file.read(reinterpret_cast<char*>(&context_len), sizeof(context_len));
            ep.context.resize(context_len);
            file.read(&ep.context[0], context_len);
            
            // Read node sequence
            uint32_t node_count;
            file.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
            ep.node_sequence.resize(node_count);
            file.read(reinterpret_cast<char*>(ep.node_sequence.data()), 
                     node_count * sizeof(NodeID));
            
            // Read edge sequence
            uint32_t edge_count;
            file.read(reinterpret_cast<char*>(&edge_count), sizeof(edge_count));
            ep.edge_sequence.resize(edge_count);
            file.read(reinterpret_cast<char*>(ep.edge_sequence.data()), 
                     edge_count * sizeof(EdgeID));
            
            episodes_.push_back(ep);
            
            if (ep.is_active) {
                stats_.active_episodes++;
            }
            next_episode_id_ = std::max(next_episode_id_, ep.id + 1);
        }
        
        stats_.total_episodes = episodes_.size();
        
        return true;
    }
};

// ============================================================================
// Public Interface
// ============================================================================

EpisodicMemory::EpisodicMemory(Storage* storage, const Config& config)
    : impl_(std::make_unique<Impl>(storage, config)) {}

EpisodicMemory::~EpisodicMemory() = default;

uint32_t EpisodicMemory::create_episode(const std::string& context) {
    return impl_->create_episode(context);
}

void EpisodicMemory::end_episode(uint32_t episode_id) {
    impl_->end_episode(episode_id);
}

void EpisodicMemory::add_node(uint32_t episode_id, NodeID node_id) {
    impl_->add_node(episode_id, node_id);
}

void EpisodicMemory::add_edge(uint32_t episode_id, EdgeID edge_id) {
    impl_->add_edge(episode_id, edge_id);
}

bool EpisodicMemory::get_episode(uint32_t episode_id, Episode& out) const {
    return impl_->get_episode(episode_id, out);
}

std::vector<Episode> EpisodicMemory::get_all_episodes() const {
    return impl_->get_all_episodes();
}

std::vector<Episode> EpisodicMemory::recall_timerange(uint64_t start_time, uint64_t end_time) const {
    return impl_->recall_timerange(start_time, end_time);
}

std::vector<Episode> EpisodicMemory::recall_similar(const std::vector<NodeID>& context, size_t top_k) const {
    return impl_->recall_similar(context, top_k);
}

std::vector<Episode> EpisodicMemory::recall_recent(size_t count) const {
    return impl_->recall_recent(count);
}

ReasoningPath EpisodicMemory::reconstruct_episode(uint32_t episode_id) const {
    return impl_->reconstruct_episode(episode_id);
}

void EpisodicMemory::consolidate() {
    impl_->consolidate();
}

void EpisodicMemory::prune_old_episodes() {
    impl_->prune_old_episodes();
}

EpisodicMemory::Stats EpisodicMemory::get_stats() const {
    return impl_->get_stats();
}

void EpisodicMemory::print_stats() const {
    auto stats = get_stats();
    std::cout << "\n=== Episodic Memory Statistics ===\n";
    std::cout << "Total episodes: " << stats.total_episodes << "\n";
    std::cout << "Active episodes: " << stats.active_episodes << "\n";
    std::cout << "Average episode size: " << stats.avg_episode_size << " nodes\n";
    std::cout << "Total nodes in episodes: " << stats.total_nodes_in_episodes << "\n";
    std::cout << "Consolidations performed: " << stats.consolidations_performed << "\n";
    
    if (stats.total_episodes > 0) {
        uint64_t now = get_timestamp_ms();
        uint64_t age_ms = now - stats.oldest_episode_time;
        std::cout << "Oldest episode age: " << (age_ms / 1000) << " seconds\n";
    }
    
    std::cout << "==================================\n\n";
}

bool EpisodicMemory::save(const std::string& path) const {
    return impl_->save(path);
}

bool EpisodicMemory::load(const std::string& path) {
    return impl_->load(path);
}

void EpisodicMemory::set_storage(Storage* storage) {
    impl_->storage_ = storage;
}

} // namespace episodic
} // namespace melvin

