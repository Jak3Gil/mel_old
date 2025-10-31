/**
 * @file cm_index.h
 */

#ifndef MELVIN_CROSSMODAL_CM_INDEX_H
#define MELVIN_CROSSMODAL_CM_INDEX_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "cm_space.h"

namespace melvin {
namespace crossmodal {

class CMIndex {
public:
    void Add(const std::string& key, const CMVec& v);
    std::vector<std::pair<std::string,float>> TopK(const CMVec& q, int k) const;

private:
    std::unordered_map<std::string, CMVec> kv_;
    mutable std::mutex mu_;
};

} // namespace crossmodal
} // namespace melvin

#endif // MELVIN_CROSSMODAL_CM_INDEX_H


