/**
 * @file cm_grounder.cpp
 */

#include "cm_grounder.h"
#include <algorithm>
#include <functional>
#include <cmath>

namespace melvin {
namespace crossmodal {

static inline float clamp01(float x){ return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }

// Softmax with temperature over scored pairs
static inline void softmax_normalize(std::vector<std::pair<std::string,float>>& s, float temperature){
    if (s.empty()) return;
    double denom = 0.0; double maxv = -1e9;
    for (auto& p : s) maxv = std::max(maxv, (double)p.second);
    for (auto& p : s) denom += std::exp(((double)p.second - maxv) / std::max(1e-6, (double)temperature));
    for (auto& p : s) p.second = (float)(std::exp(((double)p.second - maxv) / std::max(1e-6, (double)temperature)) / std::max(1e-12, denom));
}

void CMGrounder::SetContextRelevance(const std::vector<std::pair<std::string,float>>& keys_with_weights){
    context_relevance_.clear();
    for (auto& kv : keys_with_weights) context_relevance_[kv.first] = kv.second;
}

void CMGrounder::NoteTemporalKey(const std::string& key, float strength){
    temporal_consistency_[key] = std::min(1.0f, temporal_consistency_[key] + strength);
}

void CMGrounder::DecayTemporal(float gamma){
    for (auto& kv : temporal_consistency_) kv.second *= gamma;
}

void CMGrounder::SetWeights(float alpha_context, float beta_temporal, float temperature){
    alpha_ = alpha_context; beta_ = beta_temporal; temperature_ = temperature;
}

GroundingResult CMGrounder::PredictVisionForConcept(int64_t concept_id, int k) {
    GroundingResult gr;
    if (!concept_encoder_) return gr;
    CMVec c = concept_encoder_(concept_id);
    gr.predictions = vision_idx_.TopK(c, k);
    // Apply contextual and temporal gating multiplicatively
    for (auto& p : gr.predictions) {
        float ctx = context_relevance_.count(p.first) ? context_relevance_[p.first] : 0.0f;
        float tmp = temporal_consistency_.count(p.first) ? temporal_consistency_[p.first] : 0.0f;
        p.second = p.second * (1.0f + alpha_ * ctx + beta_ * tmp);
    }
    softmax_normalize(gr.predictions, temperature_);
    return gr;
}

GroundingResult CMGrounder::PredictAudioForConcept(int64_t concept_id, int k) {
    GroundingResult gr;
    if (!concept_encoder_) return gr;
    CMVec c = concept_encoder_(concept_id);
    gr.predictions = audio_idx_.TopK(c, k);
    for (auto& p : gr.predictions) {
        float ctx = context_relevance_.count(p.first) ? context_relevance_[p.first] : 0.0f;
        float tmp = temporal_consistency_.count(p.first) ? temporal_consistency_[p.first] : 0.0f;
        p.second = p.second * (1.0f + alpha_ * ctx + beta_ * tmp);
    }
    softmax_normalize(gr.predictions, temperature_);
    return gr;
}

GroundingResult CMGrounder::PredictMotorForConcept(int64_t concept_id, int k) {
    GroundingResult gr;
    if (!concept_encoder_) return gr;
    CMVec c = concept_encoder_(concept_id);
    gr.predictions = motor_idx_.TopK(c, k);
    for (auto& p : gr.predictions) {
        float ctx = context_relevance_.count(p.first) ? context_relevance_[p.first] : 0.0f;
        float tmp = temporal_consistency_.count(p.first) ? temporal_consistency_[p.first] : 0.0f;
        p.second = p.second * (1.0f + alpha_ * ctx + beta_ * tmp);
    }
    softmax_normalize(gr.predictions, temperature_);
    return gr;
}

std::vector<std::pair<int64_t,float>> CMGrounder::PredictConceptForVision(const std::string& vision_key, int k) {
    // For a lite version, approximate by scanning bindings for this key
    std::vector<std::pair<int64_t,float>> out;
    auto bs = bindings_.ForKey(vision_key);
    for (const auto& b : bs) if (b.mod == Binding::VISION) out.emplace_back(b.concept_id, b.weight);
    std::sort(out.begin(), out.end(), [](auto&a, auto&b){ return a.second>b.second; });
    if ((int)out.size() > k) out.resize(k);
    return out;
}

std::vector<std::pair<int64_t,float>> CMGrounder::PredictConceptForAudio(const std::string& audio_key, int k) {
    std::vector<std::pair<int64_t,float>> out;
    auto bs = bindings_.ForKey(audio_key);
    for (const auto& b : bs) if (b.mod == Binding::AUDIO) out.emplace_back(b.concept_id, b.weight);
    std::sort(out.begin(), out.end(), [](auto&a, auto&b){ return a.second>b.second; });
    if ((int)out.size() > k) out.resize(k);
    return out;
}

std::vector<std::pair<int64_t,float>> CMGrounder::PredictConceptForMotor(const std::string& motor_schema_id, int k) {
    std::vector<std::pair<int64_t,float>> out;
    auto bs = bindings_.ForKey(motor_schema_id);
    for (const auto& b : bs) if (b.mod == Binding::MOTOR) out.emplace_back(b.concept_id, b.weight);
    std::sort(out.begin(), out.end(), [](auto&a, auto&b){ return a.second>b.second; });
    if ((int)out.size() > k) out.resize(k);
    return out;
}

void CMGrounder::Reinforce(const Binding& b, float delta) {
    Binding upd = b;
    upd.weight = clamp01(b.weight + delta);
    bindings_.Upsert(upd);
}

} // namespace crossmodal
} // namespace melvin


