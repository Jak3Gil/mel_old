#include "melvin_output.h"
#include <iostream>

namespace melvin {

OutputManager::OutputManager()
    : current_focus_node_(0)
{}

void OutputManager::apply_focus(const FocusSelection& f) {
    current_focus_node_ = f.node_id;
    // In real implementation, would send ROI/gaze command to camera/motors
}

void OutputManager::say(std::string_view text) {
    // Optional speech output
    if (!text.empty()) {
        std::cout << "🗣️  Melvin: " << text << std::endl;
    }
}

void OutputManager::log_event(AtomicGraph& graph, std::string_view what, float conf) {
    // Create event node
    uint64_t event_id = graph.get_or_create_concept(std::string(what));
    
    // Link to current focus
    if (current_focus_node_ != 0) {
        graph.add_or_bump_edge(event_id, current_focus_node_, Rel::OBSERVED_AS, conf);
    }
}

} // namespace melvin


