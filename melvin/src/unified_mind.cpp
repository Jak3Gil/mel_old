#include "unified_mind.h"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace melvin {

UnifiedMind::UnifiedMind()
    : graph_(std::make_unique<AtomicGraph>())
    , context_(std::make_unique<ContextField>(*graph_))
    , vision_(std::make_unique<VisionSystem>(*graph_))
    , focus_(std::make_unique<FocusManager>(*graph_))
    , reason_(std::make_unique<ReasoningEngine>(*graph_))
    , reflect_(std::make_unique<ReflectionEngine>(*graph_))
    , output_(std::make_unique<OutputManager>())
    , cycle_count_(0)
    , total_time_(0)
{
    std::cout << "🧠 UnifiedMind initialized with DYNAMIC context-driven attention" << std::endl;
}

UnifiedMind::~UnifiedMind() {}

void UnifiedMind::set_mode(ContextField::CognitiveMode mode) {
    context_->set_mode(mode);
    std::cout << "🔄 Cognitive mode changed to: ";
    switch (mode) {
        case ContextField::EXPLORING: std::cout << "EXPLORING (curiosity-driven)"; break;
        case ContextField::SEARCHING: std::cout << "SEARCHING (goal-driven)"; break;
        case ContextField::MONITORING: std::cout << "MONITORING (balanced)"; break;
        case ContextField::LEARNING: std::cout << "LEARNING (memory-focused)"; break;
    }
    std::cout << std::endl;
}

void UnifiedMind::tick(const FrameMeta& fm, std::span<const uint8_t> rgb) {
    auto start = std::chrono::steady_clock::now();
    
    // ========================================================================
    // 1. PERCEPTION - Vision tokenization (V1→V4)
    // ========================================================================
    uint64_t frame_id = vision_->ingest_frame(fm, rgb);
    
    // Group into objects
    auto objects = vision_->group_objects(frame_id);
    
    // ========================================================================
    // 2. CONTEXT UPDATE - Update activation field (working memory)
    // ========================================================================
    // Inject perceived nodes into context
    context_->update_from_perception(objects);
    
    // Spread activation through graph
    context_->diffuse(0.05f);
    context_->decay();
    
    // Get active concepts from context field
    active_concepts_ = context_->active(0.2f);
    
    // ========================================================================
    // 3. ATTENTION - Score regions with DYNAMIC weights from context
    // ========================================================================
    // Set active concepts from context
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
        
        // Update context from reasoning (reinforces thought concepts)
        context_->update_from_thought(thought.subj, thought.pred_concept, thought.obj);
        
        // Update active concepts from context
        active_concepts_ = context_->active(0.2f);
        
        // Verbalize (optional) - now includes context state!
        if (cycle_count_ % 10 == 0) {
            std::string text = reason_->verbalize(thought);
            output_->say(text);
            
            // Show context summary
            std::cout << "   " << context_->get_summary() << std::endl;
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
    std::cout << "\n  " << context_->get_summary() << "\n";
    
    // Show dynamic attention weights
    auto weights = context_->compute_dynamic_weights();
    std::cout << "\n  Current attention formula:\n";
    std::cout << "    F = " << std::fixed << std::setprecision(2) 
              << weights.alpha << "·S + " 
              << weights.beta << "·G + " 
              << weights.gamma << "·C\n";
    std::cout << "    (" << weights.reason << ")\n";
    
    graph_->print_stats();
    context_->print_stats();
}

} // namespace melvin


