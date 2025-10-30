#include "output_generator.h"
#include <random>
#include <algorithm>
#include <cmath>

namespace melvin {
namespace reasoning {

OutputGenerator::OutputGenerator(int embedding_dim)
    : embedding_dim_(embedding_dim)
{
}

std::vector<int> OutputGenerator::generate(
    const std::vector<int>& start_nodes,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    ActivationField& activation_field,
    int max_length,
    float temperature
) {
    if (start_nodes.empty()) {
        return {};
    }
    
    std::vector<int> output = start_nodes;
    int current = start_nodes.back();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int step = 0; step < max_length; ++step) {
        // Get neighbors
        auto it = graph.find(current);
        if (it == graph.end() || it->second.empty()) {
            break;
        }
        
        // Score candidates
        std::vector<std::pair<int, float>> candidates;
        for (const auto& edge : it->second) {
            int neighbor_id = edge.first;
            float edge_weight = edge.second;
            
            // Skip if already in output
            if (std::find(output.begin(), output.end(), neighbor_id) != output.end()) {
                continue;
            }
            
            float activation = activation_field.get_activation(neighbor_id);
            float score = edge_weight * (1.0f + activation);
            
            candidates.emplace_back(neighbor_id, score);
        }
        
        if (candidates.empty()) {
            break;
        }
        
        // Apply temperature
        std::vector<float> scores;
        for (const auto& cand : candidates) {
            scores.push_back(cand.second / temperature);
        }
        
        // Softmax
        float max_score = *std::max_element(scores.begin(), scores.end());
        float sum = 0.0f;
        for (float& score : scores) {
            score = std::exp(score - max_score);
            sum += score;
        }
        for (float& score : scores) {
            score /= sum;
        }
        
        // Sample
        std::discrete_distribution<> dist(scores.begin(), scores.end());
        int idx = dist(gen);
        
        current = candidates[idx].first;
        output.push_back(current);
        
        // Stop if activation too low
        if (activation_field.get_activation(current) < 0.05f && step > 2) {
            break;
        }
    }
    
    return output;
}

} // namespace reasoning
} // namespace melvin
