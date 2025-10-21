#include "unified_loop_v2.h"
#include <iostream>
#include <chrono>
#include <fstream>

namespace melvin::v2 {

// ============================================================================
// UNIFIED LOOP V2 IMPLEMENTATION
// ============================================================================

UnifiedLoopV2::UnifiedLoopV2(const evolution::Genome& genome, uint64_t seed)
    : genome_(genome), seed_(seed), cycle_count_(0), last_focus_(0) {
    config_ = Config{};
    stats_ = Stats{};
    initialize_components();
}

UnifiedLoopV2::UnifiedLoopV2(const evolution::Genome& genome, uint64_t seed, const Config& config)
    : genome_(genome), seed_(seed), config_(config), cycle_count_(0), last_focus_(0) {
    stats_ = Stats{};
    initialize_components();
}

UnifiedLoopV2::~UnifiedLoopV2() = default;

void UnifiedLoopV2::initialize_components() {
    // Create all components from genome
    gw_ = std::make_unique<GlobalWorkspace>();
    neuromod_ = std::make_unique<Neuromodulators>(genome_);
    wm_ = std::make_unique<memory::WorkingMemory>(genome_);
    
    // Attention components
    saliency_ = std::make_unique<attention::SaliencyComputer>();
    topdown_ = std::make_unique<attention::TopDownBias>();
    attention_ = std::make_unique<attention::AttentionArbitration>(genome_);
    
    // REAL C++ CAMERA BRIDGE (genome-driven!)
    camera_ = std::make_unique<perception::CameraBridge>(genome_);
    
    // Semantic bridge will be set by set_semantic_memory()
    
    if (config_.verbose) {
        std::cout << "[UnifiedLoopV2] Initialized all components from genome #" 
                  << genome_.id << std::endl;
        std::cout << "[UnifiedLoopV2] Vision system: 100% C++ with OpenCV" << std::endl;
    }
}

// ========================================================================
// MAIN TICK CYCLE
// ========================================================================

UnifiedLoopV2::CycleResult UnifiedLoopV2::tick(const uint8_t* image_data, int width, int height) {
    auto cycle_start = std::chrono::steady_clock::now();
    
    CycleResult result;
    result.cycle_number = ++cycle_count_;
    
    // ====================================================================
    // STAGE 1: PERCEPTION
    // ====================================================================
    
    auto objects = perception_stage(image_data, width, height);
    stats_.objects_perceived += objects.size();
    
    // ====================================================================
    // STAGE 2: ATTENTION
    // ====================================================================
    
    NodeID focused = attention_stage(objects);
    result.focused_object = focused;
    
    if (focused != last_focus_ && focused != 0) {
        stats_.focus_switches++;
        last_focus_ = focused;
    }
    
    // ====================================================================
    // STAGE 3: WORKING MEMORY
    // ====================================================================
    
    if (focused != 0) {
        // Get salience for focused object
        float salience = 0.8f;  // Placeholder
        working_memory_stage(focused, salience);
    }
    
    // ====================================================================
    // STAGE 4: REASONING
    // ====================================================================
    
    std::vector<NodeID> related_concepts;
    if (focused != 0 && semantic_) {
        related_concepts = reasoning_stage(focused);
    }
    
    // ====================================================================
    // STAGE 5: PREDICTION
    // ====================================================================
    
    float prediction_error = 0.0f;
    if (config_.enable_prediction) {
        prediction_error = prediction_stage(related_concepts);
    }
    result.total_surprise = prediction_error;
    
    // ====================================================================
    // STAGE 6: NEUROMODULATOR UPDATE
    // ====================================================================
    
    neuromod_update_stage(prediction_error);
    
    // ====================================================================
    // STAGE 7: GLOBAL WORKSPACE UPDATE
    // ====================================================================
    
    global_workspace_stage(focused, related_concepts);
    
    // ====================================================================
    // STAGE 8: TICK ALL SYSTEMS
    // ====================================================================
    
    auto cycle_end = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(cycle_end - cycle_start).count();
    
    gw_->tick(dt);
    neuromod_->tick(dt);
    wm_->tick(dt);
    
    // ====================================================================
    // COLLECT RESULTS
    // ====================================================================
    
    result.active_thoughts = gw_->get_all_thoughts();
    result.neuromod_state = neuromod_->get_state();
    result.wm_slots = wm_->get_active_slots();
    
    auto cycle_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        cycle_end - cycle_start
    ).count();
    result.cycle_time_ns = cycle_duration;
    
    // Update statistics
    float cycle_time_ms = cycle_duration / 1e6f;
    update_statistics(result, cycle_time_ms);
    
    // Store telemetry
    if (config_.enable_telemetry) {
        telemetry_.push_back(result);
        
        // Keep last 1000 cycles
        if (telemetry_.size() > 1000) {
            telemetry_.erase(telemetry_.begin());
        }
    }
    
    return result;
}

UnifiedLoopV2::CycleResult UnifiedLoopV2::tick_no_vision() {
    // Tick without external input (internal thought only)
    return tick(nullptr, 0, 0);
}

// ========================================================================
// SEMANTIC MEMORY
// ========================================================================

void UnifiedLoopV2::set_semantic_memory(melvin::AtomicGraph* v1_graph) {
    memory::SemanticBridge::Config sem_config;
    sem_config.readonly_mode = false;  // Allow v2 to add knowledge
    sem_config.verbose = config_.verbose;
    
    semantic_ = std::make_unique<memory::SemanticBridge>(v1_graph, sem_config);
    
    if (config_.verbose) {
        auto stats = semantic_->get_graph_stats();
        std::cout << "[UnifiedLoopV2] Semantic memory set: " 
                  << stats.node_count << " nodes, "
                  << stats.edge_count << " edges" << std::endl;
    }
}

// ========================================================================
// GOAL MANAGEMENT
// ========================================================================

void UnifiedLoopV2::set_goal(const std::string& goal, float priority) {
    gw_->set_goal(goal, priority);
}

std::string UnifiedLoopV2::get_current_goal() const {
    return gw_->get_current_goal();
}

// ========================================================================
// STATE INTROSPECTION
// ========================================================================

GlobalWorkspace::Snapshot UnifiedLoopV2::get_conscious_content() const {
    return gw_->get_snapshot();
}

NeuromodState UnifiedLoopV2::get_neuromod_state() const {
    return neuromod_->get_state();
}

std::vector<WMSlot> UnifiedLoopV2::get_wm_state() const {
    return wm_->get_active_slots();
}

// ========================================================================
// TELEMETRY
// ========================================================================

bool UnifiedLoopV2::export_telemetry(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    file << "{\n";
    file << "  \"genome_id\": " << genome_.id << ",\n";
    file << "  \"total_cycles\": " << telemetry_.size() << ",\n";
    file << "  \"cycles\": [\n";
    
    for (size_t i = 0; i < telemetry_.size(); ++i) {
        const auto& result = telemetry_[i];
        
        file << "    {\n";
        file << "      \"cycle\": " << result.cycle_number << ",\n";
        file << "      \"focused_object\": " << result.focused_object << ",\n";
        file << "      \"num_thoughts\": " << result.active_thoughts.size() << ",\n";
        file << "      \"num_wm_slots\": " << result.wm_slots.size() << ",\n";
        file << "      \"surprise\": " << result.total_surprise << ",\n";
        file << "      \"cycle_time_ms\": " << (result.cycle_time_ns / 1e6) << ",\n";
        file << "      \"neuromod\": {\n";
        file << "        \"dopamine\": " << result.neuromod_state.dopamine << ",\n";
        file << "        \"norepinephrine\": " << result.neuromod_state.norepinephrine << ",\n";
        file << "        \"acetylcholine\": " << result.neuromod_state.acetylcholine << ",\n";
        file << "        \"serotonin\": " << result.neuromod_state.serotonin << "\n";
        file << "      }\n";
        file << "    }";
        
        if (i < telemetry_.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    return true;
}

std::string UnifiedLoopV2::get_cycle_telemetry(size_t cycle_num) {
    // TODO: Return JSON for specific cycle
    return "{}";
}

// ========================================================================
// STATISTICS
// ========================================================================

UnifiedLoopV2::Stats UnifiedLoopV2::get_stats() const {
    Stats stats = stats_;
    stats.total_cycles = cycle_count_;
    return stats;
}

void UnifiedLoopV2::reset_stats() {
    stats_ = Stats{};
}

// ========================================================================
// INTERNAL CYCLE STAGES
// ========================================================================

std::vector<PerceivedObject> UnifiedLoopV2::perception_stage(
    const uint8_t* image_data, int width, int height) {
    
    // ========================================================================
    // REAL C++ VISION PROCESSING - No more mocking!
    // ========================================================================
    
    if (!image_data || width <= 0 || height <= 0) {
        return {}; // No visual input
    }
    
    // Process frame through genome-driven camera bridge
    // - Edge detection (genome-controlled threshold)
    // - Motion detection (genome-controlled sensitivity)
    // - Color variance (genome-controlled weighting)
    // - Saliency computation (genome-weighted formula!)
    
    auto objects = camera_->process_frame(image_data, width, height);
    
    if (config_.verbose && !objects.empty()) {
        std::cout << "[Perception] Detected " << objects.size() << " objects ";
        
        if (!objects.empty()) {
            float max_saliency = 0.0f;
            for (const auto& obj : objects) {
                if (obj.features.saliency > max_saliency) {
                    max_saliency = obj.features.saliency;
                }
            }
            std::cout << "(max saliency: " << max_saliency << ")";
        }
        std::cout << std::endl;
    }
    
    return objects;
}

NodeID UnifiedLoopV2::attention_stage(const std::vector<PerceivedObject>& objects) {
    if (objects.empty()) return 0;
    
    // Compute bottom-up saliency
    auto saliency_scores = saliency_->compute_visual(objects);
    
    // Compute top-down relevance
    auto gw_snapshot = gw_->get_snapshot();
    auto relevance_scores = topdown_->compute(objects, gw_snapshot, semantic_.get());
    
    // Select focus (combines both + neuromod)
    auto neuromod_state = neuromod_->get_state();
    NodeID focused = attention_->select_focus(objects, saliency_scores, relevance_scores, neuromod_state);
    
    return focused;
}

void UnifiedLoopV2::working_memory_stage(NodeID focused_object, float salience) {
    // Try to gate into working memory
    float plasticity = neuromod_->get_plasticity_rate();
    
    bool gated = wm_->try_add(focused_object, "focus", salience, plasticity);
    
    if (gated) {
        stats_.wm_gates++;
    }
}

std::vector<NodeID> UnifiedLoopV2::reasoning_stage(NodeID focused_object) {
    if (!semantic_ || focused_object == 0) {
        return {};
    }
    
    // Query related concepts
    auto related = semantic_->find_related(focused_object, 0.5f);
    
    // Limit to top 5
    if (related.size() > 5) {
        related.resize(5);
    }
    
    return related;
}

float UnifiedLoopV2::prediction_stage(const std::vector<NodeID>& active_concepts) {
    // Mock prediction for now
    // In production, would use predictive coding module
    
    // Generate random prediction error for demo
    float prediction_error = (std::rand() % 100) / 500.0f - 0.1f;  // -0.1 to +0.1
    
    return prediction_error;
}

void UnifiedLoopV2::neuromod_update_stage(float prediction_error) {
    // Update neuromodulators based on prediction error
    if (std::abs(prediction_error) > 0.05f) {
        neuromod_->on_prediction_error(prediction_error);
    }
}

void UnifiedLoopV2::global_workspace_stage(NodeID focused_object, 
                                          const std::vector<NodeID>& related) {
    // Post percept thought
    if (focused_object != 0) {
        auto percept = make_percept_thought(focused_object, 0.8f);
        gw_->post(percept);
    }
    
    // Post related concepts as hypotheses
    if (!related.empty()) {
        auto hypothesis = make_hypothesis_thought(related, 0.6f);
        gw_->post(hypothesis);
    }
    
    // Update GW snapshot with WM and neuromod state
    gw_->update_snapshot_external(wm_->get_active_slots(), neuromod_->get_state());
}

void UnifiedLoopV2::update_statistics(const CycleResult& result, float cycle_time_ms) {
    // Running averages
    float n = static_cast<float>(cycle_count_);
    stats_.avg_cycle_time_ms = (stats_.avg_cycle_time_ms * (n - 1) + cycle_time_ms) / n;
    stats_.avg_surprise = (stats_.avg_surprise * (n - 1) + result.total_surprise) / n;
}

} // namespace melvin::v2

