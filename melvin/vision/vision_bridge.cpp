/*
 * VisionBridge - Visual Perception Integration
 * Converts visual events into AtomicGraph nodes and edges
 */

#include "vision_bridge.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace melvin {
namespace vision {

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class VisionBridge::Impl {
public:
    Config config;
    Stats stats;
    
    // Event history for temporal tracking
    struct TrackedEvent {
        uint64_t node_id;
        std::string label;
        float timestamp;
        int frame_number;
    };
    
    std::vector<TrackedEvent> recent_events;
    uint64_t next_event_id = 0;
    
    Impl(const Config& cfg) : config(cfg) {}
    
    // Create node label in standardized format
    std::string create_node_label(const std::string& label, const std::string& type) {
        std::stringstream ss;
        ss << type << ":" << label;
        return ss.str();
    }
    
    // Check if two events are temporally close
    bool within_temporal_window(float t1, float t2) {
        return std::abs(t1 - t2) <= config.temporal_window;
    }
    
    // Clean old events from history
    void cleanup_old_events(float current_time) {
        recent_events.erase(
            std::remove_if(recent_events.begin(), recent_events.end(),
                [this, current_time](const TrackedEvent& e) {
                    return current_time - e.timestamp > config.temporal_window * 2;
                }),
            recent_events.end()
        );
    }
};

// ============================================================================
// PUBLIC API
// ============================================================================

VisionBridge::VisionBridge() 
    : impl_(std::make_unique<Impl>(Config())) {}

VisionBridge::VisionBridge(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

VisionBridge::~VisionBridge() = default;

void VisionBridge::process(const VisualEvent& event, AtomicGraph& graph) {
    // Check confidence threshold
    if (event.confidence < impl_->config.min_confidence) {
        return;
    }
    
    impl_->stats.events_processed++;
    impl_->stats.avg_confidence = 
        (impl_->stats.avg_confidence * (impl_->stats.events_processed - 1) + event.confidence) 
        / impl_->stats.events_processed;
    
    // Create node for visual concept
    if (impl_->config.create_object_nodes && event.type == "object") {
        std::string node_label = impl_->create_node_label(event.label, "object");
        
        // Get or create node (1 = instance type)
        uint64_t node_id = graph.get_or_create_node(node_label, 1);
        
        // Track if this is new or reused
        bool is_new = (graph.node_count() > impl_->stats.nodes_created + impl_->stats.nodes_reused);
        if (is_new) {
            impl_->stats.nodes_created++;
        } else {
            impl_->stats.nodes_reused++;
        }
        
        // Record for temporal tracking
        impl_->recent_events.push_back({
            node_id,
            event.label,
            event.timestamp,
            event.frame_number
        });
        
        if (impl_->config.verbose) {
            std::cout << "[VisionBridge] " << (is_new ? "Created" : "Reused") 
                      << " node: " << node_label << " (id=" << node_id << ")" << std::endl;
        }
    }
    
    // Cleanup old events
    impl_->cleanup_old_events(event.timestamp);
}

void VisionBridge::process_batch(const std::vector<VisualEvent>& events, AtomicGraph& graph) {
    // Process each event
    for (const auto& event : events) {
        process(event, graph);
    }
    
    // Create spatial edges (objects in same frame)
    create_spatial_edges(events, graph);
}

void VisionBridge::sync_with_audio(
    const std::vector<VisualEvent>& visual_events,
    const std::vector<uint64_t>& audio_node_ids,
    const std::vector<float>& audio_timestamps,
    AtomicGraph& graph
) {
    if (!impl_->config.enable_cross_modal) {
        return;
    }
    
    // For each visual event, find temporally close audio events
    for (const auto& vis_event : visual_events) {
        if (vis_event.confidence < impl_->config.min_confidence) {
            continue;
        }
        
        // Get visual node
        std::string vis_label = impl_->create_node_label(vis_event.label, "object");
        uint64_t vis_node_id = graph.get_or_create_node(vis_label, 1);
        
        // Find matching audio nodes
        for (size_t i = 0; i < audio_node_ids.size(); ++i) {
            if (impl_->within_temporal_window(vis_event.timestamp, audio_timestamps[i])) {
                // Create cross-modal edge
                graph.add_edge(vis_node_id, audio_node_ids[i], CO_OCCURS_WITH, 1.0f);
                impl_->stats.cross_modal_links++;
                
                if (impl_->config.verbose) {
                    std::cout << "[VisionBridge] Cross-modal link: visual:" << vis_event.label 
                              << " ↔ audio (t_diff=" << std::abs(vis_event.timestamp - audio_timestamps[i]) 
                              << "s)" << std::endl;
                }
            }
        }
    }
}

void VisionBridge::create_temporal_edges(
    const std::vector<VisualEvent>& current_events,
    const std::vector<VisualEvent>& previous_events,
    AtomicGraph& graph
) {
    // Link objects that appear in consecutive frames
    for (const auto& curr : current_events) {
        if (curr.confidence < impl_->config.min_confidence) continue;
        
        std::string curr_label = impl_->create_node_label(curr.label, "object");
        uint64_t curr_node = graph.get_or_create_node(curr_label, 1);
        
        for (const auto& prev : previous_events) {
            if (prev.confidence < impl_->config.min_confidence) continue;
            
            // Only link if same object type or different objects in temporal sequence
            std::string prev_label = impl_->create_node_label(prev.label, "object");
            uint64_t prev_node = graph.get_or_create_node(prev_label, 1);
            
            if (curr_node != prev_node) {
                // Create temporal edge (LEAP)
                graph.add_edge(prev_node, curr_node, TEMPORAL_NEXT, 0.5f);
                impl_->stats.temporal_edges_created++;
                
                if (impl_->config.verbose) {
                    std::cout << "[VisionBridge] Temporal: " << prev.label 
                              << " → " << curr.label << std::endl;
                }
            }
        }
    }
}

void VisionBridge::create_spatial_edges(
    const std::vector<VisualEvent>& events,
    AtomicGraph& graph
) {
    // Create edges between all objects that appear together in same frame
    if (events.size() < 2) return;
    
    // Get all node IDs for this frame
    std::vector<uint64_t> node_ids;
    for (const auto& event : events) {
        if (event.confidence < impl_->config.min_confidence) continue;
        
        std::string label = impl_->create_node_label(event.label, "object");
        uint64_t node_id = graph.get_or_create_node(label, 1);
        node_ids.push_back(node_id);
    }
    
    // Create edges between all pairs (co-occurrence)
    for (size_t i = 0; i < node_ids.size(); ++i) {
        for (size_t j = i + 1; j < node_ids.size(); ++j) {
            if (node_ids[i] != node_ids[j]) {
                // EXACT edge (spatial co-occurrence)
                graph.add_edge(node_ids[i], node_ids[j], CO_OCCURS_WITH, 1.0f);
                impl_->stats.spatial_edges_created++;
                
                if (impl_->config.verbose) {
                    std::cout << "[VisionBridge] Spatial: " << events[i].label 
                              << " ↔ " << events[j].label << std::endl;
                }
            }
        }
    }
}

uint64_t VisionBridge::get_or_create_visual_node(
    const std::string& label,
    const std::string& type,
    AtomicGraph& graph
) {
    std::string node_label = impl_->create_node_label(label, type);
    return graph.get_or_create_node(node_label, 1);
}

void VisionBridge::record_event(const VisualEvent& event, uint64_t node_id) {
    impl_->recent_events.push_back({
        node_id,
        event.label,
        event.timestamp,
        event.frame_number
    });
}

// ============================================================================
// STATISTICS
// ============================================================================

VisionBridge::Stats VisionBridge::get_stats() const {
    return impl_->stats;
}

void VisionBridge::reset_stats() {
    impl_->stats = Stats();
}

void VisionBridge::print_stats() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  👁️  VISION BRIDGE STATISTICS                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Events processed:      " << impl_->stats.events_processed << "\n";
    std::cout << "Nodes created:         " << impl_->stats.nodes_created << "\n";
    std::cout << "Nodes reused:          " << impl_->stats.nodes_reused << "\n";
    std::cout << "Spatial edges:         " << impl_->stats.spatial_edges_created << "\n";
    std::cout << "Temporal edges:        " << impl_->stats.temporal_edges_created << "\n";
    std::cout << "Cross-modal links:     " << impl_->stats.cross_modal_links << "\n";
    std::cout << "Avg confidence:        " << impl_->stats.avg_confidence << "\n";
    
    if (impl_->stats.events_processed > 0) {
        float reuse_rate = 100.0f * impl_->stats.nodes_reused / 
                          (impl_->stats.nodes_created + impl_->stats.nodes_reused);
        std::cout << "Node reuse rate:       " << reuse_rate << "%\n";
    }
    
    std::cout << std::endl;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void VisionBridge::set_config(const Config& config) {
    impl_->config = config;
}

const VisionBridge::Config& VisionBridge::get_config() const {
    return impl_->config;
}

} // namespace vision
} // namespace melvin

