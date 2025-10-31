/**
 * @file cm_index.cpp
 */

#include "cm_index.h"
#include <algorithm>

namespace melvin {
namespace crossmodal {

void CMIndex::Add(const std::string& key, const CMVec& v) {
    std::lock_guard<std::mutex> lock(mu_);
    kv_[key] = v;
}

std::vector<std::pair<std::string,float>> CMIndex::TopK(const CMVec& q, int k) const {
    std::vector<std::pair<std::string,float>> scores;
    scores.reserve(kv_.size());
    CMSpace& S = CMSpace::Instance();
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (const auto& it : kv_) {
            scores.emplace_back(it.first, S.Cosine(q, it.second));
        }
    }
    if ((int)scores.size() > k) {
        std::nth_element(scores.begin(), scores.begin() + k, scores.end(),
            [](const auto& a, const auto& b){ return a.second > b.second; });
        scores.resize(k);
    }
    std::sort(scores.begin(), scores.end(), [](const auto& a, const auto& b){ return a.second > b.second; });
    return scores;
}

} // namespace crossmodal
} // namespace melvin


