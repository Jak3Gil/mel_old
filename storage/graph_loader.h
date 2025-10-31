/**
 * @file graph_loader.h
 */

#ifndef MELVIN_STORAGE_GRAPH_LOADER_H
#define MELVIN_STORAGE_GRAPH_LOADER_H

#include <string>
#include <unordered_map>
#include <vector>

namespace melvin {
namespace storage {

class GraphLoader {
public:
    bool LoadNodesTSV(const std::string& path,
                      std::unordered_map<int, std::string>& id_to_label,
                      std::unordered_map<std::string, int>& label_to_id,
                      std::unordered_map<int, float>& priors);

    bool LoadEdgesTSV(const std::string& path,
                      std::unordered_map<int, std::vector<std::pair<int,float>>>& graph,
                      bool bidir = true);

    // Unified binary loaders (compact, Jetson-friendly)
    // Binary format (little-endian):
    // nodes.bin: int32 N; repeat N times: int32 id; int32 label_len; bytes label; float prior
    // edges.bin: int32 M; repeat M times: int32 src; int32 dst; float weight
    bool LoadNodesBIN(const std::string& path,
                      std::unordered_map<int, std::string>& id_to_label,
                      std::unordered_map<std::string, int>& label_to_id,
                      std::unordered_map<int, float>& priors);

    bool LoadEdgesBIN(const std::string& path,
                      std::unordered_map<int, std::vector<std::pair<int,float>>>& graph,
                      bool bidir = true);
};

} // namespace storage
} // namespace melvin

#endif // MELVIN_STORAGE_GRAPH_LOADER_H


