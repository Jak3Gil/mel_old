#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include <span>
#include <vector>
#include <utility>

namespace melvin {

// Forward declaration
class VisualContext;

/**
 * VisionSystem - Retina → V1/V2/V4 → IT
 * 
 * Biological analog:
 * - Retina/LGN: Frame ingestion
 * - V1: Low-level features (edges, contrast, motion)
 * - V2: Gestalt grouping
 * - V4: Object formation
 * - IT: Concept linking
 * - Parietal: Saliency computation
 */
class VisionSystem {
public:
    struct Scores {
        float saliency;      // Bottom-up (contrast, motion, novelty)
        float goal;          // Top-down (relevance to active concepts)
        float curiosity;     // Prediction error
        float diversity;     // NEW: Visual variety seeking
    };
    
    explicit VisionSystem(AtomicGraph& graph);
    
    /**
     * Ingest frame and tokenize into patches
     * @param m Frame metadata
     * @param rgb RGB image data (row-major, 3 bytes per pixel)
     * @return Frame node ID
     */
    uint64_t ingest_frame(const FrameMeta& m, std::span<const uint8_t> rgb);
    
    /**
     * Score visual regions for attention
     * @param frame_node Frame node ID
     * @return List of (node_id, scores) pairs
     */
    std::vector<std::pair<uint64_t, Scores>> score_regions(uint64_t frame_node);
    
    /**
     * Group patches into objects
     * @param frame_node Frame node ID
     * @return List of object node IDs
     */
    std::vector<uint64_t> group_objects(uint64_t frame_node);
    
    /**
     * Set active concepts for top-down modulation
     */
    void set_active_concepts(const std::vector<uint64_t>& concepts);
    
private:
    AtomicGraph& graph_;
    std::vector<uint64_t> active_concepts_;
    uint64_t prev_frame_id_;
    std::vector<std::pair<uint64_t, float>> prev_patches_; // (patch_id, luminance)
};

} // namespace melvin


