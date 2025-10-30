#ifndef OUTPUT_GENERATOR_H
#define OUTPUT_GENERATOR_H

#include "spreading_activation.h"
#include <vector>
#include <unordered_map>

namespace melvin {
namespace reasoning {

class OutputGenerator {
public:
    explicit OutputGenerator(int embedding_dim = 128);
    
    std::vector<int> generate(
        const std::vector<int>& start_nodes,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        ActivationField& activation_field,
        int max_length = 20,
        float temperature = 0.7f
    );
    
private:
    int embedding_dim_;
};

} // namespace reasoning
} // namespace melvin

#endif // OUTPUT_GENERATOR_H
