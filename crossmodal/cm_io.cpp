/**
 * @file cm_io.cpp
 */

#include "cm_io.h"
#include <fstream>
#include <sstream>

namespace melvin {
namespace crossmodal {

static inline bool parse_tsv_line(const std::string& line, std::vector<std::string>& cols) {
    cols.clear();
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, '\t')) cols.push_back(item);
    return !cols.empty();
}

bool CMIO::LoadVisionMap(const std::string& path, CMGrounder& g) {
    std::ifstream f(path);
    if (!f.good()) return false;
    std::string line; std::vector<std::string> cols;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (!parse_tsv_line(line, cols) || cols.size() < 3) continue;
        int64_t cid = std::stoll(cols[0]);
        const std::string& key = cols[1];
        float cs = std::stof(cols[2]);
        // index add
        g.vision_index().Add(key, CMSpace::Instance().EncodeVision(key));
        // seed binding
        Binding b{cid, Binding::VISION, key, std::max(0.0f, std::min(1.0f, cs)), "grounding"};
        g.bindings().Upsert(b);
    }
    return true;
}

bool CMIO::LoadAudioMap(const std::string& path, CMGrounder& g) {
    std::ifstream f(path);
    if (!f.good()) return false;
    std::string line; std::vector<std::string> cols;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (!parse_tsv_line(line, cols) || cols.size() < 3) continue;
        int64_t cid = std::stoll(cols[0]);
        const std::string& key = cols[1];
        float cs = std::stof(cols[2]);
        g.audio_index().Add(key, CMSpace::Instance().EncodeAudio(key));
        Binding b{cid, Binding::AUDIO, key, std::max(0.0f, std::min(1.0f, cs)), "grounding"};
        g.bindings().Upsert(b);
    }
    return true;
}

bool CMIO::LoadMotorMap(const std::string& path, CMGrounder& g) {
    std::ifstream f(path);
    if (!f.good()) return false;
    std::string line; std::vector<std::string> cols;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (!parse_tsv_line(line, cols) || cols.size() < 3) continue;
        int64_t cid = std::stoll(cols[0]);
        const std::string& key = cols[1];
        float cs = std::stof(cols[2]);
        g.motor_index().Add(key, CMSpace::Instance().EncodeMotor(key));
        Binding b{cid, Binding::MOTOR, key, std::max(0.0f, std::min(1.0f, cs)), "grounding"};
        g.bindings().Upsert(b);
    }
    return true;
}

bool CMIO::ExportBindingsTSV(const std::string& path, const CMBindings&) {
    // Minimal stub: append-only export can be added when needed
    (void)path;
    return true;
}

} // namespace crossmodal
} // namespace melvin


