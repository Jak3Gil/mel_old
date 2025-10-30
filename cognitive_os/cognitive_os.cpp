/**
 * @file cognitive_os.cpp
 * @brief Implementation of Cognitive OS
 */

#include "cognitive_os.h"
#include <chrono>
#include <thread>
#include <algorithm>
#include <iostream>
#include <random>

namespace melvin {
namespace cognitive_os {

CognitiveOS::CognitiveOS() : field_(nullptr), intelligence_(nullptr) {
    // Initialize arousal to balanced state
    arousal_.noradrenaline = 0.5f;
    arousal_.dopamine = 0.5f;
    arousal_.acetylcholine = 0.5f;
}

CognitiveOS::~CognitiveOS() {
    stop();
}

void CognitiveOS::attach(
    melvin::intelligence::UnifiedIntelligence* intelligence,
    FieldFacade* field
) {
    intelligence_ = intelligence;
    field_ = field;
}

void CognitiveOS::start() {
    if (running_.load(std::memory_order_relaxed)) {
        return;
    }
    
    running_.store(true, std::memory_order_relaxed);
    
    // Start scheduler thread
    scheduler_thread_ = std::thread([this]() {
        scheduler_loop();
    });
    
    std::cout << "✅ Cognitive OS started\n";
    std::cout << "   Services running at natural frequencies\n";
    std::cout << "   Scheduler: 50 Hz (20ms ticks)\n";
}

void CognitiveOS::stop() {
    if (!running_.load(std::memory_order_relaxed)) {
        return;
    }
    
    running_.store(false, std::memory_order_relaxed);
    
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    
    std::cout << "✅ Cognitive OS stopped\n";
}

void CognitiveOS::join() {
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// SCHEDULER LOOP (50 Hz - 20ms ticks)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void CognitiveOS::scheduler_loop() {
    const auto tick_period = std::chrono::milliseconds(20);  // 50 Hz
    auto next_tick = std::chrono::steady_clock::now();
    
    while (running_.load(std::memory_order_relaxed)) {
        auto start = std::chrono::steady_clock::now();
        
        // Run one tick
        run_tick();
        
        // Sleep until next tick
        next_tick += tick_period;
        std::this_thread::sleep_until(next_tick);
        
        total_ticks_++;
    }
}

void CognitiveOS::run_tick() {
    auto tick_start = std::chrono::high_resolution_clock::now();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 1. GET FIELD METRICS
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    auto field_metrics_raw = field_->get_metrics();
    
    FieldMetrics metrics;
    metrics.timestamp = get_timestamp();
    metrics.active_nodes = field_metrics_raw.active_nodes;
    metrics.energy_variance = field_metrics_raw.energy_variance;
    metrics.sparsity = field_metrics_raw.sparsity;
    metrics.entropy = field_metrics_raw.entropy;
    metrics.coherence = 0.0f;  // Computed by cognition
    metrics.confidence = 0.0f;  // Computed by cognition
    
    // Publish to bus
    bus_.publish(topics::FIELD_METRICS, metrics);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 2. COMPUTE AROUSAL (neuromodulator analog)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    compute_arousal(metrics);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 3. ADAPT BUDGETS
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    float cpu_load = estimate_cpu_load();
    adapt_budgets(metrics, cpu_load);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 4. UPDATE GENOME FROM AROUSAL
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    update_genome_from_arousal();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 5. RUN SERVICES (inline for now, at varying rates)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Cognition: 30 Hz (run every 1-2 ticks)
    if (total_ticks_ % 2 == 0) {
        tick_cognition(budgets_.cognition);
    }
    
    // Attention: 60 Hz (run every tick)
    tick_attention(budgets_.attention);
    
    // Working Memory: 30 Hz
    if (total_ticks_ % 2 == 0) {
        tick_working_memory(budgets_.wm);
    }
    
    // Learning: 10 Hz (run every 5 ticks)
    if (total_ticks_ % 5 == 0) {
        tick_learning(budgets_.learning);
    }
    
    // Reflection: 5 Hz (run every 10 ticks)
    if (total_ticks_ % 10 == 0) {
        tick_reflection(budgets_.reflection);
    }
    
    // Field maintenance: every tick
    tick_field_maintenance(0.5f);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 6. LOG METRICS
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    auto tick_end = std::chrono::high_resolution_clock::now();
    auto tick_duration = std::chrono::duration<double, std::milli>(tick_end - tick_start);
    
    SystemKPIs kpis;
    kpis.timestamp = metrics.timestamp;
    kpis.active_nodes = metrics.active_nodes;
    kpis.energy_variance = metrics.energy_variance;
    kpis.sparsity = metrics.sparsity;
    kpis.entropy = metrics.entropy;
    kpis.coherence = metrics.coherence;
    kpis.confidence = metrics.confidence;
    kpis.fps = (tick_duration.count() > 0) ? (1000.0f / tick_duration.count()) : 0.0f;
    kpis.cpu_usage = cpu_load;
    kpis.gpu_usage = 0.0f;
    kpis.dropped_msgs = bus_.dropped_messages();
    kpis.services_active = 6;
    kpis.avg_service_load = cpu_load;
    
    metrics_.log(kpis);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// AROUSAL COMPUTATION (Neuromodulator Analog)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void CognitiveOS::compute_arousal(const FieldMetrics& metrics) {
    // Noradrenaline: high when novelty/entropy high
    arousal_.noradrenaline = std::min(1.0f, metrics.entropy / 5.0f);
    
    // Dopamine: high when confidence high (from intelligence)
    if (intelligence_) {
        arousal_.dopamine = intelligence_->metrics().confidence;
    }
    
    // Acetylcholine: high when need focus (low coherence)
    arousal_.acetylcholine = 1.0f - std::min(1.0f, metrics.sparsity);
}

void CognitiveOS::update_genome_from_arousal() {
    if (!intelligence_) return;
    
    auto& genome = const_cast<melvin::evolution::DynamicGenome&>(intelligence_->genome());
    auto& params = genome.reasoning_params();
    
    // Temperature ← noradrenaline (high arousal = more exploration)
    params.temperature = 0.5f + arousal_.noradrenaline;
    
    // Semantic threshold ← acetylcholine (high Ach = higher threshold = focus)
    params.semantic_threshold = 0.1f + 0.3f * arousal_.acetylcholine;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// BUDGET ADAPTATION
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void CognitiveOS::adapt_budgets(const FieldMetrics& metrics, float cpu_load) {
    // Reset to defaults
    budgets_ = ServiceBudgets();
    
    // If low confidence, give more time to cognition
    if (intelligence_ && intelligence_->metrics().confidence < 0.4f) {
        budgets_.cognition += 2.0f;
    }
    
    // If high CPU load, reduce learning
    if (cpu_load > 0.85f) {
        budgets_.learning -= 1.0f;
    }
    
    // If high entropy, increase reflection
    if (metrics.entropy > 3.0f) {
        budgets_.reflection += 0.5f;
    }
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// SERVICE TICKS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void CognitiveOS::tick_cognition(float budget_ms) {
    // Check for queries
    auto events = bus_.poll(topics::COG_QUERY);
    
    for (const auto& event : events) {
        auto query = event.get<CogQuery>();
        if (!query || !intelligence_) continue;
        
        // Run reasoning
        auto result = intelligence_->reason(query->text);
        
        // Publish answer
        CogAnswer answer;
        answer.timestamp = get_timestamp();
        answer.text = result.answer;
        answer.reasoning_chain = result.reasoning_path;
        answer.confidence = result.confidence;
        
        bus_.publish(topics::COG_ANSWER, answer);
        
        // Activate result concepts in field
        for (const auto& [concept, score] : result.top_concepts) {
            // Would need to look up node_id from concept string
            // For now, just update field metrics
        }
    }
}

void CognitiveOS::tick_attention(float budget_ms) {
    (void)budget_ms;
    if (!field_ || !intelligence_) return;
    
    auto metrics = field_->get_metrics();
    auto& params = intelligence_->genome().reasoning_params();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ADAPTIVE BASELINE ACTIVITY (Controlled Noise Floor)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Update rolling average of activity
    float alpha = params.baseline_adaptation_rate;
    rolling_avg_activity_ = alpha * metrics.active_nodes + (1.0f - alpha) * rolling_avg_activity_;
    
    // Compute drive-modulated baseline target
    float curiosity = compute_curiosity_drive();
    float boredom = compute_boredom_drive();
    
    target_baseline_activity_ = 
        rolling_avg_activity_ * 0.8f +  // Track recent history
        params.baseline_activity_min +  // Minimum floor
        params.curiosity_baseline_scale * curiosity +  // Curiosity boost
        params.boredom_baseline_scale * boredom;  // Boredom boost
    
    // Clamp to genome-defined range
    target_baseline_activity_ = std::max(params.baseline_activity_min,
        std::min(params.baseline_activity_max, target_baseline_activity_));
    
    // If below target, inject contextual spontaneous activity
    if (metrics.active_nodes < target_baseline_activity_) {
        int nodes_needed = static_cast<int>(target_baseline_activity_ - metrics.active_nodes);
        
        // Sample contextual seeds (biased towards recent/self nodes)
        auto seeds = sample_contextual_seeds(nodes_needed);
        
        // Activate with curiosity-weighted energy
        float base_energy = params.baseline_power_budget * curiosity * 0.5f + 0.05f;
        for (int node_id : seeds) {
            field_->activate(node_id, base_energy, "baseline");
        }
    }
    
    // Boost working memory items (attention amplification)
    for (const auto& slot : working_memory_) {
        field_->activate(slot.node_id, 0.05f * slot.strength, "attention");
    }
    
    // Update recent node history for contextual seeding
    update_baseline_targets(metrics.active_nodes, metrics.entropy, 0.5f);
}

void CognitiveOS::tick_working_memory(float budget_ms) {
    // Decay existing slots
    for (auto& slot : working_memory_) {
        slot.strength *= 0.95f;
        slot.age++;
    }
    
    // Remove weak/old items
    working_memory_.erase(
        std::remove_if(working_memory_.begin(), working_memory_.end(),
            [](const WMSlot& s) { return s.strength < 0.1f || s.age > 100; }),
        working_memory_.end()
    );
    
    // Add highly active nodes
    auto active = field_->get_active(0.5f);
    for (int node_id : active) {
        float activation = field_->get_activation(node_id);
        
        // Check if already in WM
        bool found = false;
        for (auto& slot : working_memory_) {
            if (slot.node_id == node_id) {
                slot.strength = std::max(slot.strength, activation);
                slot.age = 0;
                found = true;
                break;
            }
        }
        
        // Add new item if room
        if (!found && working_memory_.size() < MAX_WM_SLOTS) {
            working_memory_.push_back({node_id, activation, 0});
        }
    }
    
    // Publish WM context
    WMContext wm;
    wm.timestamp = get_timestamp();
    for (const auto& slot : working_memory_) {
        wm.node_ids.push_back(slot.node_id);
        wm.strengths.push_back(slot.strength);
    }
    
    bus_.publish(topics::WM_CONTEXT, wm);
}

void CognitiveOS::tick_learning(float budget_ms) {
    if (!intelligence_) return;
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ONLINE LEARNING: Process feedback events and update system
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Check for motor feedback (success/failure from actions)
    auto motor_events = bus_.poll(topics::MOTOR_FEEDBACK);
    for (const auto& event : motor_events) {
        // If feedback event type exists, extract success/failure
        // For now, assume positive feedback
        intelligence_->learn(true);  // TODO: Extract actual success value from event
    }
    
    // Check for cognitive feedback (user corrections)
    auto cognitive_feedback = bus_.poll(topics::COG_FEEDBACK);
    for (const auto& event : cognitive_feedback) {
        // Would extract correct/incorrect from feedback event
        intelligence_->learn(true);  // TODO: Extract actual correctness from event
    }
    
    // Hebbian learning is automatically applied after each reasoning step
    // (see UnifiedIntelligence::reason() → apply_hebbian_learning())
}

void CognitiveOS::tick_reflection(float budget_ms) {
    if (!intelligence_) return;
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CONTINUOUS EVOLUTION: Self-improve when no prompt
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    static double last_evolution_time = get_timestamp();
    double now = get_timestamp();
    float dt = static_cast<float>(now - last_evolution_time);
    
    // Every reflection tick (~5Hz), evolve genome towards intelligence
    auto& genome = const_cast<evolution::DynamicGenome&>(intelligence_->genome());
    genome.evolve_towards_intelligence(dt);
    
    last_evolution_time = now;
    
    // Observe current metrics
    auto& metrics = intelligence_->metrics();
    auto mode = intelligence_->mode();
    
    // Publish reflection command if mode changed
    ReflectCommand cmd;
    cmd.timestamp = get_timestamp();
    cmd.mode = static_cast<int>(mode);
    cmd.beta = intelligence_->genome().reasoning_params().temperature;
    cmd.theta = intelligence_->genome().reasoning_params().semantic_threshold;
    cmd.strategy = "adaptive";
    
    bus_.publish(topics::REFLECT_COMMAND, cmd);
}

void CognitiveOS::tick_field_maintenance(float budget_ms) {
    // Global decay
    field_->decay(0.05f);
    
    // Normalize degrees (optional, expensive)
    if (total_ticks_ % 10 == 0) {
        field_->normalize_degrees();
    }
    
    // Apply k-WTA sparsity (keep top 1000 most active)
    if (field_->get_metrics().active_nodes > 1000) {
        field_->apply_kwta(1000);
        
        // Publish safety event
        SafetyEvent safety;
        safety.timestamp = get_timestamp();
        safety.event_type = "BACKPRESSURE";
        safety.severity = 0.7f;
        safety.details = "Too many active nodes, applied k-WTA";
        
        bus_.publish(topics::SAFETY_EVENTS, safety);
    }
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// HELPERS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

double CognitiveOS::get_timestamp() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

float CognitiveOS::estimate_cpu_load() const {
    // Simple estimate: if we're hitting our 20ms budget, we're at 100%
    // This is a placeholder - real implementation would query system
    return 0.5f;  // Assume 50% for now
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ADAPTIVE BASELINE HELPERS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void CognitiveOS::update_baseline_targets(int active_nodes, float entropy, float coherence) {
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // SELF-TUNING: Detect baseline failure and evolve parameters
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Compute baseline drift (error signal for evolution)
    baseline_drift_ = target_baseline_activity_ - static_cast<float>(active_nodes);
    
    // Detect persistent "death" (0 nodes)
    if (active_nodes == 0) {
        consecutive_dead_ticks_++;
    } else {
        consecutive_dead_ticks_ = 0;
    }
    
    // If system is dead for too long, trigger emergency evolution
    if (consecutive_dead_ticks_ > MAX_DEAD_TICKS) {
        evolve_baseline_parameters();
        consecutive_dead_ticks_ = 0;  // Reset democracy
    }
    
    // Continuous micro-evolution: if drift is large, gradually adjust
    if (std::abs(baseline_drift_) > 2.0f && intelligence_) {
        auto& genome = const_cast<evolution::DynamicGenome&>(intelligence_->genome());
        auto& params = genome.reasoning_params();
        
        // If actual is too low, increase baseline parameters
        if (baseline_drift_ > 0) {
            params.baseline_activity_min = std::min(8.0f, params.baseline_activity_min * 1.01f);
            params.baseline_activity_max = std::min(15.0f, params.baseline_activity_max * 1.01f);
            params.baseline_power_budget = std::min(0.10f, params.baseline_power_budget * 1.02f);
        }
        // If actual is too high (saturated), decrease
        else {
            params.baseline_activity_min = std::max(2.0f, params.baseline_activity_min * 0.99f);
            params.baseline_activity_max = std::max(5.0f, params.baseline_activity_max * 0.99f);
        }
    }
    
    // Update boredom: increases when low novelty
    if (entropy < 0.3f) {
        boredom_accumulator_ += 0.01f;
    } else {
        boredom_accumulator_ *= 0.95f;  // Decay when things are interesting
    }
    boredom_accumulator_ = std::min(1.0f, boredom_accumulator_);
    
    // Update prediction error (curiosity) based on coherence
    // Low coherence = high surprise = high curiosity
    recent_prediction_error_ = 0.9f * recent_prediction_error_ + 
                               0.1f * (1.0f - coherence);
}

void CognitiveOS::evolve_baseline_parameters() {
    if (!intelligence_) return;
    
    auto& genome = const_cast<evolution::DynamicGenome&>(intelligence_->genome());
    auto& params = genome.reasoning_params();
    
    std::cout << "🧬 EMERGENCY EVOLUTION: System detected death (0 nodes), self-tuning baseline...\n";
    
    // Aggressive parameter increase to revive system
    params.baseline_activity_min = std::min(8.0f, params.baseline_activity_min * 1.2f);
    params.baseline_activity_max = std::min(15.0f, params.baseline_activity_max * 1.2f);
    params.baseline_power_budget = std::min(0.10f, params.baseline_power_budget * 1.5f);
    params.baseline_adaptation_rate = std::min(0.1f, params.baseline_adaptation_rate * 1.3f);
    params.curiosity_baseline_scale = std::min(0.5f, params.curiosity_baseline_scale * 1.2f);
    
    // Reset target to be more aggressive
    target_baseline_activity_ = params.baseline_activity_max;
    rolling_avg_activity_ = params.baseline_activity_min;
    
    std::cout << "   ✅ Baseline params evolved: min=" << params.baseline_activity_min 
              << ", max=" << params.baseline_activity_max 
              << ", power=" << params.baseline_power_budget << "\n";
}

std::vector<int> CognitiveOS::sample_contextual_seeds(int k) {
    if (!intelligence_) return {};
    
    auto& params = intelligence_->genome().reasoning_params();
    std::vector<int> seeds;
    
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> node_dist(0, 24);  // Assuming 25 nodes in minimal graph
    
    // Network cycling: alternate focus areas
    double now = get_timestamp();
    if (now - last_dmn_switch_ > params.dmn_cycle_period) {
        // Cycle through DMN focuses
        switch (dmn_focus_) {
            case DMNFocus::INTROSPECTION:
                dmn_focus_ = DMNFocus::SALIENCE;
                break;
            case DMNFocus::SALIENCE:
                dmn_focus_ = DMNFocus::EXPLORATION;
                break;
            case DMNFocus::EXPLORATION:
                dmn_focus_ = DMNFocus::INTROSPECTION;
                break;
        }
        last_dmn_switch_ = now;
    }
    
    for (int i = 0; i < k; i++) {
        int node_id;
        
        if (dmn_focus_ == DMNFocus::INTROSPECTION) {
            // Introspection: bias towards self-related nodes (0-5)
            if (prob_dist(rng) < params.introspection_bias) {
                node_id = std::uniform_int_distribution<int>(0, 5)(rng);
            } else {
                node_id = node_dist(rng);
            }
        } else if (dmn_focus_ == DMNFocus::EXPLORATION) {
            // Exploration: novelty-weighted random selection
            // Bias towards nodes not recently active
            node_id = node_dist(rng);
            // TODO: Filter against recent_active_nodes_ for true novelty
        } else {
            // Salience: working memory or random
            if (!working_memory_.empty() && prob_dist(rng) < 0.5f) {
                auto& slot = working_memory_[std::uniform_int_distribution<size_t>(0, working_memory_.size()-1)(rng)];
                node_id = slot.node_id;
            } else {
                node_id = node_dist(rng);
            }
        }
        
        seeds.push_back(node_id);
    }
    
    return seeds;
}

float CognitiveOS::compute_curiosity_drive() const {
    if (!intelligence_) return 0.0f;
    
    auto& params = intelligence_->genome().reasoning_params();
    
    // Curiosity = prediction error + novelty seeking trait
    return params.novelty_exploration_weight * recent_prediction_error_;
}

float CognitiveOS::compute_boredom_drive() const {
    // Boredom accumulates when environment is repetitive
    return boredom_accumulator_;
}

} // namespace cognitive_os
} // namespace melvin

