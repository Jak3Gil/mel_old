/**
 * @file cm_space.cpp
 */

#include "cm_space.h"
#include <cmath>
#include <cstdint>
#include <cstring>

namespace melvin {
namespace crossmodal {

static inline uint64_t splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    return x;
}

CMSpace& CMSpace::Instance() {
    static CMSpace inst;
    return inst;
}

void CMSpace::SetSeed(uint64_t seed) { seed_ = seed; }

void CMSpace::LoadCalib(const std::string&) { /* optional future use */ }

CMVec CMSpace::encodeDeterministic(const std::string& key, uint64_t salt) const {
    uint64_t h = seed_ ^ salt;
    // simple Fowler–Noll–Vo XOR variant combined with splitmix
    for (unsigned char c : key) {
        h = splitmix64(h ^ (uint64_t)c * 0x100000001b3ULL);
    }
    CMVec out;
    // Low-discrepancy projection into 256-D using sin/cos of hashed sequence
    for (size_t i = 0; i < out.v.size(); ++i) {
        uint64_t t = splitmix64(h + i * 0x9e3779b97f4a7c15ULL);
        double a = (double)(t & 0xFFFFFFFFULL) / (double)0xFFFFFFFFULL;
        double b = (double)(t >> 32) / (double)0xFFFFFFFFULL;
        float val = static_cast<float>(std::sin(a * 6.283185307179586 + b));
        out.v[i] = val;
    }
    // L2 normalize
    double norm = 0.0;
    for (float f : out.v) norm += (double)f * (double)f;
    norm = std::sqrt(std::max(1e-12, norm));
    for (float& f : out.v) f = static_cast<float>(f / norm);
    return out;
}

CMVec CMSpace::EncodeText(const std::string& label) { return encodeDeterministic(label, 0x01ULL); }
CMVec CMSpace::EncodeVision(const std::string& vision_key) { return encodeDeterministic(vision_key, 0x02ULL); }
CMVec CMSpace::EncodeAudio(const std::string& audio_key) { return encodeDeterministic(audio_key, 0x03ULL); }
CMVec CMSpace::EncodeMotor(const std::string& motor_schema_id) { return encodeDeterministic(motor_schema_id, 0x04ULL); }

float CMSpace::Cosine(const CMVec& a, const CMVec& b) const {
    double dot = 0.0;
    for (size_t i = 0; i < a.v.size(); ++i) dot += (double)a.v[i] * (double)b.v[i];
    return static_cast<float>(dot);
}

} // namespace crossmodal
} // namespace melvin


