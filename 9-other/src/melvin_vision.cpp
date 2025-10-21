#include "melvin_vision.h"
#include <cmath>
#include <algorithm>

namespace melvin {

VisionSystem::VisionSystem(AtomicGraph& graph)
    : graph_(graph)
    , prev_frame_id_(0)
{}

uint64_t VisionSystem::ingest_frame(const FrameMeta& m, std::span<const uint8_t> rgb) {
    // Create frame node
    uint64_t frame_id = graph_.create_frame(m);
    
    // Tokenize into patches (grid-based)
    const int patch_size = 32;
    const int grid_w = m.w / patch_size;
    const int grid_h = m.h / patch_size;
    
    std::vector<std::pair<uint64_t, float>> current_patches;
    
    for (int py = 0; py < grid_h; ++py) {
        for (int px = 0; px < grid_w; ++px) {
            // Sample center of patch
            int cx = px * patch_size + patch_size / 2;
            int cy = py * patch_size + patch_size / 2;
            
            if (cx >= m.w || cy >= m.h) continue;
            
            int idx = (cy * m.w + cx) * 3;
            float r = rgb[idx + 0] / 255.0f;
            float g = rgb[idx + 1] / 255.0f;
            float b = rgb[idx + 2] / 255.0f;
            float lum = (r + g + b) / 3.0f;
            
            // Create patch node
            uint64_t patch_id = graph_.get_or_create_visual_patch(frame_id, px, py);
            current_patches.push_back({patch_id, lum});
            
            // Link patch to frame
            graph_.add_or_bump_edge(frame_id, patch_id, Rel::PART_OF, 0.1f);
        }
    }
    
    // Temporal links
    if (prev_frame_id_ != 0) {
        graph_.add_temp_next(prev_frame_id_, frame_id);
        
        // Link temporally adjacent patches (simple motion detection)
        for (size_t i = 0; i < std::min(current_patches.size(), prev_patches_.size()); ++i) {
            graph_.add_temp_next(prev_patches_[i].first, current_patches[i].first);
        }
    }
    
    prev_frame_id_ = frame_id;
    prev_patches_ = current_patches;
    
    return frame_id;
}

std::vector<std::pair<uint64_t, VisionSystem::Scores>> VisionSystem::score_regions(uint64_t frame_node) {
    std::vector<std::pair<uint64_t, Scores>> results;
    
    // Get all patches for this frame
    auto patches = graph_.neighbors(frame_node, {Rel::PART_OF});
    
    for (uint64_t patch_id : patches) {
        const Node* patch = graph_.get_node(patch_id);
        if (!patch) continue;
        
        Scores s;
        
        // 1. SALIENCY (bottom-up): contrast, edges
        // Simplified: higher luminance variance = more salient
        float lum_var = 0.0f;
        if (!prev_patches_.empty()) {
            // Compare to previous patches (simple temporal contrast)
            for (const auto& [prev_id, prev_lum] : prev_patches_) {
                const Node* prev = graph_.get_node(prev_id);
                if (prev && std::abs((int)prev->a - (int)patch->a) < 2 && 
                           std::abs((int)prev->b - (int)patch->b) < 2) {
                    lum_var = std::abs(patch->c - prev_lum);
                    break;
                }
            }
        }
        s.saliency = std::min(1.0f, lum_var * 5.0f);
        
        // 2. GOAL RELEVANCE (top-down): connection to active concepts
        s.goal = 0.0f;
        for (uint64_t concept_id : active_concepts_) {
            // Check if patch has any connection to this concept
            auto neighbors = graph_.all_neighbors(patch_id);
            for (uint64_t n : neighbors) {
                if (n == concept_id) {
                    s.goal = std::min(1.0f, s.goal + 0.3f);
                }
            }
        }
        
        // 3. CURIOSITY (prediction error): novelty
        // Simplified: patches with no previous history are novel
        bool is_novel = prev_patches_.empty();
        s.curiosity = is_novel ? constants::CURIOSITY_MAX_BOOST : 0.0f;
        
        results.push_back({patch_id, s});
    }
    
    return results;
}

std::vector<uint64_t> VisionSystem::group_objects(uint64_t frame_node) {
    std::vector<uint64_t> objects;
    
    // Get patches
    auto patches = graph_.neighbors(frame_node, {Rel::PART_OF});
    
    // Simple grouping: spatially adjacent patches with similar luminance
    std::vector<bool> grouped(patches.size(), false);
    
    for (size_t i = 0; i < patches.size(); ++i) {
        if (grouped[i]) continue;
        
        const Node* p1 = graph_.get_node(patches[i]);
        if (!p1) continue;
        
        // Start new object
        std::vector<uint64_t> group;
        group.push_back(patches[i]);
        grouped[i] = true;
        
        // Find neighbors
        for (size_t j = i + 1; j < patches.size(); ++j) {
            if (grouped[j]) continue;
            
            const Node* p2 = graph_.get_node(patches[j]);
            if (!p2) continue;
            
            // Check spatial proximity
            int dx = std::abs((int)p1->a - (int)p2->a);
            int dy = std::abs((int)p1->b - (int)p2->b);
            
            if (dx <= 1 && dy <= 1) {
                // Adjacent
                group.push_back(patches[j]);
                grouped[j] = true;
            }
        }
        
        // Create object if group is significant
        if (group.size() >= 3) {
            // Compute center
            int center_x = 0, center_y = 0;
            for (uint64_t pid : group) {
                const Node* p = graph_.get_node(pid);
                center_x += p->a;
                center_y += p->b;
            }
            center_x /= group.size();
            center_y /= group.size();
            
            uint64_t obj_id = graph_.create_object(center_x, center_y);
            objects.push_back(obj_id);
            
            // Link patches to object
            for (uint64_t pid : group) {
                graph_.add_or_bump_edge(obj_id, pid, Rel::PART_OF, 0.1f);
                graph_.add_or_bump_edge(pid, obj_id, Rel::PART_OF, 0.1f);
            }
        }
    }
    
    return objects;
}

void VisionSystem::set_active_concepts(const std::vector<uint64_t>& concepts) {
    active_concepts_ = concepts;
}

} // namespace melvin


