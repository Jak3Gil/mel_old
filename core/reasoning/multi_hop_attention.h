#ifndef MULTI_HOP_ATTENTION_H
#define MULTI_HOP_ATTENTION_H

#include "spreading_activation.h"
#include <vector>
#include <unordered_map>

namespace melvin {
namespace reasoning {

class MultiHopAttention {
public:
    explicit MultiHopAttention(int embedding_dim = 128, int attention_heads = 4);
    
    struct QueryResult {
        int node_id;
        float attention_score;
        int hop_number;
    };
    
    std::vector<QueryResult> query(
        const std::vector<float>& query_embedding,
        ActivationField& activation_field,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        int max_hops = 100,
        float frontier_threshold = 0.05f
    );
    
private:
    int embedding_dim_;
    int attention_heads_;
    int head_dim_;
    
    float compute_attention(
        const std::vector<float>& query,
        const std::vector<float>& key,
        const std::vector<float>& value
    );
};

} // namespace reasoning
} // namespace melvin

#endif // MULTI_HOP_ATTENTION_H
