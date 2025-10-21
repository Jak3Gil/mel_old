/*
 * Visual Tokenization - Unsupervised Object Learning
 * 
 * Converts frames into:
 *   Patches → Visual Nodes → Object Clusters → Scene Nodes
 * 
 * No external object detection - pure Gestalt grouping!
 */

#include "visual_tokenization.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace melvin {
namespace vision {

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class VisualTokenizer::Impl {
public:
    Config config;
    Stats stats;
    
    // Temporal memory
    std::vector<Scene> recent_scenes;
    std::unordered_map<uint64_t, VisualFeatures> prev_frame_patches;
    
    uint64_t frame_counter = 0;
    NodeID next_node_id = 1;
    
    Impl(const Config& cfg) : config(cfg) {}
    
    // Calculate motion between current and previous frame patch
    float calculate_motion(const VisualFeatures& current, int px, int py) {
        uint64_t prev_key = (static_cast<uint64_t>(px) << 32) | static_cast<uint64_t>(py);
        
        auto it = prev_frame_patches.find(prev_key);
        if (it == prev_frame_patches.end()) {
            return 0.0f;  // No previous frame
        }
        
        const VisualFeatures& prev = it->second;
        
        // Simple motion: change in brightness + color
        float brightness_change = std::abs(current.brightness - prev.brightness);
        float color_change = std::abs(current.avg_red - prev.avg_red) +
                            std::abs(current.avg_green - prev.avg_green) +
                            std::abs(current.avg_blue - prev.avg_blue);
        
        return (brightness_change + color_change / 255.0f) * 0.5f;
    }
    
    // Calculate texture entropy (complexity measure)
    float calculate_texture_entropy(const uint8_t* patch_data, int patch_size) {
        // Simple histogram-based entropy
        int histogram[256] = {0};
        int total_pixels = patch_size * patch_size;
        
        for (int i = 0; i < total_pixels; ++i) {
            // Use green channel as grayscale approximation
            int idx = i * 3 + 1;
            histogram[patch_data[idx]]++;
        }
        
        float entropy = 0.0f;
        for (int i = 0; i < 256; ++i) {
            if (histogram[i] > 0) {
                float p = static_cast<float>(histogram[i]) / total_pixels;
                entropy -= p * std::log2(p);
            }
        }
        
        return entropy / 8.0f;  // Normalize to 0-1
    }
    
    // Calculate edge density using simple gradient
    float calculate_edge_density(const uint8_t* patch_data, int patch_size) {
        int edge_count = 0;
        int total = 0;
        
        for (int y = 1; y < patch_size - 1; ++y) {
            for (int x = 1; x < patch_size - 1; ++x) {
                int idx = (y * patch_size + x) * 3 + 1;  // Green channel
                int center = patch_data[idx];
                int right = patch_data[idx + 3];
                int down = patch_data[idx + patch_size * 3];
                
                // Simple edge detection
                int gx = std::abs(center - right);
                int gy = std::abs(center - down);
                
                if (gx + gy > 30) {  // Threshold
                    edge_count++;
                }
                total++;
            }
        }
        
        return total > 0 ? static_cast<float>(edge_count) / total : 0.0f;
    }
};

// ============================================================================
// PUBLIC API
// ============================================================================

VisualTokenizer::VisualTokenizer(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

VisualTokenizer::~VisualTokenizer() = default;

VisualFeatures VisualTokenizer::extract_patch_features(
    const uint8_t* frame_data,
    int frame_width,
    int patch_x,
    int patch_y,
    int patch_size
) {
    VisualFeatures f;
    f.patch_x = patch_x;
    f.patch_y = patch_y;
    
    // Extract patch region
    std::vector<uint8_t> patch_data;
    patch_data.reserve(patch_size * patch_size * 3);
    
    for (int y = 0; y < patch_size; ++y) {
        for (int x = 0; x < patch_size; ++x) {
            int px = patch_x * patch_size + x;
            int py = patch_y * patch_size + y;
            int idx = (py * frame_width + px) * 3;
            
            patch_data.push_back(frame_data[idx]);     // B
            patch_data.push_back(frame_data[idx + 1]); // G
            patch_data.push_back(frame_data[idx + 2]); // R
        }
    }
    
    // Calculate average color
    float sum_r = 0, sum_g = 0, sum_b = 0;
    int pixel_count = patch_size * patch_size;
    
    for (int i = 0; i < pixel_count; ++i) {
        sum_b += patch_data[i * 3];
        sum_g += patch_data[i * 3 + 1];
        sum_r += patch_data[i * 3 + 2];
    }
    
    f.avg_red = sum_r / pixel_count;
    f.avg_green = sum_g / pixel_count;
    f.avg_blue = sum_b / pixel_count;
    f.brightness = (f.avg_red + f.avg_green + f.avg_blue) / 3.0f;
    
    // Calculate texture
    f.texture_entropy = impl_->calculate_texture_entropy(patch_data.data(), patch_size);
    
    // Calculate edges
    f.edge_density = impl_->calculate_edge_density(patch_data.data(), patch_size);
    
    // Calculate motion
    f.motion_magnitude = impl_->calculate_motion(f, patch_x, patch_y);
    
    return f;
}

std::vector<ObjectCluster> VisualTokenizer::cluster_patches(
    const std::vector<VisualPatch>& patches
) {
    std::vector<ObjectCluster> clusters;
    std::vector<bool> assigned(patches.size(), false);
    
    // Gestalt clustering: group similar adjacent patches
    for (size_t i = 0; i < patches.size(); ++i) {
        if (assigned[i]) continue;
        
        ObjectCluster cluster;
        cluster.patch_ids.push_back(patches[i].node_id);
        cluster.centroid = patches[i].features;
        cluster.min_x = cluster.max_x = patches[i].features.patch_x;
        cluster.min_y = cluster.max_y = patches[i].features.patch_y;
        assigned[i] = true;
        
        // Grow cluster
        bool grew = true;
        while (grew && cluster.patch_ids.size() < static_cast<size_t>(impl_->config.max_cluster_size)) {
            grew = false;
            
            for (size_t j = 0; j < patches.size(); ++j) {
                if (assigned[j]) continue;
                
                // Check if patch is similar to cluster centroid AND spatially adjacent
                float similarity = patches[j].features.similarity(cluster.centroid);
                bool adjacent = false;
                
                // Check adjacency to any cluster member
                for (size_t k = 0; k < patches.size(); ++k) {
                    if (assigned[k] && 
                        std::find(cluster.patch_ids.begin(), cluster.patch_ids.end(), 
                                 patches[k].node_id) != cluster.patch_ids.end()) {
                        if (patches[j].features.is_adjacent(patches[k].features)) {
                            adjacent = true;
                            break;
                        }
                    }
                }
                
                if (similarity > impl_->config.similarity_threshold && adjacent) {
                    cluster.patch_ids.push_back(patches[j].node_id);
                    assigned[j] = true;
                    grew = true;
                    
                    // Update bounds
                    cluster.min_x = std::min(cluster.min_x, patches[j].features.patch_x);
                    cluster.max_x = std::max(cluster.max_x, patches[j].features.patch_x);
                    cluster.min_y = std::min(cluster.min_y, patches[j].features.patch_y);
                    cluster.max_y = std::max(cluster.max_y, patches[j].features.patch_y);
                    
                    // Update centroid (running average)
                    int n = cluster.patch_ids.size();
                    cluster.centroid.avg_red = 
                        (cluster.centroid.avg_red * (n-1) + patches[j].features.avg_red) / n;
                    cluster.centroid.avg_green = 
                        (cluster.centroid.avg_green * (n-1) + patches[j].features.avg_green) / n;
                    cluster.centroid.avg_blue = 
                        (cluster.centroid.avg_blue * (n-1) + patches[j].features.avg_blue) / n;
                    cluster.centroid.texture_entropy = 
                        (cluster.centroid.texture_entropy * (n-1) + patches[j].features.texture_entropy) / n;
                }
            }
        }
        
        cluster.size = cluster.patch_ids.size();
        
        // Only keep clusters above minimum size
        if (cluster.size >= impl_->config.min_cluster_size) {
            // Calculate coherence (how well patches fit together)
            float total_similarity = 0.0f;
            for (const auto& patch : patches) {
                if (std::find(cluster.patch_ids.begin(), cluster.patch_ids.end(), 
                             patch.node_id) != cluster.patch_ids.end()) {
                    total_similarity += patch.features.similarity(cluster.centroid);
                }
            }
            cluster.coherence = total_similarity / cluster.size;
            
            cluster.object_node_id = impl_->next_node_id++;
            clusters.push_back(cluster);
        }
    }
    
    return clusters;
}

NodeID VisualTokenizer::create_scene_node(
    const std::vector<ObjectCluster>& objects,
    uint64_t frame_id
) {
    Scene scene;
    scene.scene_node_id = impl_->next_node_id++;
    scene.frame_id = frame_id;
    scene.timestamp = impl_->frame_counter;
    
    for (const auto& obj : objects) {
        scene.object_ids.push_back(obj.object_node_id);
    }
    
    // Add to recent memory
    impl_->recent_scenes.push_back(scene);
    if (impl_->recent_scenes.size() > static_cast<size_t>(impl_->config.scene_window)) {
        impl_->recent_scenes.erase(impl_->recent_scenes.begin());
    }
    
    impl_->stats.scenes_recorded++;
    impl_->stats.objects_formed += objects.size();
    
    // Update averages
    impl_->stats.avg_objects_per_scene = 
        static_cast<float>(impl_->stats.objects_formed) / impl_->stats.scenes_recorded;
    
    return scene.scene_node_id;
}

NodeID VisualTokenizer::process_frame(
    const uint8_t* frame_data,
    int width,
    int height,
    const std::vector<float>& attention_map
) {
    impl_->frame_counter++;
    impl_->stats.frames_processed++;
    
    // Step 1: Tokenize into patches
    std::vector<VisualPatch> patches;
    
    int grid_w = width / impl_->config.patch_size;
    int grid_h = height / impl_->config.patch_size;
    
    for (int py = 0; py < grid_h; ++py) {
        for (int px = 0; px < grid_w; ++px) {
            // Check attention score
            int attention_idx = py * grid_w + px;
            float attention = (attention_idx < static_cast<int>(attention_map.size())) 
                            ? attention_map[attention_idx] : 0.0f;
            
            if (attention > impl_->config.attention_threshold) {
                VisualPatch patch;
                patch.features = extract_patch_features(
                    frame_data, width, px, py, impl_->config.patch_size
                );
                patch.features.frame_id = impl_->frame_counter;
                patch.attention_score = attention;
                patch.node_id = impl_->next_node_id++;
                
                patches.push_back(patch);
                impl_->stats.patches_created++;
                
                // Store for next frame motion calculation
                uint64_t key = (static_cast<uint64_t>(px) << 32) | static_cast<uint64_t>(py);
                impl_->prev_frame_patches[key] = patch.features;
            }
        }
    }
    
    // Update stats
    impl_->stats.avg_patches_per_frame = 
        static_cast<float>(impl_->stats.patches_created) / impl_->stats.frames_processed;
    
    if (patches.empty()) {
        if (impl_->config.verbose) {
            std::cout << "[VisualTokenizer] Frame " << impl_->frame_counter 
                      << ": No patches above attention threshold\n";
        }
        return 0;  // No scene created
    }
    
    // Step 2: Cluster patches into objects
    std::vector<ObjectCluster> objects = cluster_patches(patches);
    
    // Calculate average coherence
    if (!objects.empty()) {
        float total_coherence = 0.0f;
        for (const auto& obj : objects) {
            total_coherence += obj.coherence;
        }
        impl_->stats.avg_cluster_coherence = total_coherence / objects.size();
    }
    
    if (impl_->config.verbose) {
        std::cout << "[VisualTokenizer] Frame " << impl_->frame_counter << ":\n";
        std::cout << "  Patches: " << patches.size() << "\n";
        std::cout << "  Objects: " << objects.size() << "\n";
        for (const auto& obj : objects) {
            std::cout << "    Object " << obj.object_node_id 
                      << ": " << obj.size << " patches"
                      << " coherence=" << obj.coherence << "\n";
        }
    }
    
    // Step 3: Create scene node
    NodeID scene_id = create_scene_node(objects, impl_->frame_counter);
    
    return scene_id;
}

VisualTokenizer::Stats VisualTokenizer::get_stats() const {
    return impl_->stats;
}

void VisualTokenizer::print_stats() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  📊 VISUAL TOKENIZATION STATISTICS                       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "  Frames processed: " << impl_->stats.frames_processed << "\n";
    std::cout << "  Patches created: " << impl_->stats.patches_created << "\n";
    std::cout << "  Objects formed: " << impl_->stats.objects_formed << "\n";
    std::cout << "  Scenes recorded: " << impl_->stats.scenes_recorded << "\n\n";
    
    std::cout << "  Avg patches/frame: " << impl_->stats.avg_patches_per_frame << "\n";
    std::cout << "  Avg objects/scene: " << impl_->stats.avg_objects_per_scene << "\n";
    std::cout << "  Avg cluster coherence: " << impl_->stats.avg_cluster_coherence << "\n\n";
}

}} // namespace melvin::vision

