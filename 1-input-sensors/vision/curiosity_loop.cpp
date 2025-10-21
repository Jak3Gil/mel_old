/*
 * Curiosity Loop Implementation
 * Biological curiosity for unknown/uncertain objects
 */

#include "curiosity_loop.h"
#include <iostream>
#include <algorithm>

namespace melvin {
namespace vision {

CuriosityLoop::CuriosityLoop(AtomicGraph* graph)
    : graph_(graph) {
    stats_.active_curiosities = 0;
    stats_.resolved_mysteries = 0;
    stats_.created_unknowns = 0;
    stats_.decayed_objects = 0;
}

CuriosityLoop::~CuriosityLoop() = default;

void CuriosityLoop::observe(const std::string& label, float confidence, uint64_t node_id) {
    // Curiosity triggered by:
    // 1. Low confidence (uncertain detection)
    // 2. "unknown" label (unidentified object)
    
    if (confidence < confidence_threshold_ || label == "unknown" || label.find("unknown:") == 0) {
        // Check if already in curiosity loop
        auto it = active_.find(node_id);
        
        if (it != active_.end()) {
            // Already curious - update
            it->second.frame_count++;
            it->second.confidence = std::max(it->second.confidence, confidence);
        } else {
            // New curiosity
            active_.emplace(node_id, CuriosityObject(node_id, label, confidence));
            stats_.active_curiosities++;
        }
    }
}

void CuriosityLoop::update() {
    for (auto& pair : active_) {
        auto& obj = pair.second;
        
        if (obj.resolved) {
            continue;  // Already resolved
        }
        
        obj.frame_count++;
        
        // If observed for sufficient duration, create unknown node
        if (obj.frame_count >= curiosity_duration_ && !obj.resolved) {
            // Create persistent unknown concept
            std::string unknown_label = "unknown_object_" + std::to_string(obj.id);
            
            uint64_t unknown_id = graph_->get_or_create_node(unknown_label, 1);  // type=1 (instance)
            
            // Link to original detection
            graph_->add_edge(obj.id, unknown_id, OBSERVED_AS, 1.0f);
            
            stats_.created_unknowns++;
            
            // Mark as having created unknown node
            obj.resolved = true;  // "Resolved" by creating unknown node
        }
    }
}

void CuriosityLoop::reinforce(uint64_t id, float similarity) {
    auto it = active_.find(id);
    
    if (it != active_.end()) {
        it->second.last_similarity = similarity;
        
        // High similarity = recognized!
        if (similarity > resolution_threshold_) {
            it->second.resolved = true;
            stats_.resolved_mysteries++;
        }
    }
}

void CuriosityLoop::decay() {
    // Decay confidence for all curiosity objects
    // Remove those that fade below threshold
    
    for (auto it = active_.begin(); it != active_.end(); ) {
        it->second.confidence *= decay_rate_;
        
        if (it->second.confidence < 0.1f) {
            stats_.decayed_objects++;
            it = active_.erase(it);
        } else {
            ++it;
        }
    }
}

CuriosityLoop::Stats CuriosityLoop::get_stats() const {
    Stats s = stats_;
    s.active_curiosities = active_.size();  // Current active count
    return s;
}

void CuriosityLoop::print_stats() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🔍 CURIOSITY LOOP STATISTICS                                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Active curiosities:    " << active_.size() << "\n";
    std::cout << "Resolved mysteries:    " << stats_.resolved_mysteries << "\n";
    std::cout << "Unknown nodes created: " << stats_.created_unknowns << "\n";
    std::cout << "Decayed/forgotten:     " << stats_.decayed_objects << "\n";
    
    if (!active_.empty()) {
        std::cout << "\n🔍 Currently investigating:\n";
        for (const auto& pair : active_) {
            const auto& obj = pair.second;
            std::cout << "  " << obj.label 
                      << " (conf=" << obj.confidence 
                      << ", frames=" << obj.frame_count
                      << ", " << (obj.resolved ? "resolved" : "investigating")
                      << ")\n";
        }
    }
    
    std::cout << "\n";
}

} // namespace vision
} // namespace melvin

