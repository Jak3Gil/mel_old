/**
 * @file cm_grounder.h
 */

#ifndef MELVIN_CROSSMODAL_CM_GROUNDER_H
#define MELVIN_CROSSMODAL_CM_GROUNDER_H

#include <string>
#include <vector>
#include <utility>
#include "cm_space.h"
#include "cm_index.h"
#include "cm_binding.h"

namespace melvin {
namespace crossmodal {

struct GroundingResult {
    std::vector<std::pair<std::string,float>> predictions; // key, confidence
};

class CMGrounder {
public:
    GroundingResult PredictVisionForConcept(int64_t concept_id, int k);
    GroundingResult PredictAudioForConcept(int64_t concept_id, int k);
    GroundingResult PredictMotorForConcept(int64_t concept_id, int k);

    std::vector<std::pair<int64_t,float>> PredictConceptForVision(const std::string& vision_key, int k);
    std::vector<std::pair<int64_t,float>> PredictConceptForAudio(const std::string& audio_key, int k);
    std::vector<std::pair<int64_t,float>> PredictConceptForMotor(const std::string& motor_schema_id, int k);

    void Reinforce(const Binding& b, float delta); // clamp in [0,1]

    // Expose indices for I/O loaders
    CMIndex& vision_index() { return vision_idx_; }
    CMIndex& audio_index() { return audio_idx_; }
    CMIndex& motor_index() { return motor_idx_; }
    CMBindings& bindings() { return bindings_; }

    template<typename F>
    void SetConceptEncoder(F fn) { enc_holder_.store(fn); concept_encoder_.set(&enc_holder_); }

    // Context and temporal smoothing controls
    void SetContextRelevance(const std::vector<std::pair<std::string,float>>& keys_with_weights);
    void NoteTemporalKey(const std::string& key, float strength = 1.0f);
    void DecayTemporal(float gamma = 0.9f);
    void SetWeights(float alpha_context, float beta_temporal, float temperature);

private:
    CMIndex vision_idx_;
    CMIndex audio_idx_;
    CMIndex motor_idx_;
    CMBindings bindings_;
    // type-erased callable
    struct ConceptEnc {
        CMVec (*call)(void*, int64_t) = nullptr;
        void* ctx = nullptr;
        template<typename F>
        void set(F* f) { call = [](void* c, int64_t id){ return (*(F*)c)(id); }; ctx = f; }
        CMVec operator()(int64_t id) const { return call ? call(ctx, id) : CMVec{}; }
        operator bool() const { return call != nullptr; }
    } concept_encoder_;

    // Holder for encoder functor (type-erased storage)
    struct EncHolder {
        CMVec (*fn)(int64_t) = nullptr;
        template<typename F>
        void store(F f){ (void)f; fn = +[](int64_t id){ return F{}(id); }; }
        CMVec operator()(int64_t id) const { return fn ? fn(id) : CMVec{}; }
    } enc_holder_;

    // Contextual and temporal maps
    std::unordered_map<std::string, float> context_relevance_;
    std::unordered_map<std::string, float> temporal_consistency_;
    float alpha_{0.3f};
    float beta_{0.3f};
    float temperature_{0.7f};
};

} // namespace crossmodal
} // namespace melvin

#endif // MELVIN_CROSSMODAL_CM_GROUNDER_H


