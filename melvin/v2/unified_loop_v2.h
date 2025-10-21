#pragma once

#include "core/types_v2.h"
#include "core/global_workspace.h"
#include "core/neuromodulators.h"
#include "memory/working_memory.h"
#include "memory/semantic_bridge.h"
#include "attention/saliency.h"
#include "attention/topdown.h"
#include "attention/arbitration.h"
#include "evolution/genome.h"
#include "perception/camera_bridge.h"
#include <memory>
#include <vector>
#include <string>

namespace melvin::v2 {

// ============================================================================
// UNIFIED LOOP V2 - Complete Cognitive Cycle
// ============================================================================

/**
 * UnifiedLoopV2 is the main cognitive orchestrator for MELVIN v2.
 * 
 * Implements the complete perception-attention-reasoning-action cycle:
 * 
 * 1. PERCEPTION: Process sensory input → thoughts
 * 2. ATTENTION: Select focus (saliency + topdown + neuromod)
 * 3. WORKING MEMORY: Gate focused items into buffer
 * 4. REASONING: Query semantic memory for related concepts
 * 5. PREDICTION: Generate predictions, compute errors
 * 6. UPDATE: Neuromodulators react to errors
 * 7. OUTPUT: Post results to Global Workspace
 * 8. FEEDBACK: Active concepts bias next cycle
 * 
 * All parameters come from the evolved Genome.
 */
class UnifiedLoopV2 {
public:
    struct Config {
        float target_cycle_hz;       // Target cognitive cycle rate (10-30 Hz)
        bool enable_prediction;      // Enable predictive coding
        bool enable_consolidation;   // Enable sleep/replay
        bool enable_telemetry;       // Record detailed telemetry
        bool verbose;                // Debug output
        
        Config()
            : target_cycle_hz(20.0f), enable_prediction(true),
              enable_consolidation(false), enable_telemetry(true),
              verbose(false) {}
    };
    
    /**
     * Result of one cognitive cycle
     */
    struct CycleResult {
        NodeID focused_object;                  // What was focused
        std::vector<Thought> active_thoughts;   // Current conscious content
        NeuromodState neuromod_state;           // Neuromodulator levels
        std::vector<WMSlot> wm_slots;           // Working memory content
        float total_surprise;                   // Sum of prediction errors
        Timestamp cycle_time_ns;                // How long this cycle took
        size_t cycle_number;                    // Cycle counter
        
        CycleResult() 
            : focused_object(0), total_surprise(0.0f), 
              cycle_time_ns(0), cycle_number(0) {}
    };
    
    UnifiedLoopV2(const evolution::Genome& genome, uint64_t seed);
    UnifiedLoopV2(const evolution::Genome& genome, uint64_t seed, const Config& config);
    ~UnifiedLoopV2();
    
    // ========================================================================
    // MAIN TICK CYCLE
    // ========================================================================
    
    /**
     * Run one complete cognitive cycle
     * 
     * @param image_data - Raw image data (RGB)
     * @param width - Image width
     * @param height - Image height
     * @returns CycleResult with all outputs
     */
    CycleResult tick(const uint8_t* image_data, int width, int height);
    
    /**
     * Run cycle without vision (internal thought only)
     */
    CycleResult tick_no_vision();
    
    // ========================================================================
    // SEMANTIC MEMORY ACCESS
    // ========================================================================
    
    /**
     * Set semantic memory (v1 AtomicGraph)
     * - Must be called before tick()
     */
    void set_semantic_memory(melvin::AtomicGraph* v1_graph);
    
    /**
     * Get semantic bridge
     */
    memory::SemanticBridge* get_semantic_bridge() { return semantic_.get(); }
    
    // ========================================================================
    // GOAL MANAGEMENT
    // ========================================================================
    
    /**
     * Set current goal
     */
    void set_goal(const std::string& goal, float priority = 1.0f);
    
    /**
     * Get current goal
     */
    std::string get_current_goal() const;
    
    // ========================================================================
    // STATE INTROSPECTION
    // ========================================================================
    
    /**
     * Get Global Workspace snapshot
     */
    GlobalWorkspace::Snapshot get_conscious_content() const;
    
    /**
     * Get neuromodulator state
     */
    NeuromodState get_neuromod_state() const;
    
    /**
     * Get working memory state
     */
    std::vector<WMSlot> get_wm_state() const;
    
    // ========================================================================
    // TELEMETRY
    // ========================================================================
    
    /**
     * Export telemetry to JSON file
     * - Contains full history of cycles
     */
    bool export_telemetry(const std::string& path);
    
    /**
     * Get telemetry for specific cycle
     */
    std::string get_cycle_telemetry(size_t cycle_num);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        size_t total_cycles;
        size_t objects_perceived;
        size_t focus_switches;
        size_t wm_gates;
        float avg_cycle_time_ms;
        float avg_surprise;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
private:
    // ========================================================================
    // COMPONENTS (All genome-driven)
    // ========================================================================
    
    evolution::Genome genome_;
    uint64_t seed_;
    Config config_;
    
    // Core modules
    std::unique_ptr<GlobalWorkspace> gw_;
    std::unique_ptr<Neuromodulators> neuromod_;
    std::unique_ptr<memory::WorkingMemory> wm_;
    
    // Attention system
    std::unique_ptr<attention::SaliencyComputer> saliency_;
    std::unique_ptr<attention::TopDownBias> topdown_;
    std::unique_ptr<attention::AttentionArbitration> attention_;
    
    // Perception
    std::unique_ptr<perception::CameraBridge> camera_;
    
    // Memory bridge
    std::unique_ptr<memory::SemanticBridge> semantic_;
    
    // State
    size_t cycle_count_;
    NodeID last_focus_;
    std::vector<CycleResult> telemetry_;
    Stats stats_;
    
    // ========================================================================
    // INTERNAL CYCLE STAGES
    // ========================================================================
    
    // Stage 1: Perception (C++ camera bridge with genome-driven features)
    std::vector<PerceivedObject> perception_stage(const uint8_t* image_data, int width, int height);
    
    // Stage 2: Attention (saliency + topdown + arbitration)
    NodeID attention_stage(const std::vector<PerceivedObject>& objects);
    
    // Stage 3: Working memory gating
    void working_memory_stage(NodeID focused_object, float salience);
    
    // Stage 4: Reasoning (query semantic memory)
    std::vector<NodeID> reasoning_stage(NodeID focused_object);
    
    // Stage 5: Prediction & error computation
    float prediction_stage(const std::vector<NodeID>& active_concepts);
    
    // Stage 6: Update neuromodulators
    void neuromod_update_stage(float prediction_error);
    
    // Stage 7: Global workspace update
    void global_workspace_stage(NodeID focused_object, const std::vector<NodeID>& related);
    
    // ========================================================================
    // HELPERS
    // ========================================================================
    
    void initialize_components();
    void update_statistics(const CycleResult& result, float cycle_time_ms);
};

} // namespace melvin::v2

