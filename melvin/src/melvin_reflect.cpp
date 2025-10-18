#include "melvin_reflect.h"
#include <cmath>
#include <iostream>

namespace melvin {

ReflectionEngine::ReflectionEngine(AtomicGraph& graph)
    : graph_(graph)
    , total_maintenance_time_(0)
{}

float ReflectionEngine::prediction_error(uint64_t node_id) {
    // Check if we have a prediction for this node
    auto pred_it = predictions_.find(node_id);
    if (pred_it == predictions_.end()) {
        // No prediction = high novelty
        return constants::CURIOSITY_MAX_BOOST;
    }
    
    // Check actual activation
    auto actual_it = actual_.find(node_id);
    if (actual_it == actual_.end()) {
        // Predicted but not observed = moderate error
        return constants::CURIOSITY_MIN_BOOST + 0.1f;
    }
    
    // Compute error
    float error = std::abs(pred_it->second - actual_it->second);
    return std::min(constants::CURIOSITY_CAP, error);
}

void ReflectionEngine::learn_from_error(uint64_t node_id, float err) {
    if (err < 0.1f) {
        return; // Error too small
    }
    
    // Reinforce connections that led to correct predictions
    auto neighbors = graph_.all_neighbors(node_id, 10);
    
    for (uint64_t n : neighbors) {
        // Strengthen connection
        graph_.reinforce(n, node_id, Rel::EXPECTS, constants::REINFORCE_ETA);
    }
    
    // Maybe form LEAP if error is significant
    if (err > constants::LEAP_ERROR_THRESHOLD) {
        // Find potential shortcut
        for (uint64_t n1 : neighbors) {
            auto n2_neighbors = graph_.all_neighbors(n1, 5);
            for (uint64_t n2 : n2_neighbors) {
                if (n2 != node_id) {
                    graph_.maybe_form_leap(n1, n2, err);
                }
            }
        }
    }
}

void ReflectionEngine::maintenance(float seconds_elapsed) {
    total_maintenance_time_ += seconds_elapsed;
    
    // Decay edges
    graph_.decay_edges(seconds_elapsed);
    
    // Clear old predictions
    if (total_maintenance_time_ > 10.0f) {
        predictions_.clear();
        actual_.clear();
        total_maintenance_time_ = 0;
    }
}

void ReflectionEngine::record_prediction(uint64_t node_id, float expected_activation) {
    predictions_[node_id] = expected_activation;
}

} // namespace melvin


