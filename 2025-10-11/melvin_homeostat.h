/*
 * MELVIN LLM TRAINING HOMEOSTATIC MICRO-CONTROLLER
 * 
 * Continuous, cheap parameter adjustment based on metrics feedback.
 * Implements the exact homeostatic rules specified in the training plan.
 */

#pragma once

#include "melvin_guardrails.h"
#include <chrono>
#include <iostream>

namespace melvin {

// ==================== HOMEOSTATIC CONTROLLER ====================

class HomeostaticController {
private:
    // Control parameters
    struct ControlParams {
        float adjustment_rate = 0.001f;  // Base adjustment rate
        float entropy_adjustment = 0.2f;  // Entropy-based β adjustment
        float alpha_adjustment = 0.01f;   // α adjustment rate
        float delta_adjustment = 0.02f;   // δ adjustment rate
        float eta_adjustment = 0.001f;    // η adjustment rate
        float gamma_adjustment = 0.1f;    // γ adjustment rate
    } params_;
    
    // State tracking
    std::chrono::system_clock::time_point last_adjustment_;
    std::chrono::milliseconds adjustment_interval_{100};  // Adjust every 100ms
    int consecutive_adjustments_ = 0;
    static constexpr int MAX_CONSECUTIVE_ADJUSTMENTS = 10;
    
public:
    HomeostaticController() : last_adjustment_(std::chrono::system_clock::now()) {}
    
    // Main control loop - call this regularly
    void update(GenomeSnapshot& genome, const MetricSnapshot& metrics) {
        auto now = std::chrono::system_clock::now();
        
        // Only adjust if enough time has passed
        if (now - last_adjustment_ < adjustment_interval_) {
            return;
        }
        
        last_adjustment_ = now;
        
        // Apply homeostatic rules
        bool adjusted = false;
        
        // Rule 1: If entropy_mid > 1.55 ⇒ β += 0.2, α += 0.01 (trust memory, sharpen choice)
        if (metrics.entropy_mid > 1.55f) {
            genome.beta += params_.entropy_adjustment;
            genome.alpha += params_.alpha_adjustment;
            adjusted = true;
            
            std::cout << "🏠 Homeostat: entropy too high (" << metrics.entropy_mid 
                      << ") → β=" << genome.beta << ", α=" << genome.alpha << "\n";
        }
        
        // Rule 2: If entropy_mid < 0.65 ⇒ β -= 0.2, α -= 0.02 (loosen, explore semantics)
        if (metrics.entropy_mid < 0.65f) {
            genome.beta -= params_.entropy_adjustment;
            genome.alpha -= (params_.alpha_adjustment * 2.0f);  // 0.02 instead of 0.01
            adjusted = true;
            
            std::cout << "🏠 Homeostat: entropy too low (" << metrics.entropy_mid 
                      << ") → β=" << genome.beta << ", α=" << genome.alpha << "\n";
        }
        
        // Rule 3: If top2_margin_mid < 0.12 ⇒ δ += 0.02 (stronger n-gram bonus)
        if (metrics.top2_margin_mid < 0.12f) {
            genome.delta += params_.delta_adjustment;
            adjusted = true;
            
            std::cout << "🏠 Homeostat: margin too low (" << metrics.top2_margin_mid 
                      << ") → δ=" << genome.delta << "\n";
        }
        
        // Rule 4: If drift_long > 0.28 ⇒ η -= 0.001, γ += 0.1 (slow plasticity, lean on relations)
        if (metrics.drift_long > 0.28f) {
            genome.eta -= params_.eta_adjustment;
            genome.gamma += params_.gamma_adjustment;
            adjusted = true;
            
            std::cout << "🏠 Homeostat: drift too high (" << metrics.drift_long 
                      << ") → η=" << genome.eta << ", γ=" << genome.gamma << "\n";
        }
        
        // Clamp parameters to valid ranges
        genome.clamp();
        
        // Update consecutive adjustment counter
        if (adjusted) {
            consecutive_adjustments_++;
        } else {
            consecutive_adjustments_ = 0;
        }
        
        // Prevent over-adjustment
        if (consecutive_adjustments_ >= MAX_CONSECUTIVE_ADJUSTMENTS) {
            std::cout << "🏠 Homeostat: Too many consecutive adjustments, cooling down...\n";
            consecutive_adjustments_ = 0;
            adjustment_interval_ = std::chrono::milliseconds(1000);  // Slow down
        } else {
            adjustment_interval_ = std::chrono::milliseconds(100);  // Normal speed
        }
    }
    
    // Check if homeostatic adjustment is needed
    bool needs_adjustment(const MetricSnapshot& metrics) const {
        return metrics.entropy_mid > 1.55f || 
               metrics.entropy_mid < 0.65f || 
               metrics.top2_margin_mid < 0.12f || 
               metrics.drift_long > 0.28f;
    }
    
    // Get current control parameters
    const ControlParams& get_params() const { return params_; }
    
    // Update control parameters
    void update_params(const ControlParams& new_params) {
        params_ = new_params;
    }
    
    // Reset controller state
    void reset() {
        consecutive_adjustments_ = 0;
        adjustment_interval_ = std::chrono::milliseconds(100);
        last_adjustment_ = std::chrono::system_clock::now();
    }
    
    // Get adjustment history (for debugging)
    int get_consecutive_adjustments() const { return consecutive_adjustments_; }
    std::chrono::milliseconds get_adjustment_interval() const { return adjustment_interval_; }
    
    // Manual parameter adjustment (for testing)
    void manual_adjust(GenomeSnapshot& genome, const std::string& parameter, float delta) {
        if (parameter == "alpha") {
            genome.alpha += delta;
        } else if (parameter == "beta") {
            genome.beta += delta;
        } else if (parameter == "gamma") {
            genome.gamma += delta;
        } else if (parameter == "eta") {
            genome.eta += delta;
        } else if (parameter == "delta") {
            genome.delta += delta;
        } else if (parameter == "epsilon") {
            genome.epsilon += delta;
        }
        
        genome.clamp();
        
        std::cout << "🏠 Manual adjustment: " << parameter << " += " << delta 
                  << " → " << genome.get_summary() << "\n";
    }
    
    // Apply task-specific adjustments
    void apply_task_adjustment(GenomeSnapshot& genome, const std::string& task_type) {
        if (task_type == "active_learning") {
            // Increase η slightly (plasticity) during active tasks
            genome.eta += params_.eta_adjustment * 2.0f;
            std::cout << "🏠 Task adjustment: Active learning → η=" << genome.eta << "\n";
        } else if (task_type == "sleep_cycle") {
            // Reduce η during sleep cycles
            genome.eta -= params_.eta_adjustment;
            std::cout << "🏠 Task adjustment: Sleep cycle → η=" << genome.eta << "\n";
        } else if (task_type == "qa_mode") {
            // Optimize for question answering
            genome.alpha += params_.alpha_adjustment;  // Trust memory more
            genome.beta += params_.entropy_adjustment * 0.5f;  // Sharpen choices
            std::cout << "🏠 Task adjustment: QA mode → α=" << genome.alpha << ", β=" << genome.beta << "\n";
        } else if (task_type == "creative_mode") {
            // Optimize for creative sampling
            genome.alpha -= params_.alpha_adjustment;  // Explore more
            genome.beta -= params_.entropy_adjustment * 0.5f;  // Loosen choices
            std::cout << "🏠 Task adjustment: Creative mode → α=" << genome.alpha << ", β=" << genome.beta << "\n";
        }
        
        genome.clamp();
    }
    
    // Get recommended adjustment based on metrics
    struct AdjustmentRecommendation {
        std::string parameter;
        float delta;
        std::string reason;
    };
    
    std::vector<AdjustmentRecommendation> get_recommendations(const MetricSnapshot& metrics) const {
        std::vector<AdjustmentRecommendation> recommendations;
        
        if (metrics.entropy_mid > 1.55f) {
            recommendations.push_back({"beta", params_.entropy_adjustment, "entropy too high"});
            recommendations.push_back({"alpha", params_.alpha_adjustment, "trust memory more"});
        }
        
        if (metrics.entropy_mid < 0.65f) {
            recommendations.push_back({"beta", -params_.entropy_adjustment, "entropy too low"});
            recommendations.push_back({"alpha", -params_.alpha_adjustment * 2.0f, "explore more"});
        }
        
        if (metrics.top2_margin_mid < 0.12f) {
            recommendations.push_back({"delta", params_.delta_adjustment, "margin too low"});
        }
        
        if (metrics.drift_long > 0.28f) {
            recommendations.push_back({"eta", -params_.eta_adjustment, "slow plasticity"});
            recommendations.push_back({"gamma", params_.gamma_adjustment, "lean on relations"});
        }
        
        return recommendations;
    }
    
    // Status reporting
    void log_status() const {
        std::cout << "🏠 Homeostat Status:\n";
        std::cout << "   Consecutive adjustments: " << consecutive_adjustments_ << "\n";
        std::cout << "   Adjustment interval: " << adjustment_interval_.count() << "ms\n";
        std::cout << "   Params: α_adj=" << params_.alpha_adjustment 
                  << ", β_adj=" << params_.entropy_adjustment 
                  << ", δ_adj=" << params_.delta_adjustment << "\n";
    }
};

// ==================== TASK-SPECIFIC CONTROLLER ====================

class TaskSpecificController {
private:
    HomeostaticController base_controller_;
    std::string current_task_mode_ = "default";
    std::chrono::system_clock::time_point task_start_time_;
    
public:
    TaskSpecificController() : task_start_time_(std::chrono::system_clock::now()) {}
    
    // Switch task mode
    void switch_task_mode(const std::string& new_mode) {
        if (new_mode != current_task_mode_) {
            std::cout << "🔄 Switching task mode: " << current_task_mode_ << " → " << new_mode << "\n";
            current_task_mode_ = new_mode;
            task_start_time_ = std::chrono::system_clock::now();
        }
    }
    
    // Update with task-specific adjustments
    void update(GenomeSnapshot& genome, const MetricSnapshot& metrics) {
        // Apply base homeostatic adjustments
        base_controller_.update(genome, metrics);
        
        // Apply task-specific adjustments
        base_controller_.apply_task_adjustment(genome, current_task_mode_);
    }
    
    // Get current task mode
    const std::string& get_current_task_mode() const { return current_task_mode_; }
    
    // Get time in current task mode
    std::chrono::duration<double> get_task_duration() const {
        return std::chrono::system_clock::now() - task_start_time_;
    }
    
    // Check if mode switch is recommended based on performance
    std::string get_recommended_mode_switch(const MetricSnapshot& metrics) const {
        if (metrics.success_mid < 0.6f && current_task_mode_ == "creative_mode") {
            return "qa_mode";  // Switch to QA if creative mode is failing
        }
        
        if (metrics.success_mid > 0.9f && current_task_mode_ == "qa_mode") {
            return "creative_mode";  // Switch to creative if QA is too easy
        }
        
        if (metrics.drift_long > 0.3f) {
            return "sleep_cycle";  // Switch to sleep if drifting too much
        }
        
        return current_task_mode_;  // No change recommended
    }
};

} // namespace melvin
