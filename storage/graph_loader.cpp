/**
 * @file graph_loader.cpp
 */

#include "graph_loader.h"
#include <fstream>
#include <sstream>
#include <cstring>

namespace melvin {
namespace storage {

static inline bool parse_tsv_line(const std::string& line, std::vector<std::string>& cols) {
    cols.clear();
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, '\t')) cols.push_back(item);
    return !cols.empty();
}

bool GraphLoader::LoadNodesTSV(const std::string& path,
                               std::unordered_map<int, std::string>& id_to_label,
                               std::unordered_map<std::string, int>& label_to_id,
                               std::unordered_map<int, float>& priors) {
    std::ifstream f(path);
    if (!f.good()) return false;
    std::string line; std::vector<std::string> cols;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (!parse_tsv_line(line, cols) || cols.size() < 4) continue;
        int id = static_cast<int>(std::stoll(cols[0]));
        const std::string& label = cols[1];
        // type = cols[2]
        float prior = std::stof(cols[3]);
        id_to_label[id] = label;
        label_to_id[label] = id;
        priors[id] = prior;
    }
    return true;
}

bool GraphLoader::LoadEdgesTSV(const std::string& path,
                               std::unordered_map<int, std::vector<std::pair<int,float>>>& graph,
                               bool bidir) {
    std::ifstream f(path);
    if (!f.good()) return false;
    std::string line; std::vector<std::string> cols;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (!parse_tsv_line(line, cols) || cols.size() < 4) continue;
        int src = static_cast<int>(std::stoll(cols[0]));
        int dst = static_cast<int>(std::stoll(cols[1]));
        // rel = cols[2]
        float w = std::stof(cols[3]);
        graph[src].push_back({dst, w});
        if (bidir) graph[dst].push_back({src, w});
    }
    return true;
}

// Binary loaders
bool GraphLoader::LoadNodesBIN(const std::string& path,
                               std::unordered_map<int, std::string>& id_to_label,
                               std::unordered_map<std::string, int>& label_to_id,
                               std::unordered_map<int, float>& priors) {
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) return false;
    auto read_int32 = [&](int32_t& v){ f.read(reinterpret_cast<char*>(&v), sizeof(v)); return (bool)f; };
    auto read_float = [&](float& v){ f.read(reinterpret_cast<char*>(&v), sizeof(v)); return (bool)f; };
    int32_t N = 0;
    if (!read_int32(N) || N < 0) return false;
    id_to_label.reserve((size_t)N);
    priors.reserve((size_t)N);
    for (int32_t i = 0; i < N; ++i) {
        int32_t id = 0, len = 0; float prior = 0.f;
        if (!read_int32(id)) return false;
        if (!read_int32(len) || len < 0 || len > (1<<20)) return false;
        std::string label; label.resize((size_t)len);
        if (len > 0) f.read(&label[0], len);
        if (!read_float(prior)) return false;
        id_to_label[id] = label;
        label_to_id[label] = id;
        priors[id] = prior;
    }
    return true;
}

bool GraphLoader::LoadEdgesBIN(const std::string& path,
                               std::unordered_map<int, std::vector<std::pair<int,float>>>& graph,
                               bool bidir) {
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) return false;
    auto read_int32 = [&](int32_t& v){ f.read(reinterpret_cast<char*>(&v), sizeof(v)); return (bool)f; };
    auto read_float = [&](float& v){ f.read(reinterpret_cast<char*>(&v), sizeof(v)); return (bool)f; };
    int32_t M = 0;
    if (!read_int32(M) || M < 0) return false;
    for (int32_t i = 0; i < M; ++i) {
        int32_t src = 0, dst = 0; float w = 0.f;
        if (!read_int32(src)) return false;
        if (!read_int32(dst)) return false;
        if (!read_float(w)) return false;
        graph[src].push_back({dst, w});
        if (bidir) graph[dst].push_back({src, w});
    }
    return true;
}

} // namespace storage
} // namespace melvin


