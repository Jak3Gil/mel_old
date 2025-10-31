/**
 * @file cm_binding.h
 */

#ifndef MELVIN_CROSSMODAL_CM_BINDING_H
#define MELVIN_CROSSMODAL_CM_BINDING_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace melvin {
namespace crossmodal {

struct Binding {
    int64_t concept_id;
    enum Modality { TEXT, VISION, AUDIO, MOTOR } mod;
    std::string key;    // vision_key / audio_key / motor_schema_id
    float weight;       // [0,1]
    std::string source; // provenance
};

class CMBindings {
public:
    void Upsert(const Binding& b);
    std::vector<Binding> ForConcept(int64_t id) const;
    std::vector<Binding> ForKey(const std::string& key) const;
    void PruneConcept(int64_t id, size_t max_keep = 64);

private:
    // concept_id -> list
    std::unordered_map<int64_t, std::vector<Binding>> by_concept_;
    // key -> list
    std::unordered_map<std::string, std::vector<Binding>> by_key_;
    mutable std::mutex mu_;
};

} // namespace crossmodal
} // namespace melvin

#endif // MELVIN_CROSSMODAL_CM_BINDING_H


