#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include <string>
#include <string_view>

namespace melvin {

/**
 * OutputManager - Motor Cortex + Speech
 * 
 * Biological analog:
 * - Primary Motor Cortex (M1): Motor commands
 * - Broca's area: Speech production
 * - Event logging → Episodic memory formation
 */
class OutputManager {
public:
    OutputManager();
    
    /**
     * Apply focus selection (gaze/ROI shift)
     * @param f Focus selection result
     */
    void apply_focus(const FocusSelection& f);
    
    /**
     * Generate speech output
     * @param text Text to output
     */
    void say(std::string_view text);
    
    /**
     * Log event to graph
     * @param graph Graph to update
     * @param what Event description
     * @param conf Confidence level
     */
    void log_event(AtomicGraph& graph, std::string_view what, float conf);
    
private:
    uint64_t current_focus_node_;
};

} // namespace melvin


