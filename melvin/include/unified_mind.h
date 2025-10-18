#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include "melvin_vision.h"
#include "melvin_focus.h"
#include "melvin_reasoning.h"
#include "melvin_reflect.h"
#include "melvin_output.h"
#include "context_field.h"
#include <span>
#include <memory>

namespace melvin {

/**
 * UnifiedMind - Complete Cognitive Loop Orchestrator
 * 
 * Implements full perception-reasoning-action cycle:
 * INPUT → PERCEPTION → ATTENTION → REASONING → REFLECTION → OUTPUT
 *   ↑                                                          ↓
 *   └──────────────── FEEDBACK (ActiveConcepts) ─────────────┘
 * 
 * Biological parallel: Cortical-subcortical loops
 * Target: ~10-30 Hz operation
 */
class UnifiedMind {
public:
    UnifiedMind();
    ~UnifiedMind();
    
    /**
     * Run one cognitive cycle
     * 
     * Pipeline:
     * 1. Ingest frame → PERCEPTION (Vision tokenization)
     * 2. Score regions → ATTENTION (Saliency + Goal + Curiosity)
     * 3. Select focus → FOCUS (FEF/SC selection)
     * 4. Infer thought → REASONING (Multi-hop graph reasoning)
     * 5. Compute error → REFLECTION (Predictive coding)
     * 6. Output → MOTOR/SPEECH (Actions and verbalization)
     * 7. Update → FEEDBACK (ActiveConcepts → next cycle)
     * 
     * @param fm Frame metadata
     * @param rgb RGB image data
     */
    void tick(const FrameMeta& fm, std::span<const uint8_t> rgb);
    
    /**
     * Get graph reference (for external queries/persistence)
     */
    AtomicGraph& get_graph() { return *graph_; }
    
    /**
     * Get context field reference (for querying active concepts)
     */
    ContextField& get_context() { return *context_; }
    
    /**
     * Set cognitive mode (affects attention weights dynamically)
     */
    void set_mode(ContextField::CognitiveMode mode);
    
    /**
     * Print system statistics
     */
    void print_stats() const;
    
private:
    // Core components
    std::unique_ptr<AtomicGraph> graph_;
    std::unique_ptr<ContextField> context_;  // NEW: Dynamic context system
    std::unique_ptr<VisionSystem> vision_;
    std::unique_ptr<FocusManager> focus_;
    std::unique_ptr<ReasoningEngine> reason_;
    std::unique_ptr<ReflectionEngine> reflect_;
    std::unique_ptr<OutputManager> output_;
    
    // State
    std::vector<uint64_t> active_concepts_;
    uint64_t cycle_count_;
    float total_time_;
};

} // namespace melvin


