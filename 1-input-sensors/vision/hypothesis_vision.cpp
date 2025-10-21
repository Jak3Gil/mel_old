/*
 * Melvin Spatial Hypothesis Graph - Implementation
 * Self-organizing visual perception without labels
 */

#include "hypothesis_vision.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <deque>

namespace melvin {
namespace vision {

// ============================================================================
// CONFIG
// ============================================================================

HypothesisVision::Config::Config()
    : scales({30, 60, 120}),  // 3 resolution levels
      max_hypotheses_per_scale(50),
      hypothesis_confidence_threshold(0.4f),
      merge_overlap_threshold(0.5f),
      merge_similarity_threshold(0.7f),
      object_match_threshold(0.6f),
      object_max_missing_frames(30),
      concept_min_instances(5),
      concept_similarity_threshold(0.8f),
      enable_prediction(true),
      prediction_learning_rate(0.1f)
{}

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class HypothesisVision::Impl {
public:
    Config config;
    
    // Graph storage
    std::vector<VisualPatch> patches_;
    std::vector<Hypothesis> hypotheses_;
    std::vector<ObjectNode> objects_;
    std::vector<SceneNode> scenes_;
    std::vector<ConceptNode> concepts_;
    std::vector<GraphEdge> edges_;
    
    // Previous state for temporal tracking
    std::vector<ObjectNode> prev_objects_;
    cv::Mat prev_frame_;
    
    // ID generation
    NodeID next_patch_id_ = 1;
    NodeID next_hypothesis_id_ = 10000;
    NodeID next_object_id_ = 20000;
    NodeID next_scene_id_ = 30000;
    NodeID next_concept_id_ = 40000;
    
    // Frame counter
    int frame_count_ = 0;
    
    // Foveal attention point
    cv::Point2f focus_point_;
    int focus_radius_ = 150;  // 300×300 region
    
    // Stats
    HypothesisVision::Stats stats_;
    
    Impl(const Config& cfg) : config(cfg), focus_point_(0, 0) {}
    
    // Add edge to graph
    void add_edge(NodeID from, NodeID to, EdgeType type, float weight = 1.0f) {
        edges_.push_back({from, to, type, weight, {}});
    }
    
    // Calculate IoU (Intersection over Union) for overlap
    float calculate_iou(const cv::Rect& a, const cv::Rect& b) {
        cv::Rect intersection = a & b;
        if (intersection.area() == 0) return 0.0f;
        
        int union_area = a.area() + b.area() - intersection.area();
        return (float)intersection.area() / union_area;
    }
    
    // Calculate visual similarity between embeddings
    float calculate_similarity(const cv::Mat& a, const cv::Mat& b) {
        if (a.empty() || b.empty()) return 0.0f;
        
        // Cosine similarity
        double dot = a.dot(b);
        double norm_a = cv::norm(a);
        double norm_b = cv::norm(b);
        
        if (norm_a == 0 || norm_b == 0) return 0.0f;
        return (float)(dot / (norm_a * norm_b));
    }
};

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

HypothesisVision::HypothesisVision(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

HypothesisVision::~HypothesisVision() = default;

// ============================================================================
// 1. SPATIAL GRID FORMATION (Multi-Scale)
// ============================================================================

std::vector<std::vector<VisualPatch>> HypothesisVision::create_spatial_grid(
    const cv::Mat& frame
) {
    std::vector<std::vector<VisualPatch>> multi_scale_grids;
    
    cv::Mat gray, prev_gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    
    // Motion map
    cv::Mat motion_map;
    if (!impl_->prev_frame_.empty()) {
        cv::cvtColor(impl_->prev_frame_, prev_gray, cv::COLOR_BGR2GRAY);
        cv::absdiff(gray, prev_gray, motion_map);
        cv::threshold(motion_map, motion_map, 25, 255, cv::THRESH_BINARY);
    }
    
    // Create grid at each scale
    for (size_t scale_idx = 0; scale_idx < impl_->config.scales.size(); ++scale_idx) {
        int patch_size = impl_->config.scales[scale_idx];
        std::vector<VisualPatch> grid;
        
        int grid_h = frame.rows / patch_size;
        int grid_w = frame.cols / patch_size;
        
        for (int py = 0; py < grid_h; ++py) {
            for (int px = 0; px < grid_w; ++px) {
                int y = py * patch_size;
                int x = px * patch_size;
                
                if (y + patch_size > frame.rows || x + patch_size > frame.cols) continue;
                
                cv::Rect roi(x, y, patch_size, patch_size);
                cv::Mat patch = frame(roi);
                cv::Mat patch_gray = gray(roi);
                
                VisualPatch vp;
                vp.id = impl_->next_patch_id_++;
                vp.x = px;
                vp.y = py;
                vp.scale = scale_idx;
                
                // Extract features
                vp.color_mean = cv::mean(patch);
                cv::Scalar mean, stddev;
                cv::meanStdDev(patch_gray, mean, stddev);
                vp.color_std = stddev[0];
                vp.brightness = mean[0];
                
                // Edges
                cv::Mat edges;
                cv::Canny(patch_gray, edges, 50, 150);
                vp.edge_density = (float)cv::countNonZero(edges) / (patch_size * patch_size);
                
                // Motion
                vp.motion = 0.0f;
                if (!motion_map.empty()) {
                    cv::Mat motion_patch = motion_map(roi);
                    vp.motion = (float)cv::countNonZero(motion_patch) / (patch_size * patch_size);
                }
                
                // Texture entropy (simplified)
                vp.texture_entropy = stddev[0] / 128.0f;
                
                // Attention (reweighted for stability)
                vp.saliency = stddev[0] / 128.0f;
                vp.focus_score = vp.saliency + vp.motion * 5.0f + vp.edge_density * 2.0f;  // Motion >> edges >> contrast
                
                // Create simple embedding (8D feature vector)
                float features[8] = {
                    static_cast<float>(vp.color_mean[0] / 255.0),
                    static_cast<float>(vp.color_mean[1] / 255.0),
                    static_cast<float>(vp.color_mean[2] / 255.0),
                    vp.edge_density,
                    vp.motion,
                    vp.brightness / 255.0f,
                    vp.color_std / 128.0f,
                    vp.texture_entropy
                };
                vp.embedding = cv::Mat(1, 8, CV_32F, features).clone();
                
                grid.push_back(vp);
                impl_->patches_.push_back(vp);
                
                // SPATIALLY_NEAR edges
                if (px > 0 && !grid.empty()) {
                    impl_->add_edge(grid[grid.size()-2].id, vp.id, EdgeType::SPATIALLY_NEAR);
                }
                if (py > 0 && grid.size() > grid_w) {
                    impl_->add_edge(grid[grid.size()-grid_w-1].id, vp.id, EdgeType::SPATIALLY_NEAR);
                }
            }
        }
        
        multi_scale_grids.push_back(grid);
        
        // PART_OF edges between scales
        if (scale_idx > 0 && !multi_scale_grids[scale_idx-1].empty()) {
            // Connect fine scale to coarse scale
            for (const auto& fine_patch : multi_scale_grids[scale_idx-1]) {
                // Find corresponding coarse patch
                int coarse_x = fine_patch.x / 2;
                int coarse_y = fine_patch.y / 2;
                
                for (const auto& coarse_patch : grid) {
                    if (coarse_patch.x == coarse_x && coarse_patch.y == coarse_y) {
                        impl_->add_edge(fine_patch.id, coarse_patch.id, EdgeType::PART_OF);
                        break;
                    }
                }
            }
        }
    }
    
    return multi_scale_grids;
}

// ============================================================================
// 2. HYPOTHESIS GENERATION
// ============================================================================

std::vector<Hypothesis> HypothesisVision::generate_hypotheses(
    const std::vector<std::vector<VisualPatch>>& grids
) {
    std::vector<Hypothesis> hypotheses;
    
    // For each scale, find clusters of high-attention patches NEAR FOCUS POINT
    for (const auto& grid : grids) {
        if (grid.empty()) continue;
        
        int patch_size = impl_->config.scales[grid[0].scale];
        
        // Find high-attention patches WITHIN FOCUS REGION
        std::vector<VisualPatch> interesting_patches;
        for (const auto& patch : grid) {
            // Calculate patch center in pixel coordinates
            float patch_center_x = patch.x * patch_size + patch_size / 2.0f;
            float patch_center_y = patch.y * patch_size + patch_size / 2.0f;
            
            // Check if within focus region
            float dist_to_focus = cv::norm(
                cv::Point2f(patch_center_x, patch_center_y) - impl_->focus_point_
            );
            
            // Only process patches near focus point with HIGH attention
            if (dist_to_focus < impl_->focus_radius_ && patch.focus_score > 1.0f) {
                interesting_patches.push_back(patch);
            }
        }
        
        // Cluster nearby patches into hypothesis regions
        std::vector<bool> visited(interesting_patches.size(), false);
        
        for (size_t i = 0; i < interesting_patches.size(); ++i) {
            if (visited[i]) continue;
            
            Hypothesis hyp;
            hyp.id = impl_->next_hypothesis_id_++;
            hyp.patch_ids.push_back(interesting_patches[i].id);
            hyp.age_frames = 0;
            hyp.stability = 0.0f;
            
            // Cluster expansion (simple region growing)
            std::vector<cv::Point> points;
            points.push_back(cv::Point(interesting_patches[i].x, interesting_patches[i].y));
            visited[i] = true;
            
            for (size_t j = i + 1; j < interesting_patches.size(); ++j) {
                if (visited[j]) continue;
                
                // Check if nearby
                int dx = abs(interesting_patches[j].x - interesting_patches[i].x);
                int dy = abs(interesting_patches[j].y - interesting_patches[i].y);
                
                if (dx <= 2 && dy <= 2) {
                    hyp.patch_ids.push_back(interesting_patches[j].id);
                    points.push_back(cv::Point(interesting_patches[j].x, interesting_patches[j].y));
                    visited[j] = true;
                }
            }
            
            // Create bounding box
            if (!points.empty()) {
                cv::Rect bbox = cv::boundingRect(points);
                hyp.bbox = cv::Rect(
                    bbox.x * impl_->config.scales[interesting_patches[i].scale],
                    bbox.y * impl_->config.scales[interesting_patches[i].scale],
                    bbox.width * impl_->config.scales[interesting_patches[i].scale],
                    bbox.height * impl_->config.scales[interesting_patches[i].scale]
                );
                
                // Confidence based on cluster size and focus
                hyp.confidence = std::min(1.0f, (float)hyp.patch_ids.size() / 5.0f);
                
                if (hyp.confidence > impl_->config.hypothesis_confidence_threshold) {
                    // OBSERVES edges
                    for (NodeID patch_id : hyp.patch_ids) {
                        impl_->add_edge(hyp.id, patch_id, EdgeType::OBSERVES);
                    }
                    
                    hypotheses.push_back(hyp);
                }
            }
        }
    }
    
    // NEAR edges between overlapping hypotheses
    for (size_t i = 0; i < hypotheses.size(); ++i) {
        for (size_t j = i + 1; j < hypotheses.size(); ++j) {
            float overlap = impl_->calculate_iou(hypotheses[i].bbox, hypotheses[j].bbox);
            if (overlap > 0.1f) {
                impl_->add_edge(hypotheses[i].id, hypotheses[j].id, EdgeType::NEAR, overlap);
            }
        }
    }
    
    return hypotheses;
}

void HypothesisVision::refine_hypotheses(
    std::vector<Hypothesis>& hypotheses,
    const cv::Mat& frame
) {
    // Age hypotheses and update stability
    for (auto& hyp : hypotheses) {
        hyp.age_frames++;
        hyp.stability = std::min(1.0f, hyp.age_frames / 10.0f);
    }
    
    // Remove low-confidence hypotheses
    hypotheses.erase(
        std::remove_if(hypotheses.begin(), hypotheses.end(),
            [this](const Hypothesis& h) {
                return h.confidence < impl_->config.hypothesis_confidence_threshold;
            }),
        hypotheses.end()
    );
}

// ============================================================================
// 3. OBJECT FORMATION
// ============================================================================

std::vector<ObjectNode> HypothesisVision::merge_into_objects(
    const std::vector<Hypothesis>& hypotheses
) {
    std::vector<ObjectNode> objects;
    std::vector<bool> merged(hypotheses.size(), false);
    
    for (size_t i = 0; i < hypotheses.size(); ++i) {
        if (merged[i]) continue;
        
        ObjectNode obj;
        obj.id = impl_->next_object_id_++;
        obj.hypothesis_ids.push_back(hypotheses[i].id);
        obj.bbox = hypotheses[i].bbox;
        obj.belief_score = hypotheses[i].confidence;
        obj.frames_tracked = 1;
        obj.frames_since_seen = 0;
        obj.concept_id = 0;
        
        merged[i] = true;
        
        // Merge similar/overlapping hypotheses
        for (size_t j = i + 1; j < hypotheses.size(); ++j) {
            if (merged[j]) continue;
            
            float overlap = impl_->calculate_iou(hypotheses[i].bbox, hypotheses[j].bbox);
            
            if (overlap > impl_->config.merge_overlap_threshold) {
                obj.hypothesis_ids.push_back(hypotheses[j].id);
                obj.bbox |= hypotheses[j].bbox;  // Union
                obj.belief_score = std::max(obj.belief_score, hypotheses[j].confidence);
                merged[j] = true;
            }
        }
        
        // Center and properties
        obj.center = cv::Point2f(
            obj.bbox.x + obj.bbox.width / 2.0f,
            obj.bbox.y + obj.bbox.height / 2.0f
        );
        obj.velocity = cv::Point2f(0, 0);
        
        // PREDICTS edges
        for (NodeID hyp_id : obj.hypothesis_ids) {
            impl_->add_edge(hyp_id, obj.id, EdgeType::PREDICTS);
        }
        
        objects.push_back(obj);
    }
    
    return objects;
}

// ============================================================================
// 4. TEMPORAL TRACKING
// ============================================================================

void HypothesisVision::track_objects_temporal(
    std::vector<ObjectNode>& current_objects
) {
    if (impl_->prev_objects_.empty()) {
        impl_->prev_objects_ = current_objects;
        return;
    }
    
    // Keep track of which previous objects were matched
    std::vector<bool> prev_matched(impl_->prev_objects_.size(), false);
    
    // Match current objects to previous frame (much more generous matching)
    for (auto& curr_obj : current_objects) {
        float best_match_score = 0.0f;
        int best_prev_idx = -1;
        
        for (size_t i = 0; i < impl_->prev_objects_.size(); ++i) {
            const auto& prev_obj = impl_->prev_objects_[i];
            
            // Multi-factor matching (position, size, overlap)
            float dist = cv::norm(curr_obj.center - prev_obj.center);
            float overlap = impl_->calculate_iou(curr_obj.bbox, prev_obj.bbox);
            
            float size_ratio = (float)std::min(curr_obj.bbox.area(), prev_obj.bbox.area()) /
                              std::max(curr_obj.bbox.area(), prev_obj.bbox.area());
            
            // Generous matching: position OR overlap
            float position_score = 1.0f / (1.0f + dist / 200.0f);  // 200px tolerance
            float overlap_score = overlap;
            float size_score = size_ratio;
            
            // Match if EITHER position is close OR there's overlap
            float match_score = std::max(position_score * size_score, overlap_score);
            
            if (match_score > best_match_score && match_score > 0.3f) {  // Lower threshold
                best_match_score = match_score;
                best_prev_idx = i;
            }
        }
        
        if (best_prev_idx >= 0) {
            const auto& prev_obj = impl_->prev_objects_[best_prev_idx];
            
            // Maintain identity!
            curr_obj.id = prev_obj.id;
            curr_obj.velocity = curr_obj.center - prev_obj.center;
            curr_obj.frames_tracked = prev_obj.frames_tracked + 1;
            curr_obj.frames_since_seen = 0;
            curr_obj.concept_id = prev_obj.concept_id;  // Keep concept link
            
            // Smooth belief score
            curr_obj.belief_score = prev_obj.belief_score * 0.8f + curr_obj.belief_score * 0.2f;
            
            // Mark as matched
            prev_matched[best_prev_idx] = true;
        }
    }
    
    // Keep unmatched previous objects if recently seen (object persistence)
    for (size_t i = 0; i < impl_->prev_objects_.size(); ++i) {
        if (!prev_matched[i]) {
            auto lost_obj = impl_->prev_objects_[i];
            lost_obj.frames_since_seen++;
            
            // Keep object alive for a few frames even if not detected
            if (lost_obj.frames_since_seen < 10) {
                lost_obj.belief_score *= 0.9f;  // Decay confidence
                current_objects.push_back(lost_obj);
            }
        }
    }
    
    impl_->prev_objects_ = current_objects;
}

// ============================================================================
// 5. SCENE CONSTRUCTION
// ============================================================================

SceneNode HypothesisVision::construct_scene(
    const std::vector<ObjectNode>& objects
) {
    SceneNode scene;
    scene.id = impl_->next_scene_id_++;
    scene.frame_number = impl_->frame_count_;
    
    for (const auto& obj : objects) {
        scene.object_ids.push_back(obj.id);
    }
    
    // Spatial relationships
    for (size_t i = 0; i < objects.size(); ++i) {
        for (size_t j = i + 1; j < objects.size(); ++j) {
            const auto& obj_a = objects[i];
            const auto& obj_b = objects[j];
            
            // CO_OCCURS_WITH
            impl_->add_edge(obj_a.id, obj_b.id, EdgeType::CO_OCCURS_WITH);
            
            // Spatial relations
            if (obj_a.center.y < obj_b.center.y - 50) {
                impl_->add_edge(obj_a.id, obj_b.id, EdgeType::ABOVE);
            } else if (obj_a.center.y > obj_b.center.y + 50) {
                impl_->add_edge(obj_a.id, obj_b.id, EdgeType::BELOW);
            }
            
            if (obj_a.center.x < obj_b.center.x - 50) {
                impl_->add_edge(obj_a.id, obj_b.id, EdgeType::LEFT_OF);
            } else if (obj_a.center.x > obj_b.center.x + 50) {
                impl_->add_edge(obj_a.id, obj_b.id, EdgeType::RIGHT_OF);
            }
            
            // INSIDE/CONTAINS based on bbox inclusion
            if ((obj_a.bbox & obj_b.bbox) == obj_a.bbox) {
                impl_->add_edge(obj_a.id, obj_b.id, EdgeType::INSIDE);
                impl_->add_edge(obj_b.id, obj_a.id, EdgeType::CONTAINS);
            }
        }
    }
    
    // Link to previous scene
    if (!impl_->scenes_.empty()) {
        scene.prev_scene_id = impl_->scenes_.back().id;
        impl_->add_edge(scene.prev_scene_id, scene.id, EdgeType::TEMPORAL_NEXT);
    }
    
    return scene;
}

// ============================================================================
// 6. CONCEPT EMERGENCE (Simplified)
// ============================================================================

void HypothesisVision::update_concepts(
    const std::vector<ObjectNode>& objects
) {
    // Simple concept formation: group similar recurring objects
    // (Full implementation would use clustering over time)
    
    // Need to modify objects, so work with impl_->objects_ instead
    for (auto& obj : impl_->objects_) {
        if (obj.concept_id != 0) continue;  // Already categorized
        
        // Check against existing concepts
        bool matched = false;
        for (auto& concept : impl_->concepts_) {
            // Match based on size/position patterns
            // (Simplified - real version would use embeddings)
            float size_ratio = (float)obj.bbox.area() / 10000.0f;
            
            if (std::abs(size_ratio - concept.prototype_distance_threshold) < 0.3f) {
                obj.concept_id = concept.id;
                concept.instance_count++;
                impl_->add_edge(obj.id, concept.id, EdgeType::INSTANCE_OF);
                matched = true;
                break;
            }
        }
        
        // Create new concept if persistent object
        if (!matched && obj.frames_tracked > impl_->config.concept_min_instances) {
            ConceptNode concept;
            concept.id = impl_->next_concept_id_++;
            concept.label = "concept_" + std::to_string(concept.id);
            concept.instance_count = 1;
            concept.prototype_distance_threshold = (float)obj.bbox.area() / 10000.0f;
            
            impl_->concepts_.push_back(concept);
            obj.concept_id = concept.id;
            impl_->add_edge(obj.id, concept.id, EdgeType::INSTANCE_OF);
        }
    }
}

// ============================================================================
// MAIN PROCESSING PIPELINE
// ============================================================================

SceneNode HypothesisVision::process_frame(const cv::Mat& frame) {
    impl_->frame_count_++;
    
    // Clear per-frame data
    impl_->patches_.clear();
    impl_->hypotheses_.clear();
    impl_->objects_.clear();
    
    // 1. Spatial Grid (Multi-Scale)
    auto grids = create_spatial_grid(frame);
    
    // 2. Move focus point to highest attention patch (STABLE version)
    float max_focus = 0.0f;
    cv::Point2f new_focus = impl_->focus_point_;
    
    if (!grids.empty() && !grids[0].empty()) {
        // Prioritize motion heavily, require minimum threshold
        for (const auto& patch : grids[0]) {
            // Reweight: motion dominates
            float weighted_score = patch.saliency + patch.motion * 5.0f + patch.edge_density * 2.0f;
            
            // Only consider patches with significant activity
            if (weighted_score > 1.5f && weighted_score > max_focus) {
                max_focus = weighted_score;
                int patch_size = impl_->config.scales[0];
                new_focus = cv::Point2f(
                    patch.x * patch_size + patch_size / 2.0f,
                    patch.y * patch_size + patch_size / 2.0f
                );
            }
        }
    }
    
    // Strong momentum - focus moves slowly (stable attention)
    if (impl_->frame_count_ == 1) {
        impl_->focus_point_ = cv::Point2f(frame.cols / 2.0f, frame.rows / 2.0f);
    } else if (max_focus > 1.5f) {  // Only move if significant attention
        impl_->focus_point_ = impl_->focus_point_ * 0.9f + new_focus * 0.1f;  // 90% old, 10% new
    }
    // else: stay put if nothing interesting
    
    // 3. Generate Hypotheses (after focus is updated)
    auto hypotheses = generate_hypotheses(grids);
    refine_hypotheses(hypotheses, frame);
    impl_->hypotheses_ = hypotheses;
    
    // 3. Merge into Objects
    auto objects = merge_into_objects(hypotheses);
    
    // 4. Temporal Tracking
    track_objects_temporal(objects);
    impl_->objects_ = objects;
    
    // 5. Construct Scene
    auto scene = construct_scene(objects);
    impl_->scenes_.push_back(scene);
    
    // 6. Update Concepts
    update_concepts(impl_->objects_);
    
    // Update stats
    impl_->stats_.frames_processed = impl_->frame_count_;
    impl_->stats_.total_patches = impl_->patches_.size();
    impl_->stats_.total_hypotheses = impl_->hypotheses_.size();
    impl_->stats_.total_objects = impl_->objects_.size();
    impl_->stats_.total_scenes = impl_->scenes_.size();
    impl_->stats_.total_concepts = impl_->concepts_.size();
    impl_->stats_.total_edges = impl_->edges_.size();
    
    // Store for next frame
    impl_->prev_frame_ = frame.clone();
    
    return scene;
}

// ============================================================================
// ACCESSORS
// ============================================================================

const std::vector<VisualPatch>& HypothesisVision::get_patches() const {
    return impl_->patches_;
}

const std::vector<Hypothesis>& HypothesisVision::get_hypotheses() const {
    return impl_->hypotheses_;
}

const std::vector<ObjectNode>& HypothesisVision::get_objects() const {
    return impl_->objects_;
}

const std::vector<SceneNode>& HypothesisVision::get_scenes() const {
    return impl_->scenes_;
}

const std::vector<ConceptNode>& HypothesisVision::get_concepts() const {
    return impl_->concepts_;
}

const std::vector<GraphEdge>& HypothesisVision::get_edges() const {
    return impl_->edges_;
}

// ============================================================================
// STATISTICS
// ============================================================================

HypothesisVision::Stats HypothesisVision::get_stats() const {
    return impl_->stats_;
}

void HypothesisVision::print_stats() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  📊 HYPOTHESIS GRAPH STATISTICS                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "  Frames processed: " << impl_->stats_.frames_processed << "\n";
    std::cout << "  Current state:\n";
    std::cout << "    Patches:     " << impl_->stats_.total_patches << "\n";
    std::cout << "    Hypotheses:  " << impl_->stats_.total_hypotheses << "\n";
    std::cout << "    Objects:     " << impl_->stats_.total_objects << "\n";
    std::cout << "    Scenes:      " << impl_->stats_.total_scenes << "\n";
    std::cout << "    Concepts:    " << impl_->stats_.total_concepts << "\n";
    std::cout << "    Edges:       " << impl_->stats_.total_edges << "\n\n";
}

// Prediction stubs (future implementation)
void HypothesisVision::predict_next_frame() {}
void HypothesisVision::compare_prediction_to_reality(const cv::Mat& frame) {}

// Visualization stub (future implementation)
cv::Mat HypothesisVision::visualize_graph(const cv::Mat& frame) const {
    cv::Mat vis = frame.clone();
    
    // Draw small crosshair at focus point (minimal, not intrusive)
    cv::drawMarker(vis, impl_->focus_point_, cv::Scalar(0, 255, 255),
                  cv::MARKER_CROSS, 30, 2);
    
    // Draw object bounding boxes
    for (const auto& obj : impl_->objects_) {
        cv::Scalar color = obj.frames_tracked > 10 ? 
            cv::Scalar(0, 255, 0) : cv::Scalar(0, 165, 255);
        
        cv::rectangle(vis, obj.bbox, color, 3);
        
        // Draw velocity arrow
        if (cv::norm(obj.velocity) > 1.0) {
            cv::Point2f end = obj.center + obj.velocity * 3.0f;
            cv::arrowedLine(vis, obj.center, end, cv::Scalar(255, 255, 0), 2);
        }
        
        // Label with tracking info
        std::string label = "obj" + std::to_string(obj.id % 1000);
        if (obj.frames_tracked > 1) {
            label += " [" + std::to_string(obj.frames_tracked) + "f]";
        }
        if (obj.concept_id != 0) {
            label += " C" + std::to_string(obj.concept_id % 1000);
        }
        cv::putText(vis, label, cv::Point(obj.bbox.x, obj.bbox.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
    }
    
    // Draw hypothesis boxes (lighter, only in focus)
    for (const auto& hyp : impl_->hypotheses_) {
        cv::rectangle(vis, hyp.bbox, cv::Scalar(150, 150, 150), 1);
    }
    
    return vis;
}

}} // namespace melvin::vision

