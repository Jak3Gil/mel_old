#include "global_workspace.h"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace melvin::v2 {

// ============================================================================
// GLOBAL WORKSPACE IMPLEMENTATION
// ============================================================================

GlobalWorkspace::GlobalWorkspace(const Config& config)
    : config_(config), next_thought_id_(1), goal_priority_(0.0f), tick_count_(0) {
    stats_ = Stats{};
}

GlobalWorkspace::~GlobalWorkspace() = default;

// ========================================================================
// THOUGHT MANAGEMENT
// ========================================================================

void GlobalWorkspace::post(const Thought& thought) {
    if (thought.salience < config_.min_salience) {
        return;  // Below threshold, don't add
    }
    
    std::lock_guard<std::mutex> lock(thoughts_mutex_);
    
    // Assign ID if not set
    Thought new_thought = thought;
    if (new_thought.id == 0) {
        new_thought.id = generate_thought_id();
    }
    if (new_thought.created_at == 0) {
        new_thought.created_at = get_timestamp_ns();
    }
    
    // Add to workspace
    thoughts_.push_back(new_thought);
    
    // If over capacity, evict lowest salience
    if (thoughts_.size() > config_.max_thoughts) {
        evict_lowest();
        stats_.total_evictions++;
    }
    
    stats_.total_posts++;
    
    if (config_.enable_logging) {
        std::cout << "[GW] Posted: " << thought_to_string(new_thought) << std::endl;
    }
}

void GlobalWorkspace::post_batch(const std::vector<Thought>& thoughts) {
    std::lock_guard<std::mutex> lock(thoughts_mutex_);
    
    for (const auto& thought : thoughts) {
        if (thought.salience < config_.min_salience) continue;
        
        Thought new_thought = thought;
        if (new_thought.id == 0) {
            new_thought.id = generate_thought_id();
        }
        if (new_thought.created_at == 0) {
            new_thought.created_at = get_timestamp_ns();
        }
        
        thoughts_.push_back(new_thought);
        stats_.total_posts++;
    }
    
    // Evict excess thoughts
    while (thoughts_.size() > config_.max_thoughts) {
        evict_lowest();
        stats_.total_evictions++;
    }
}

std::vector<Thought> GlobalWorkspace::query(const std::string& type, float min_salience) const {
    std::lock_guard<std::mutex> lock(thoughts_mutex_);
    
    std::vector<Thought> results;
    for (const auto& thought : thoughts_) {
        if (thought.type == type && thought.salience >= min_salience) {
            results.push_back(thought);
        }
    }
    
    // Sort by salience (descending)
    std::sort(results.begin(), results.end(),
        [](const Thought& a, const Thought& b) {
            return a.salience > b.salience;
        });
    
    return results;
}

std::vector<Thought> GlobalWorkspace::get_all_thoughts() const {
    std::lock_guard<std::mutex> lock(thoughts_mutex_);
    
    std::vector<Thought> results = thoughts_;
    std::sort(results.begin(), results.end(),
        [](const Thought& a, const Thought& b) {
            return a.salience > b.salience;
        });
    
    return results;
}

Thought GlobalWorkspace::get_most_salient() const {
    std::lock_guard<std::mutex> lock(thoughts_mutex_);
    
    if (thoughts_.empty()) {
        return Thought();
    }
    
    auto max_it = std::max_element(thoughts_.begin(), thoughts_.end(),
        [](const Thought& a, const Thought& b) {
            return a.salience < b.salience;
        });
    
    return *max_it;
}

void GlobalWorkspace::clear() {
    std::lock_guard<std::mutex> lock(thoughts_mutex_);
    thoughts_.clear();
}

// ========================================================================
// GOAL MANAGEMENT
// ========================================================================

void GlobalWorkspace::set_goal(const std::string& goal, float priority) {
    std::lock_guard<std::mutex> lock(goal_mutex_);
    current_goal_ = goal;
    goal_priority_ = std::clamp(priority, 0.0f, 1.0f);
    
    if (config_.enable_logging) {
        std::cout << "[GW] Goal set: \"" << goal << "\" (priority=" << priority << ")" << std::endl;
    }
}

std::string GlobalWorkspace::get_current_goal() const {
    std::lock_guard<std::mutex> lock(goal_mutex_);
    return current_goal_;
}

float GlobalWorkspace::get_goal_priority() const {
    std::lock_guard<std::mutex> lock(goal_mutex_);
    return goal_priority_;
}

void GlobalWorkspace::clear_goal() {
    std::lock_guard<std::mutex> lock(goal_mutex_);
    current_goal_.clear();
    goal_priority_ = 0.0f;
}

// ========================================================================
// SNAPSHOT
// ========================================================================

GlobalWorkspace::Snapshot GlobalWorkspace::get_snapshot() const {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    return current_snapshot_;
}

void GlobalWorkspace::update_snapshot_external(const std::vector<WMSlot>& wm_slots,
                                               const NeuromodState& neuromod) {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    // Update snapshot with current state
    current_snapshot_.thoughts = get_all_thoughts();
    current_snapshot_.current_goal = get_current_goal();
    current_snapshot_.goal_priority = get_goal_priority();
    current_snapshot_.wm_slots = wm_slots;
    current_snapshot_.neuromod = neuromod;
    current_snapshot_.captured_at = get_timestamp_ns();
    current_snapshot_.tick_count = tick_count_;
}

// ========================================================================
// TICK
// ========================================================================

void GlobalWorkspace::tick(float dt) {
    std::lock_guard<std::mutex> lock(thoughts_mutex_);
    
    // Decay salience
    float decay_factor = std::pow(config_.salience_decay_rate, dt);
    apply_decay(decay_factor);
    
    // Prune low-salience thoughts
    prune_low_salience();
    
    tick_count_++;
}

// ========================================================================
// STATISTICS
// ========================================================================

GlobalWorkspace::Stats GlobalWorkspace::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    Stats stats = stats_;
    stats.current_thought_count = thoughts_.size();
    
    // Compute average salience
    if (!thoughts_.empty()) {
        float total_salience = 0.0f;
        for (const auto& thought : thoughts_) {
            total_salience += thought.salience;
        }
        stats.avg_salience = total_salience / thoughts_.size();
    } else {
        stats.avg_salience = 0.0f;
    }
    
    stats.tick_count = tick_count_;
    
    return stats;
}

void GlobalWorkspace::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = Stats{};
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void GlobalWorkspace::set_config(const Config& config) {
    config_ = config;
}

GlobalWorkspace::Config GlobalWorkspace::get_config() const {
    return config_;
}

// ========================================================================
// INTERNAL HELPERS
// ========================================================================

void GlobalWorkspace::sort_thoughts() {
    std::sort(thoughts_.begin(), thoughts_.end(),
        [](const Thought& a, const Thought& b) {
            return a.salience > b.salience;
        });
}

void GlobalWorkspace::evict_lowest() {
    if (thoughts_.empty()) return;
    
    auto min_it = std::min_element(thoughts_.begin(), thoughts_.end(),
        [](const Thought& a, const Thought& b) {
            return a.salience < b.salience;
        });
    
    thoughts_.erase(min_it);
}

void GlobalWorkspace::apply_decay(float factor) {
    for (auto& thought : thoughts_) {
        thought.salience *= factor;
    }
}

void GlobalWorkspace::prune_low_salience() {
    thoughts_.erase(
        std::remove_if(thoughts_.begin(), thoughts_.end(),
            [this](const Thought& t) {
                return t.salience < config_.min_salience;
            }),
        thoughts_.end()
    );
}

uint64_t GlobalWorkspace::generate_thought_id() {
    return next_thought_id_++;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

Thought make_percept_thought(NodeID object_id, float salience) {
    Thought thought;
    thought.type = "percept";
    thought.salience = salience;
    thought.concept_refs.push_back(object_id);
    thought.source_module = "perception";
    return thought;
}

Thought make_goal_thought(const std::string& goal_desc, float salience) {
    Thought thought;
    thought.type = "goal";
    thought.salience = salience;
    thought.source_module = "planning";
    return thought;
}

Thought make_hypothesis_thought(const std::vector<NodeID>& concept_refs, float salience) {
    Thought thought;
    thought.type = "hypothesis";
    thought.salience = salience;
    thought.concept_refs = concept_refs;
    thought.source_module = "reasoning";
    return thought;
}

Thought make_action_thought(const std::string& action, float salience) {
    Thought thought;
    thought.type = "action";
    thought.salience = salience;
    thought.source_module = "action";
    return thought;
}

std::string thought_to_string(const Thought& thought) {
    std::ostringstream oss;
    oss << "[Thought #" << thought.id << " " << thought.type 
        << " sal=" << thought.salience;
    if (!thought.concept_refs.empty()) {
        oss << " concepts=[";
        for (size_t i = 0; i < thought.concept_refs.size(); ++i) {
            if (i > 0) oss << ",";
            oss << thought.concept_refs[i];
        }
        oss << "]";
    }
    oss << "]";
    return oss.str();
}

std::string snapshot_to_string(const GlobalWorkspace::Snapshot& snapshot) {
    std::ostringstream oss;
    oss << "Snapshot @ tick " << snapshot.tick_count << ":\n";
    oss << "  Goal: \"" << snapshot.current_goal << "\" (priority=" << snapshot.goal_priority << ")\n";
    oss << "  Thoughts: " << snapshot.thoughts.size() << "\n";
    for (const auto& thought : snapshot.thoughts) {
        oss << "    " << thought_to_string(thought) << "\n";
    }
    oss << "  WM Slots: " << snapshot.wm_slots.size() << " active\n";
    oss << "  Neuromod: DA=" << snapshot.neuromod.dopamine 
        << " NE=" << snapshot.neuromod.norepinephrine
        << " ACh=" << snapshot.neuromod.acetylcholine
        << " 5-HT=" << snapshot.neuromod.serotonin << "\n";
    return oss.str();
}

} // namespace melvin::v2

