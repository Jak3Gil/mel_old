#include "reflective_controller.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>

namespace melvin {
namespace metacognition {

// ============================================================================
// SelfQuery Implementation
// ============================================================================

std::unordered_map<ReflectiveTrigger::Type, std::vector<std::string>> SelfQuery::query_templates_ = {
    {ReflectiveTrigger::Type::HIGH_ERROR, {
        "Why am I making more prediction errors?",
        "What patterns am I missing?",
        "Which nodes are causing the most confusion?"
    }},
    {ReflectiveTrigger::Type::LOW_COHERENCE, {
        "Why is my thinking unstable?",
        "Are there conflicting activations?",
        "What is causing the field chaos?"
    }},
    {ReflectiveTrigger::Type::HIGH_NOVELTY, {
        "What is new about this situation?",
        "Have I seen anything like this before?",
        "How should I update my understanding?"
    }},
    {ReflectiveTrigger::Type::GOAL_FAILURE, {
        "Why did I fail to achieve the goal?",
        "What went wrong in my planning?",
        "How can I improve next time?"
    }},
    {ReflectiveTrigger::Type::UNEXPECTED_REWARD, {
        "Why did I get this unexpected reward?",
        "What did I do right that I didn't expect?",
        "How can I repeat this success?"
    }},
    {ReflectiveTrigger::Type::LEARNING_PLATEAU, {
        "Why am I not improving?",
        "Am I stuck in a local optimum?",
        "Should I explore more or exploit current knowledge?"
    }},
    {ReflectiveTrigger::Type::CONTEXT_SHIFT, {
        "What changed in my environment?",
        "How is this context different from before?",
        "Do I need to adapt my parameters?"
    }}
};

std::string SelfQuery::generate_query(const ReflectiveTrigger& trigger) {
    auto it = query_templates_.find(trigger.type);
    if (it != query_templates_.end() && !it->second.empty()) {
        // Pick first template for now (could randomize or pick based on context)
        return it->second[0];
    }
    return "What is happening?";
}

std::string SelfQuery::answer_query(
    const std::string& query,
    const fields::UnifiedActivationField& field,
    const feedback::FeedbackCoordinator& feedback) {
    
    std::ostringstream answer;
    
    // Simple keyword-based routing for now
    if (query.find("error") != std::string::npos || query.find("prediction") != std::string::npos) {
        answer << "Error analysis: ";
        auto stats = feedback.get_stats();
        answer << "Average prediction error is " << stats.avg_prediction_error;
        answer << ". Most active concepts may be interfering.";
    }
    else if (query.find("unstable") != std::string::npos || query.find("coherence") != std::string::npos) {
        answer << "Coherence analysis: ";
        float coherence = field.get_coherence();
        answer << "Current coherence is " << coherence;
        if (coherence < 0.5f) {
            answer << ". Field has too many competing activations.";
        }
    }
    else if (query.find("new") != std::string::npos || query.find("novel") != std::string::npos) {
        answer << "Novelty analysis: ";
        answer << "Detected unfamiliar patterns in recent input.";
        answer << " Consider creating new LEAP edges.";
    }
    else if (query.find("goal") != std::string::npos || query.find("fail") != std::string::npos) {
        answer << "Goal failure analysis: ";
        answer << "Path to goal was blocked or prediction was inaccurate.";
    }
    else {
        answer << "General observation: ";
        answer << "Current total energy: " << field.get_total_energy();
        answer << ", Active nodes: " << field.get_active_node_count();
    }
    
    return answer.str();
}

// ============================================================================
// ReflectiveController Implementation
// ============================================================================

ReflectiveController::ReflectiveController(
    fields::UnifiedActivationField& field,
    feedback::FeedbackCoordinator& feedback)
    : field_(field), feedback_(feedback),
      last_reflection_(std::chrono::high_resolution_clock::now()) {
}

void ReflectiveController::tick(float dt) {
    // Update tracking
    auto stats = feedback_.get_stats();
    recent_errors_.push_back(stats.avg_prediction_error);
    recent_coherence_.push_back(field_.get_coherence());
    recent_rewards_.push_back(stats.avg_sensory_reward);
    
    if (recent_errors_.size() > history_window_) recent_errors_.erase(recent_errors_.begin());
    if (recent_coherence_.size() > history_window_) recent_coherence_.erase(recent_coherence_.begin());
    if (recent_rewards_.size() > history_window_) recent_rewards_.erase(recent_rewards_.begin());
    
    // Check triggers
    auto triggers = check_triggers();
    
    // Spawn reflections (rate-limited)
    auto now = std::chrono::high_resolution_clock::now();
    float time_since_last = std::chrono::duration<float>(now - last_reflection_).count();
    
    if (time_since_last >= min_reflection_interval_ && !triggers.empty()) {
        // Process the most significant trigger
        auto max_trigger = std::max_element(triggers.begin(), triggers.end(),
            [](const ReflectiveTrigger& a, const ReflectiveTrigger& b) {
                return a.magnitude < b.magnitude;
            });
        
        spawn_reflection(*max_trigger);
        last_reflection_ = now;
    }
}

std::vector<ReflectiveTrigger> ReflectiveController::check_triggers() {
    std::vector<ReflectiveTrigger> triggers;
    
    if (should_trigger_error_reflection()) {
        float magnitude = compute_variance(recent_errors_);
        triggers.emplace_back(ReflectiveTrigger::Type::HIGH_ERROR, magnitude);
    }
    
    if (should_trigger_coherence_reflection()) {
        float magnitude = 1.0f - field_.get_coherence();
        triggers.emplace_back(ReflectiveTrigger::Type::LOW_COHERENCE, magnitude);
    }
    
    if (should_trigger_novelty_reflection()) {
        // Novelty detected through high surprise
        auto stats = feedback_.get_stats();
        if (stats.avg_prediction_error > novelty_threshold_) {
            triggers.emplace_back(ReflectiveTrigger::Type::HIGH_NOVELTY, stats.avg_prediction_error);
        }
    }
    
    if (should_trigger_plateau_reflection()) {
        triggers.emplace_back(ReflectiveTrigger::Type::LEARNING_PLATEAU, 0.5f);
    }
    
    return triggers;
}

bool ReflectiveController::should_trigger_error_reflection() {
    if (recent_errors_.size() < 10) return false;
    
    float mean = std::accumulate(recent_errors_.begin(), recent_errors_.end(), 0.0f) / recent_errors_.size();
    return mean > error_threshold_;
}

bool ReflectiveController::should_trigger_coherence_reflection() {
    if (recent_coherence_.size() < 10) return false;
    
    float mean = std::accumulate(recent_coherence_.begin(), recent_coherence_.end(), 0.0f) / recent_coherence_.size();
    return mean < coherence_threshold_;
}

bool ReflectiveController::should_trigger_novelty_reflection() {
    if (recent_errors_.size() < 10) return false;
    
    // Sudden spike in error indicates novelty
    float recent_mean = 0.0f, older_mean = 0.0f;
    size_t mid = recent_errors_.size() / 2;
    
    for (size_t i = mid; i < recent_errors_.size(); ++i) {
        recent_mean += recent_errors_[i];
    }
    for (size_t i = 0; i < mid; ++i) {
        older_mean += recent_errors_[i];
    }
    
    recent_mean /= (recent_errors_.size() - mid);
    older_mean /= mid;
    
    return (recent_mean > older_mean * 1.5f);  // 50% spike
}

bool ReflectiveController::should_trigger_plateau_reflection() {
    if (recent_rewards_.size() < history_window_) return false;
    
    // Check if rewards are not improving
    float trend = compute_trend(recent_rewards_);
    float variance = compute_variance(recent_rewards_);
    
    // Flat trend + low variance = plateau
    return (std::abs(trend) < 0.01f && variance < 0.1f);
}

float ReflectiveController::compute_trend(const std::vector<float>& values) {
    if (values.size() < 2) return 0.0f;
    
    // Simple linear regression slope
    float n = values.size();
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
    
    for (size_t i = 0; i < values.size(); ++i) {
        float x = i;
        float y = values[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }
    
    float slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    return slope;
}

float ReflectiveController::compute_variance(const std::vector<float>& values) {
    if (values.empty()) return 0.0f;
    
    float mean = std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
    float variance = 0.0f;
    
    for (float v : values) {
        float diff = v - mean;
        variance += diff * diff;
    }
    
    return variance / values.size();
}

ReflectionNode ReflectiveController::spawn_reflection(const ReflectiveTrigger& trigger) {
    int node_id = next_reflection_id_++;
    std::string query = SelfQuery::generate_query(trigger);
    
    ReflectionNode reflection(node_id, query);
    reflection.answer = SelfQuery::answer_query(query, field_, feedback_);
    
    // Set metrics
    auto stats = feedback_.get_stats();
    reflection.prediction_error = stats.avg_prediction_error;
    reflection.coherence = field_.get_coherence();
    reflection.novelty = trigger.magnitude;
    
    // Suggest genome changes based on trigger type
    switch (trigger.type) {
        case ReflectiveTrigger::Type::HIGH_ERROR:
            reflection.suggested_gene_changes["base_learning_rate"] = 0.05f;  // Increase learning
            reflection.suggested_gene_changes["exploration_rate"] = 0.05f;  // Explore more
            break;
            
        case ReflectiveTrigger::Type::LOW_COHERENCE:
            reflection.suggested_gene_changes["energy_decay_rate"] = 0.02f;  // Faster decay
            reflection.suggested_gene_changes["attention_softmax_temp"] = -0.1f;  // Sharper focus
            break;
            
        case ReflectiveTrigger::Type::HIGH_NOVELTY:
            reflection.suggested_gene_changes["exploration_rate"] = 0.1f;  // Explore novel space
            reflection.suggested_gene_changes["novelty_bonus_multiplier"] = 0.2f;  // Reward novelty
            break;
            
        case ReflectiveTrigger::Type::LEARNING_PLATEAU:
            reflection.suggested_gene_changes["exploration_rate"] = 0.1f;  // Break out of local optimum
            reflection.suggested_gene_changes["base_learning_rate"] = 0.02f;  // Try different learning rate
            break;
            
        default:
            break;
    }
    
    reflections_.push_back(reflection);
    
    // Inject reflection into field as a special node
    std::vector<float> reflection_embedding(128, 0.0f);  // TODO: Encode query semantically
    field_.inject_energy(node_id, 10.0f, reflection_embedding);
    
    return reflection;
}

void ReflectiveController::apply_reflections_to_genome() {
    auto& genome = feedback_.get_genome();
    
    // Aggregate suggestions from recent reflections
    std::unordered_map<std::string, float> aggregated_changes;
    
    for (const auto& reflection : reflections_) {
        for (const auto& [gene_name, change] : reflection.suggested_gene_changes) {
            aggregated_changes[gene_name] += change;
        }
    }
    
    // Apply aggregated changes
    for (const auto& [gene_name, total_change] : aggregated_changes) {
        float current_value = genome.get(gene_name);
        genome.set(gene_name, current_value + total_change * 0.1f);  // Apply 10% of suggestion
    }
}

std::vector<ReflectionNode> ReflectiveController::get_recent_reflections(size_t count) {
    std::vector<ReflectionNode> recent;
    
    size_t start = (reflections_.size() > count) ? (reflections_.size() - count) : 0;
    for (size_t i = start; i < reflections_.size(); ++i) {
        recent.push_back(reflections_[i]);
    }
    
    return recent;
}

ReflectiveController::Stats ReflectiveController::get_stats() const {
    Stats stats;
    stats.total_reflections = reflections_.size();
    
    // Compute avg impact
    float total_impact = 0.0f;
    for (const auto& reflection : reflections_) {
        total_impact += reflection.suggested_gene_changes.size();
    }
    stats.avg_reflection_impact = reflections_.empty() ? 0.0f : (total_impact / reflections_.size());
    
    // Count triggers
    // TODO: Track trigger types in reflections
    
    return stats;
}

} // namespace metacognition
} // namespace melvin

