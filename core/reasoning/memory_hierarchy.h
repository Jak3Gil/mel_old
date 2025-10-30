#ifndef MEMORY_HIERARCHY_H
#define MEMORY_HIERARCHY_H

#include "spreading_activation.h"
#include <deque>
#include <vector>
#include <unordered_map>

namespace melvin {
namespace reasoning {

class MemoryHierarchy {
public:
    MemoryHierarchy();
    
    // Working memory (recent activations)
    void add_to_working_memory(const std::vector<int>& sequence);
    const std::deque<std::vector<int>>& get_working_memory() const { return working_memory_; }
    
    // Episodic traces (for consolidation)
    void record_episode(const std::vector<int>& activation_sequence);
    const std::deque<std::vector<int>>& get_episodes() const { return episodic_traces_; }
    
    // Context subgraph building
    std::unordered_map<int, std::vector<std::pair<int, float>>> build_context_subgraph(
        const std::vector<float>& query_embedding,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        int top_k = 20
    );
    
    // Activation field accessor
    ActivationField& activation_field() { return activation_field_; }
    const ActivationField& activation_field() const { return activation_field_; }
    
private:
    std::deque<std::vector<int>> working_memory_;    // Last 10 sequences
    std::deque<std::vector<int>> episodic_traces_;   // Last 100 episodes
    ActivationField activation_field_;
};

} // namespace reasoning
} // namespace melvin

#endif // MEMORY_HIERARCHY_H
