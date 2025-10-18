/*
 * Learning System Implementation
 */

#include "learning.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace melvin {

// ============================================================================
// CONFIG
// ============================================================================

LearningSystem::Config::Config() {}

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class LearningSystem::Impl {
public:
    Storage* storage = nullptr;
    Config config;
    Stats stats;
    
    // Parse teaching file format
    struct TeachingEntry {
        std::string fact;
        std::string context;
        std::vector<std::pair<std::string, std::string>> edges;
    };
    
    std::vector<TeachingEntry> parse_teaching_file(const std::string& path) {
        std::vector<TeachingEntry> entries;
        std::ifstream file(path);
        
        if (!file.is_open()) {
            return entries;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            TeachingEntry entry;
            entry.fact = line;
            entries.push_back(entry);
        }
        
        return entries;
    }
};

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

LearningSystem::LearningSystem(Storage* storage, const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->storage = storage;
    impl_->config = config;
}

LearningSystem::~LearningSystem() = default;

bool LearningSystem::teach_file(const std::string& path) {
    if (!impl_->storage) {
        std::cerr << "Error: No storage configured\n";
        return false;
    }
    
    auto entries = impl_->parse_teaching_file(path);
    
    if (entries.empty()) {
        std::cerr << "Warning: No entries found in " << path << "\n";
        return false;
    }
    
    for (const auto& entry : entries) {
        teach_fact(entry.fact, entry.context);
    }
    
    impl_->stats.files_processed++;
    return true;
}

bool LearningSystem::teach_fact(const std::string& fact, const std::string& context) {
    if (!impl_->storage) return false;
    
    // Create node for fact
    auto node_id = impl_->storage->create_node(fact, NodeType::CONCEPT);
    
    // If context provided, create context node and link with EXACT
    if (!context.empty()) {
        auto context_id = impl_->storage->create_node(context, NodeType::CONCEPT);
        impl_->storage->create_edge(node_id, context_id, RelationType::EXACT, 1.0f);
    }
    
    impl_->stats.facts_learned++;
    return true;
}

bool LearningSystem::teach_batch(const std::vector<std::string>& paths) {
    bool all_success = true;
    for (const auto& path : paths) {
        if (!teach_file(path)) {
            all_success = false;
        }
    }
    return all_success;
}

void LearningSystem::reinforce_path(const ReasoningPath& path, float reward) {
    if (!impl_->storage) return;
    
    // Reinforce all edges in path
    for (const auto& edge_id : path.edges) {
        Edge edge;
        if (impl_->storage->get_edge(edge_id, edge)) {
            float new_weight = edge.get_weight() + impl_->config.learning_rate * reward;
            edge.set_weight(std::max(0.0f, std::min(1.0f, new_weight)));
            // Note: reinforcement_count now in sparse metadata
            impl_->storage->update_edge(edge);
        }
    }
    
    impl_->stats.reinforcements++;
}

void LearningSystem::decay_edges() {
    if (!impl_->storage) return;
    
    auto edges = impl_->storage->get_all_edges();
    for (auto& edge : edges) {
        float new_weight = edge.get_weight() * impl_->config.decay_rate;
        edge.set_weight(new_weight);
        impl_->storage->update_edge(edge);
    }
}

void LearningSystem::consolidate_memory() {
    // TODO: Implement memory consolidation
}

std::vector<EdgeID> LearningSystem::get_due_reviews() {
    // TODO: Implement SRS scheduling
    return {};
}

void LearningSystem::mark_reviewed(const EdgeID& edge_id, bool success) {
    // TODO: Implement SRS review tracking
}

void LearningSystem::update_variance_impacts(const std::vector<NodeID>& active_nodes, float fitness_delta) {
    // TODO: Implement variance impact tracking
}

void LearningSystem::promote_dimensions() {
    // TODO: Implement dimensional promotion
}

void LearningSystem::demote_dimensions() {
    // TODO: Implement dimensional demotion
}

void LearningSystem::set_storage(Storage* storage) {
    impl_->storage = storage;
}

void LearningSystem::set_config(const Config& config) {
    impl_->config = config;
}

const LearningSystem::Config& LearningSystem::get_config() const {
    return impl_->config;
}

LearningSystem::Stats LearningSystem::get_stats() const {
    return impl_->stats;
}

} // namespace melvin

