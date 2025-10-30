#include "vision_pipeline.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace melvin {
namespace vision {

// ============================================================================
// Stage1_VisionInput
// ============================================================================

Stage1_VisionInput::Stage1_VisionInput()
    : patch_size_(64)
    , fine_patch_size_(8)
    , focus_radius_(150)
{
}

Stage1_VisionInput::Output Stage1_VisionInput::process(const cv::Mat& frame, const cv::Point2f& focus_point) {
    Output output;
    
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    
    // Compute motion
    if (!prev_frame_.empty()) {
        cv::Mat diff;
        cv::absdiff(gray, prev_frame_, diff);
        cv::threshold(diff, output.motion_map, 25, 255, cv::THRESH_BINARY);
    }
    prev_frame_ = gray.clone();
    
    int height = frame.rows;
    int width = frame.cols;
    
    // Coarse grid patches
    for (int py = 0; py < height; py += patch_size_) {
        for (int px = 0; px < width; px += patch_size_) {
            if (py + patch_size_ > height || px + patch_size_ > width) {
                continue;
            }
            
            cv::Rect roi(px, py, patch_size_, patch_size_);
            cv::Mat patch_img = frame(roi);
            
            Patch patch;
            patch.x = px;
            patch.y = py;
            patch.width = patch_size_;
            patch.height = patch_size_;
            patch.avg_color = cv::mean(patch_img);
            
            // Motion
            if (!output.motion_map.empty()) {
                cv::Mat motion_patch = output.motion_map(roi);
                patch.motion = cv::countNonZero(motion_patch) / float(patch_size_ * patch_size_);
            } else {
                patch.motion = 0.0f;
            }
            
            // Saliency (color variance)
            cv::Scalar mean, stddev;
            cv::meanStdDev(patch_img, mean, stddev);
            patch.saliency = (stddev[0] + stddev[1] + stddev[2]) / 3.0f;
            
            output.patches.push_back(patch);
        }
    }
    
    // Fine patches around focus
    if (focus_point.x > 0 && focus_point.y > 0) {
        int fx = static_cast<int>(focus_point.x);
        int fy = static_cast<int>(focus_point.y);
        
        for (int py = fy - focus_radius_; py < fy + focus_radius_; py += fine_patch_size_) {
            for (int px = fx - focus_radius_; px < fx + focus_radius_; px += fine_patch_size_) {
                if (px < 0 || py < 0 || px + fine_patch_size_ > width || py + fine_patch_size_ > height) {
                    continue;
                }
                
                cv::Rect roi(px, py, fine_patch_size_, fine_patch_size_);
                cv::Mat patch_img = frame(roi);
                
                Patch patch;
                patch.x = px;
                patch.y = py;
                patch.width = fine_patch_size_;
                patch.height = fine_patch_size_;
                patch.avg_color = cv::mean(patch_img);
                
                if (!output.motion_map.empty()) {
                    cv::Mat motion_patch = output.motion_map(roi);
                    patch.motion = cv::countNonZero(motion_patch) / float(fine_patch_size_ * fine_patch_size_);
                } else {
                    patch.motion = 0.0f;
                }
                
                cv::Scalar mean, stddev;
                cv::meanStdDev(patch_img, mean, stddev);
                patch.saliency = (stddev[0] + stddev[1] + stddev[2]) / 3.0f;
                
                output.patches.push_back(patch);
            }
        }
    }
    
    output.focus_point = focus_point;
    return output;
}

// ============================================================================
// Stage2_Tokenize
// ============================================================================

Stage2_Tokenize::Stage2_Tokenize()
    : next_node_id_(100000)
    , alpha_(0.15f)
{
    focus_history_ = std::deque<cv::Point2f>(20);
}

std::string Stage2_Tokenize::hash_patch(const Patch& patch) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0);
    oss << patch.x / 10 << "_" << patch.y / 10 << "_"
        << patch.avg_color[0] / 20 << "_"
        << patch.avg_color[1] / 20 << "_"
        << patch.avg_color[2] / 20;
    return oss.str();
}

std::vector<float> Stage2_Tokenize::create_embedding(const Patch& patch) {
    std::vector<float> emb(128);
    
    // Color (normalized)
    emb[0] = patch.avg_color[0] / 255.0f;
    emb[1] = patch.avg_color[1] / 255.0f;
    emb[2] = patch.avg_color[2] / 255.0f;
    
    // Position (normalized to 0-1)
    emb[3] = patch.x / 1000.0f;
    emb[4] = patch.y / 1000.0f;
    
    // Size
    emb[5] = patch.width / 100.0f;
    emb[6] = patch.height / 100.0f;
    
    // Motion & saliency
    emb[7] = patch.motion;
    emb[8] = patch.saliency / 100.0f;
    
    // Fill rest with derived features
    for (size_t i = 9; i < emb.size(); ++i) {
        emb[i] = std::sin(i * 0.1f + emb[i % 9]) * 0.1f;
    }
    
    return emb;
}

cv::Point2f Stage2_Tokenize::compute_focus(const std::vector<Patch>& patches) {
    if (patches.empty()) {
        return cv::Point2f(-1, -1);
    }
    
    // Find patch with highest combined score
    float best_score = -1.0f;
    const Patch* best_patch = nullptr;
    
    for (const auto& patch : patches) {
        float score = patch.motion * 0.6f + patch.saliency * 0.4f;
        if (score > best_score) {
            best_score = score;
            best_patch = &patch;
        }
    }
    
    if (best_patch) {
        return cv::Point2f(
            best_patch->x + best_patch->width / 2.0f,
            best_patch->y + best_patch->height / 2.0f
        );
    }
    
    return cv::Point2f(-1, -1);
}

Stage2_Tokenize::Output Stage2_Tokenize::process(const Stage1_VisionInput::Output& input) {
    Output output;
    output.nodes_created = 0;
    output.nodes_reused = 0;
    
    // Create tokens from patches
    for (const auto& patch : input.patches) {
        std::string hash = hash_patch(patch);
        
        Token token;
        token.patch = patch;
        token.embedding = create_embedding(patch);
        
        // Deduplication
        auto it = patch_to_node_.find(hash);
        if (it != patch_to_node_.end()) {
            token.node_id = it->second;
            output.nodes_reused++;
        } else {
            token.node_id = next_node_id_++;
            patch_to_node_[hash] = token.node_id;
            output.nodes_created++;
        }
        
        output.tokens.push_back(token);
    }
    
    // Compute focus with temporal smoothing
    cv::Point2f current_focus = compute_focus(input.patches);
    
    if (current_focus.x > 0 && current_focus.y > 0) {
        if (!focus_history_.empty()) {
            float fx_avg = 0.0f, fy_avg = 0.0f;
            int count = 0;
            for (const auto& p : focus_history_) {
                if (p.x > 0 && p.y > 0) {
                    fx_avg += p.x;
                    fy_avg += p.y;
                    count++;
                }
            }
            if (count > 0) {
                fx_avg /= count;
                fy_avg /= count;
                
                output.focus_point = cv::Point2f(
                    current_focus.x * (1.0f - alpha_) + fx_avg * alpha_,
                    current_focus.y * (1.0f - alpha_) + fy_avg * alpha_
                );
            } else {
                output.focus_point = current_focus;
            }
        } else {
            output.focus_point = current_focus;
        }
        
        focus_history_.push_back(output.focus_point);
        if (focus_history_.size() > 20) {
            focus_history_.pop_front();
        }
    } else {
        output.focus_point = cv::Point2f(-1, -1);
    }
    
    return output;
}

// ============================================================================
// Stage3_Connect
// ============================================================================

Stage3_Connect::Stage3_Connect()
    : next_object_id_(1000)
{
}

std::string Stage3_Connect::get_dominant_color(const cv::Scalar& color) {
    float b = color[0], g = color[1], r = color[2];
    
    if (r > g && r > b && r > 100) return "RED";
    if (g > r && g > b && g > 100) return "GREEN";
    if (b > r && b > g && b > 100) return "BLUE";
    if (r > 150 && g > 150 && b < 100) return "YELLOW";
    if (r > 100 && g < 80 && b > 100) return "PURPLE";
    if (r < 80 && g < 80 && b < 80) return "BLACK";
    if (r > 200 && g > 200 && b > 200) return "WHITE";
    
    return "UNKNOWN";
}

void Stage3_Connect::update_object_tracking(const std::vector<Token>& tokens) {
    // Simple centroid-based tracking
    const float MAX_DIST = 100.0f;
    
    std::unordered_map<int, bool> matched;
    
    for (const auto& token : tokens) {
        cv::Point2f center(
            token.patch.x + token.patch.width / 2.0f,
            token.patch.y + token.patch.height / 2.0f
        );
        
        // Find closest existing object
        int best_obj = -1;
        float best_dist = MAX_DIST;
        
        for (auto& obj_pair : tracked_objects_) {
            if (matched[obj_pair.first]) continue;
            
            float dx = center.x - obj_pair.second.center.x;
            float dy = center.y - obj_pair.second.center.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist < best_dist) {
                best_dist = dist;
                best_obj = obj_pair.first;
            }
        }
        
        if (best_obj != -1) {
            // Update existing object
            tracked_objects_[best_obj].center = center;
            tracked_objects_[best_obj].frames_tracked++;
            matched[best_obj] = true;
        } else {
            // Create new object
            Object obj;
            obj.id = next_object_id_++;
            obj.center = center;
            obj.color = get_dominant_color(token.patch.avg_color);
            obj.frames_tracked = 1;
            obj.embedding = token.embedding;
            tracked_objects_[obj.id] = obj;
        }
    }
    
    // Remove stale objects (not seen for a while)
    std::vector<int> to_remove;
    for (auto& pair : tracked_objects_) {
        if (!matched[pair.first]) {
            to_remove.push_back(pair.first);
        }
    }
    for (int id : to_remove) {
        tracked_objects_.erase(id);
    }
}

Stage3_Connect::Output Stage3_Connect::process(const Stage2_Tokenize::Output& input) {
    Output output;
    
    update_object_tracking(input.tokens);
    output.objects = tracked_objects_;
    
    // Create edges between nearby tokens
    for (size_t i = 0; i < input.tokens.size(); ++i) {
        for (size_t j = i + 1; j < input.tokens.size(); ++j) {
            const auto& t1 = input.tokens[i];
            const auto& t2 = input.tokens[j];
            
            float dx = (t1.patch.x - t2.patch.x);
            float dy = (t1.patch.y - t2.patch.y);
            float dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist < 150.0f) {
                output.edges.emplace_back(t1.node_id, t2.node_id);
            }
        }
    }
    
    return output;
}

// ============================================================================
// Stage5_Generalize
// ============================================================================

Stage5_Generalize::Stage5_Generalize()
    : next_concept_id_(50000)
    , match_threshold_(0.55f)
    , cluster_distance_(100.0f)
    , learning_rate_(0.15f)
    , edge_confidence_threshold_(0.5f)
{
}

std::vector<Stage5_Generalize::Cluster> Stage5_Generalize::cluster_objects(
    const std::unordered_map<int, Object>& objects
) {
    std::vector<Cluster> clusters;
    
    for (const auto& obj_pair : objects) {
        const Object& obj = obj_pair.second;
        
        if (obj.frames_tracked < 2) continue;
        
        bool added = false;
        for (auto& cluster : clusters) {
            if (cluster.color != obj.color) continue;
            
            float dx = obj.center.x - cluster.center.x;
            float dy = obj.center.y - cluster.center.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist < cluster_distance_) {
                cluster.members.push_back(obj.id);
                cluster.total_frames += obj.frames_tracked;
                cluster.embeddings.push_back(obj.embedding);
                
                // Update center
                cluster.center.x = cluster.center.x * 0.6f + obj.center.x * 0.4f;
                cluster.center.y = cluster.center.y * 0.6f + obj.center.y * 0.4f;
                
                added = true;
                break;
            }
        }
        
        if (!added) {
            Cluster cluster;
            cluster.members.push_back(obj.id);
            cluster.color = obj.color;
            cluster.center = obj.center;
            cluster.total_frames = obj.frames_tracked;
            cluster.embeddings.push_back(obj.embedding);
            clusters.push_back(cluster);
        }
    }
    
    return clusters;
}

int Stage5_Generalize::match_or_create_concept(const Cluster& cluster) {
    // Compute average embedding
    std::vector<float> avg_emb(128, 0.0f);
    for (const auto& emb : cluster.embeddings) {
        for (size_t i = 0; i < emb.size() && i < avg_emb.size(); ++i) {
            avg_emb[i] += emb[i];
        }
    }
    for (float& val : avg_emb) {
        val /= cluster.embeddings.size();
    }
    
    // Find best matching concept
    int best_concept = -1;
    float best_sim = match_threshold_;
    
    for (auto& concept_pair : concepts_) {
        Concept& concept = concept_pair.second;
        
        if (concept.color != cluster.color) continue;
        
        // Cosine similarity
        float dot = 0.0f, norm1 = 0.0f, norm2 = 0.0f;
        for (size_t i = 0; i < avg_emb.size(); ++i) {
            dot += avg_emb[i] * concept.prototype[i];
            norm1 += avg_emb[i] * avg_emb[i];
            norm2 += concept.prototype[i] * concept.prototype[i];
        }
        
        float sim = dot / (std::sqrt(norm1) * std::sqrt(norm2) + 1e-8f);
        
        if (sim > best_sim) {
            best_sim = sim;
            best_concept = concept_pair.first;
        }
    }
    
    if (best_concept != -1) {
        // Update existing concept
        Concept& concept = concepts_[best_concept];
        for (size_t i = 0; i < avg_emb.size(); ++i) {
            concept.prototype[i] = concept.prototype[i] * (1.0f - learning_rate_) + avg_emb[i] * learning_rate_;
        }
        concept.total_frames += cluster.total_frames;
        concept.instances.insert(concept.instances.end(), cluster.members.begin(), cluster.members.end());
        
        evolve_parameters(true);
        return best_concept;
    } else {
        // Create new concept
        if (cluster.total_frames > 8) {
            Concept concept;
            concept.id = next_concept_id_++;
            concept.color = cluster.color;
            concept.prototype = avg_emb;
            concept.instances = cluster.members;
            concept.total_frames = cluster.total_frames;
            concept.is_leap = false;
            concept.is_generative = false;
            
            concepts_[concept.id] = concept;
            evolve_parameters(false);
            return concept.id;
        }
    }
    
    return -1;
}

void Stage5_Generalize::evolve_parameters(bool success) {
    if (success) {
        match_threshold_ = std::min(0.70f, match_threshold_ + 0.001f);
        learning_rate_ = std::min(0.25f, learning_rate_ + 0.001f);
        edge_confidence_threshold_ = std::min(0.9f, edge_confidence_threshold_ + 0.01f);
    } else {
        match_threshold_ = std::max(0.45f, match_threshold_ - 0.002f);
        learning_rate_ = std::max(0.10f, learning_rate_ - 0.001f);
        edge_confidence_threshold_ = std::max(0.3f, edge_confidence_threshold_ - 0.02f);
    }
}

Stage5_Generalize::Output Stage5_Generalize::process(const Stage3_Connect::Output& input) {
    Output output;
    
    auto clusters = cluster_objects(input.objects);
    
    for (const auto& cluster : clusters) {
        int concept_id = match_or_create_concept(cluster);
        
        if (concept_id != -1) {
            output.concepts.push_back(concepts_[concept_id]);
            output.dominant_concepts.push_back(concepts_[concept_id].color);
        }
    }
    
    return output;
}

} // namespace vision
} // namespace melvin
