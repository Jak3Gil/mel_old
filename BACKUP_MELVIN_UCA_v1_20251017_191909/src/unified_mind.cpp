#include "unified_mind.h"
#include <iostream>
#include <chrono>

namespace melvin {

UnifiedMind::UnifiedMind()
    : graph_(std::make_unique<AtomicGraph>())
    , vision_(std::make_unique<VisionSystem>(*graph_))
    , focus_(std::make_unique<FocusManager>(*graph_))
    , reason_(std::make_unique<ReasoningEngine>(*graph_))
    , reflect_(std::make_unique<ReflectionEngine>(*graph_))
    , output_(std::make_unique<OutputManager>())
    , cycle_count_(0)
    , total_time_(0)
{}

UnifiedMind::~UnifiedMind() {}

void UnifiedMind::tick(const FrameMeta& fm, std::span<const uint8_t> rgb) {
    auto start = std::chrono::steady_clock::now();
    
    // ========================================================================
    // 1. PERCEPTION - Vision tokenization (V1→V4)
    // ========================================================================
    uint64_t frame_id = vision_->ingest_frame(fm, rgb);
    
    // Group into objects
    auto objects = vision_->group_objects(frame_id);
    
    // ========================================================================
    // 2. ATTENTION - Score regions with saliency + goal + curiosity
    // ========================================================================
    // Set active concepts from previous reasoning
    vision_->set_active_concepts(active_concepts_);
    
    auto candidates = vision_->score_regions(frame_id);
    
    // ========================================================================
    // 3. FOCUS - Select ONE target (FEF/SC)
    // ========================================================================
    auto focus_sel = focus_->select(candidates);
    output_->apply_focus(focus_sel);
    
    // Update inhibition timers
    float dt = 1.0f / constants::TARGET_HZ; // Assume constant dt
    focus_->update(dt);
    
    if (focus_sel.node_id != 0) {
        // Inhibit recently focused nodes
        focus_->inhibit(focus_sel.node_id, 0.8f);
    }
    
    // ========================================================================
    // 4. REASONING - Infer thought from focus (PFC)
    // ========================================================================
    Thought thought;
    if (focus_sel.node_id != 0) {
        thought = reason_->infer_from_focus(focus_sel.node_id);
        
        // Update active concepts
        active_concepts_ = reason_->active_concepts();
        
        // Verbalize (optional)
        if (cycle_count_ % 10 == 0) {
            std::string text = reason_->verbalize(thought);
            output_->say(text);
        }
    }
    
    // ========================================================================
    // 5. REFLECTION - Predictive coding & learning
    // ========================================================================
    if (focus_sel.node_id != 0) {
        float err = reflect_->prediction_error(focus_sel.node_id);
        reflect_->learn_from_error(focus_sel.node_id, err);
    }
    
    // Periodic maintenance (decay, etc.)
    reflect_->maintenance(dt);
    
    // ========================================================================
    // 6. OUTPUT - Log event
    // ========================================================================
    if (focus_sel.node_id != 0 && thought.confidence > 0.5f) {
        output_->log_event(*graph_, "observation", thought.confidence);
    }
    
    // ========================================================================
    // STATS
    // ========================================================================
    cycle_count_++;
    total_time_ += dt;
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float, std::milli>(end - start).count();
    
    if (cycle_count_ % 20 == 0) {
        std::cout << "[Cycle " << cycle_count_ << "] "
                  << "Focus=" << focus_sel.node_id << " "
                  << "Score=" << focus_sel.score << " "
                  << "Time=" << elapsed << "ms\n";
    }
}

void UnifiedMind::print_stats() const {
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout << "║  UnifiedMind Statistics                  ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    std::cout << "  Cycles: " << cycle_count_ << "\n";
    std::cout << "  Total time: " << total_time_ << "s\n";
    std::cout << "  Avg Hz: " << (cycle_count_ / total_time_) << "\n";
    std::cout << "  Active concepts: " << active_concepts_.size() << "\n";
    
    graph_->print_stats();
}

} // namespace melvin


