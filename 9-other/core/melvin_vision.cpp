#include "melvin_vision.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_set>

namespace melvin {
namespace uca {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

MelvinVision::MelvinVision(const Config& config)
    : config_(config)
    , frame_count_(0)
    , next_object_id_(0)
    , total_patches_processed_(0)
    , total_objects_formed_(0)
{
    std::cout << "🧠 MelvinVision initialized (V1→V2→V4→IT pathway)" << std::endl;
}

MelvinVision::~MelvinVision() {}

// ============================================================================
// MAIN PIPELINE
// ============================================================================

std::vector<VisualNode> MelvinVision::process_frame(
    const uint8_t* image_data,
    int width,
    int height,
    int channels,
    double timestamp)
{
    frame_count_++;
    
    // STAGE 1: V1 - Extract patches
    auto patches = extract_patches(image_data, width, height, channels);
    total_patches_processed_ += patches.size();
    
    // STAGE 2: V2 - Gestalt grouping
    auto groups = group_patches(patches);
    
    // STAGE 3: V4 - Form objects
    auto objects = form_objects(groups);
    total_objects_formed_ += objects.size();
    
    // STAGE 4: Parietal - Bottom-up saliency
    compute_saliency(objects);
    
    // STAGE 5: Prefrontal - Top-down relevance
    compute_relevance(objects);
    
    // STAGE 6: Predictive - Curiosity from prediction error
    compute_curiosity(objects);
    
    // STAGE 7: FEF/SC - Combined focus scores
    compute_focus_scores(objects);
    
    // STAGE 8: Temporal - Track across frames
    if (config_.enable_tracking) {
        track_objects(objects);
    }
    
    // Store patches for next frame
    prev_patches_ = std::move(patches);
    
    return objects;
}

void MelvinVision::integrate_to_graph(
    const std::vector<VisualNode>& objects,
    AtomicGraph& graph)
{
    for (const auto& obj : objects) {
        // Create object node
        uint64_t obj_node = graph.get_or_create_node(obj.object_id, 1); // type=1 (instance)
        
        // Create feature nodes and link
        if (obj.motion_magnitude > config_.motion_threshold) {
            uint64_t motion_node = graph.get_or_create_node("motion", 0); // type=0 (concept)
            graph.add_edge(obj_node, motion_node, OBSERVED_AS, obj.motion_magnitude / 10.0f);
        }
        
        // Spatial co-occurrence with other objects
        for (const auto& other : objects) {
            if (other.object_id == obj.object_id) continue;
            
            uint64_t other_node = graph.get_or_create_node(other.object_id, 1);
            float distance = std::sqrt(
                std::pow(obj.center_x - other.center_x, 2) +
                std::pow(obj.center_y - other.center_y, 2)
            );
            
            // Close objects co-occur
            if (distance < 200.0f) {
                float weight = 1.0f - (distance / 200.0f);
                graph.add_edge(obj_node, other_node, CO_OCCURS_WITH, weight);
            }
        }
        
        // Link to concepts
        for (uint64_t concept_id : obj.concept_links) {
            graph.add_edge(obj_node, concept_id, INSTANCE_OF, obj.saliency);
        }
    }
}

// ============================================================================
// TOP-DOWN MODULATION
// ============================================================================

void MelvinVision::set_active_concepts(const std::vector<std::string>& concepts) {
    active_concepts_ = concepts;
}

void MelvinVision::set_predictions(const std::vector<std::string>& expected_objects) {
    predicted_objects_ = expected_objects;
}

std::vector<VisualNode> MelvinVision::get_surprising_objects() const {
    std::vector<VisualNode> surprising;
    for (const auto& obj : tracked_objects_) {
        if (obj.curiosity > 0.7f) {
            surprising.push_back(obj);
        }
    }
    return surprising;
}

void MelvinVision::print_stats() const {
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout << "║  🧠 MelvinVision Statistics              ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    std::cout << "  Frames processed:     " << frame_count_ << "\n";
    std::cout << "  Total patches:        " << total_patches_processed_ << "\n";
    std::cout << "  Total objects formed: " << total_objects_formed_ << "\n";
    std::cout << "  Currently tracked:    " << tracked_objects_.size() << "\n";
    std::cout << std::endl;
}

// ============================================================================
// VISUAL HIERARCHY STAGES
// ============================================================================

std::vector<FrameNode> MelvinVision::extract_patches(
    const uint8_t* image_data,
    int width, int height, int channels)
{
    // V1 ANALOG: Simple/complex cells for edge, orientation, color
    std::vector<FrameNode> patches;
    
    int patch_size = config_.patch_size;
    int rows = height / patch_size;
    int cols = width / patch_size;
    
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            FrameNode patch;
            patch.x = c * patch_size;
            patch.y = r * patch_size;
            patch.w = patch_size;
            patch.h = patch_size;
            
            // Sample center pixel for features
            int center_y = patch.y + patch_size / 2;
            int center_x = patch.x + patch_size / 2;
            
            if (center_y >= height || center_x >= width) continue;
            
            int idx = (center_y * width + center_x) * channels;
            
            if (channels >= 3) {
                patch.color[0] = image_data[idx + 0] / 255.0f;
                patch.color[1] = image_data[idx + 1] / 255.0f;
                patch.color[2] = image_data[idx + 2] / 255.0f;
                patch.luminance = (patch.color[0] + patch.color[1] + patch.color[2]) / 3.0f;
            } else {
                patch.luminance = image_data[idx] / 255.0f;
                patch.color[0] = patch.color[1] = patch.color[2] = patch.luminance;
            }
            
            // Compute local contrast (Sobel-like)
            float gradient_sum = 0;
            int sample_count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int sy = center_y + dy * patch_size / 4;
                    int sx = center_x + dx * patch_size / 4;
                    if (sy >= 0 && sy < height && sx >= 0 && sx < width) {
                        int sidx = (sy * width + sx) * channels;
                        float lum = image_data[sidx] / 255.0f;
                        gradient_sum += std::abs(lum - patch.luminance);
                        sample_count++;
                    }
                }
            }
            patch.contrast = sample_count > 0 ? gradient_sum / sample_count : 0;
            patch.edge_density = patch.contrast; // Simplified
            
            // Motion computation (optical flow approximation)
            if (!prev_patches_.empty() && r * cols + c < (int)prev_patches_.size()) {
                const auto& prev = prev_patches_[r * cols + c];
                patch.motion_dx = patch.luminance - prev.luminance; // Simplified
                patch.motion_dy = 0; // Would need proper optical flow
            }
            
            patches.push_back(patch);
        }
    }
    
    return patches;
}

std::vector<std::vector<FrameNode*>> MelvinVision::group_patches(
    std::vector<FrameNode>& patches)
{
    // V2 ANALOG: Gestalt grouping (proximity + similarity)
    std::vector<std::vector<FrameNode*>> groups;
    std::unordered_set<FrameNode*> visited;
    
    for (auto& patch : patches) {
        if (visited.count(&patch)) continue;
        
        // Only group patches with sufficient contrast
        if (patch.contrast < 0.1f) continue;
        
        // BFS grouping
        std::vector<FrameNode*> group;
        std::queue<FrameNode*> queue;
        queue.push(&patch);
        visited.insert(&patch);
        
        while (!queue.empty()) {
            auto* current = queue.front();
            queue.pop();
            group.push_back(current);
            
            // Find neighbors
            for (auto& candidate : patches) {
                if (visited.count(&candidate)) continue;
                
                float dist = compute_spatial_distance(*current, candidate);
                float sim = compute_feature_similarity(*current, candidate);
                
                if (dist < config_.proximity_threshold && sim > config_.similarity_threshold) {
                    visited.insert(&candidate);
                    queue.push(&candidate);
                }
            }
        }
        
        if (group.size() >= (size_t)config_.min_patches_per_object) {
            groups.push_back(group);
        }
    }
    
    return groups;
}

std::vector<VisualNode> MelvinVision::form_objects(
    const std::vector<std::vector<FrameNode*>>& groups)
{
    // V4 ANALOG: Complex shape/surface representation
    std::vector<VisualNode> objects;
    
    for (const auto& group : groups) {
        if (group.empty()) continue;
        
        VisualNode obj;
        obj.object_id = "object_" + std::to_string(next_object_id_++);
        obj.patches = group;
        
        // Compute bounding box and center
        obj.bbox_x1 = obj.bbox_y1 = 999999;
        obj.bbox_x2 = obj.bbox_y2 = -999999;
        
        float sum_lum = 0, sum_contrast = 0;
        float sum_color[3] = {0, 0, 0};
        float sum_motion_x = 0, sum_motion_y = 0;
        
        for (const auto* patch : group) {
            obj.bbox_x1 = std::min(obj.bbox_x1, patch->x);
            obj.bbox_y1 = std::min(obj.bbox_y1, patch->y);
            obj.bbox_x2 = std::max(obj.bbox_x2, patch->x + patch->w);
            obj.bbox_y2 = std::max(obj.bbox_y2, patch->y + patch->h);
            
            sum_lum += patch->luminance;
            sum_contrast += patch->contrast;
            sum_color[0] += patch->color[0];
            sum_color[1] += patch->color[1];
            sum_color[2] += patch->color[2];
            sum_motion_x += patch->motion_dx;
            sum_motion_y += patch->motion_dy;
        }
        
        int n = group.size();
        obj.center_x = (obj.bbox_x1 + obj.bbox_x2) / 2;
        obj.center_y = (obj.bbox_y1 + obj.bbox_y2) / 2;
        obj.area = (obj.bbox_x2 - obj.bbox_x1) * (obj.bbox_y2 - obj.bbox_y1);
        
        obj.avg_luminance = sum_lum / n;
        obj.avg_contrast = sum_contrast / n;
        obj.dominant_color[0] = sum_color[0] / n;
        obj.dominant_color[1] = sum_color[1] / n;
        obj.dominant_color[2] = sum_color[2] / n;
        
        // Motion
        obj.motion_magnitude = std::sqrt(sum_motion_x * sum_motion_x + sum_motion_y * sum_motion_y) / n;
        obj.motion_direction = std::atan2(sum_motion_y, sum_motion_x);
        
        // Shape complexity (simplified)
        float perimeter = 2 * (obj.bbox_x2 - obj.bbox_x1) + 2 * (obj.bbox_y2 - obj.bbox_y1);
        obj.shape_complexity = (perimeter * perimeter) / (obj.area + 1e-6f);
        
        objects.push_back(obj);
    }
    
    return objects;
}

void MelvinVision::compute_saliency(std::vector<VisualNode>& objects) {
    // PARIETAL ANALOG: Bottom-up attention
    
    // Normalize by frame statistics
    float max_contrast = 1e-6f;
    float max_motion = 1e-6f;
    for (const auto& obj : objects) {
        max_contrast = std::max(max_contrast, obj.avg_contrast);
        max_motion = std::max(max_motion, obj.motion_magnitude);
    }
    
    for (auto& obj : objects) {
        float contrast_sal = obj.avg_contrast / max_contrast;
        float motion_sal = obj.motion_magnitude / max_motion;
        
        // Color pop-out (distance from gray)
        float color_dist = std::abs(obj.dominant_color[0] - 0.5f) +
                          std::abs(obj.dominant_color[1] - 0.5f) +
                          std::abs(obj.dominant_color[2] - 0.5f);
        float color_sal = color_dist / 1.5f;
        
        obj.saliency = (contrast_sal * 0.4f + motion_sal * 0.4f + color_sal * 0.2f);
        obj.saliency = std::min(1.0f, obj.saliency);
    }
}

void MelvinVision::compute_relevance(std::vector<VisualNode>& objects) {
    // PREFRONTAL ANALOG: Top-down goal matching
    
    for (auto& obj : objects) {
        obj.relevance = 0.0f;
        
        // Match against active concepts
        for (const auto& concept : active_concepts_) {
            // Simplified: would use learned feature matching
            // For now, just give baseline relevance to all
            obj.relevance = 0.5f;
        }
    }
}

void MelvinVision::compute_curiosity(std::vector<VisualNode>& objects) {
    // PREDICTIVE CODING: Prediction error → curiosity
    
    if (!config_.enable_prediction) {
        for (auto& obj : objects) obj.curiosity = 0.0f;
        return;
    }
    
    for (auto& obj : objects) {
        obj.curiosity = 0.0f;
        
        // Check if predicted
        bool was_predicted = false;
        for (const auto& pred : predicted_objects_) {
            if (obj.object_id == pred) {
                was_predicted = true;
                break;
            }
        }
        
        // Unpredicted = high curiosity
        if (!was_predicted && obj.frames_visible < 3) {
            obj.curiosity = 0.8f;
        }
        
        // Unexpected motion
        if (obj.motion_magnitude > config_.motion_threshold * 2) {
            obj.curiosity = std::max(obj.curiosity, 0.6f);
        }
    }
}

void MelvinVision::compute_focus_scores(std::vector<VisualNode>& objects) {
    // FEF/SC ANALOG: Combined attention score
    
    for (auto& obj : objects) {
        obj.focus_score = 
            config_.alpha_saliency * obj.saliency +
            config_.beta_relevance * obj.relevance +
            config_.gamma_curiosity * obj.curiosity;
    }
}

void MelvinVision::track_objects(std::vector<VisualNode>& new_objects) {
    // MT/MST ANALOG: Object tracking across frames
    
    // Match new objects to tracked objects
    std::vector<bool> matched(new_objects.size(), false);
    
    for (auto& tracked : tracked_objects_) {
        tracked.frames_since_last_seen++;
        
        // Find best match
        float best_sim = 0;
        int best_idx = -1;
        
        for (size_t i = 0; i < new_objects.size(); ++i) {
            if (matched[i]) continue;
            
            float sim = compute_object_similarity(tracked, new_objects[i]);
            if (sim > best_sim) {
                best_sim = sim;
                best_idx = i;
            }
        }
        
        if (best_idx >= 0 && best_sim > 0.5f) {
            // Update tracked object
            new_objects[best_idx].persistent_id = tracked.persistent_id;
            new_objects[best_idx].frames_visible = tracked.frames_visible + 1;
            matched[best_idx] = true;
        }
    }
    
    // Assign new IDs to unmatched
    static uint64_t next_persistent_id = 1000;
    for (size_t i = 0; i < new_objects.size(); ++i) {
        if (!matched[i]) {
            new_objects[i].persistent_id = next_persistent_id++;
            new_objects[i].frames_visible = 1;
        }
    }
    
    // Remove old tracked objects
    tracked_objects_.erase(
        std::remove_if(tracked_objects_.begin(), tracked_objects_.end(),
            [](const VisualNode& obj) { return obj.frames_since_last_seen > 10; }),
        tracked_objects_.end()
    );
    
    // Add new objects to tracking
    tracked_objects_ = new_objects;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

float MelvinVision::compute_feature_similarity(const FrameNode& a, const FrameNode& b) {
    float lum_diff = std::abs(a.luminance - b.luminance);
    float contrast_diff = std::abs(a.contrast - b.contrast);
    float color_diff = std::abs(a.color[0] - b.color[0]) +
                       std::abs(a.color[1] - b.color[1]) +
                       std::abs(a.color[2] - b.color[2]);
    
    float total_diff = (lum_diff + contrast_diff + color_diff / 3.0f) / 3.0f;
    return 1.0f - std::min(1.0f, total_diff);
}

float MelvinVision::compute_spatial_distance(const FrameNode& a, const FrameNode& b) {
    int dx = (a.x + a.w / 2) - (b.x + b.w / 2);
    int dy = (a.y + a.h / 2) - (b.y + b.h / 2);
    return std::sqrt(dx * dx + dy * dy);
}

float MelvinVision::compute_object_similarity(const VisualNode& a, const VisualNode& b) {
    // Spatial proximity
    int dx = a.center_x - b.center_x;
    int dy = a.center_y - b.center_y;
    float spatial_sim = 1.0f / (1.0f + std::sqrt(dx * dx + dy * dy) / 100.0f);
    
    // Feature similarity
    float lum_sim = 1.0f - std::abs(a.avg_luminance - b.avg_luminance);
    float size_sim = 1.0f - std::abs(a.area - b.area) / std::max(a.area, b.area);
    
    return spatial_sim * 0.6f + lum_sim * 0.2f + size_sim * 0.2f;
}

} // namespace uca
} // namespace melvin


