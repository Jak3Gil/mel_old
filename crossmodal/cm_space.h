/**
 * @file cm_space.h
 * @brief Shared cross-modal embedding space (deterministic, Jetson-safe)
 */

#ifndef MELVIN_CROSSMODAL_CM_SPACE_H
#define MELVIN_CROSSMODAL_CM_SPACE_H

#include <array>
#include <string>

namespace melvin {
namespace crossmodal {

struct CMVec {
    std::array<float, 256> v{};
};

class CMSpace {
public:
    static CMSpace& Instance();

    CMVec EncodeText(const std::string& label);
    CMVec EncodeVision(const std::string& vision_key);
    CMVec EncodeAudio(const std::string& audio_key);
    CMVec EncodeMotor(const std::string& motor_schema_id);

    float Cosine(const CMVec& a, const CMVec& b) const;
    void LoadCalib(const std::string& path);

    void SetSeed(uint64_t seed);

private:
    CMSpace() = default;
    CMVec encodeDeterministic(const std::string& key, uint64_t salt) const;
    uint64_t seed_{42};
};

} // namespace crossmodal
} // namespace melvin

#endif // MELVIN_CROSSMODAL_CM_SPACE_H


