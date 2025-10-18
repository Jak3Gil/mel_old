#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include <unordered_map>

namespace melvin {

/**
 * ReflectionEngine - Predictive Coding
 * 
 * Biological analog:
 * - Cerebellum: Forward models and predictions
 * - ACC (Anterior Cingulate Cortex): Error detection
 * - Dopamine system: Prediction error signals
 */
class ReflectionEngine {
public:
    explicit ReflectionEngine(AtomicGraph& graph);
    
    /**
     * Compute prediction error for node
     * Compares expected vs actual activation
     * 
     * @param node_id Node to evaluate
     * @return Prediction error magnitude [0,1]
     */
    float prediction_error(uint64_t node_id);
    
    /**
     * Learn from prediction error
     * Updates graph weights based on error
     * 
     * @param node_id Node with error
     * @param err Error magnitude
     */
    void learn_from_error(uint64_t node_id, float err);
    
    /**
     * Periodic maintenance (decay, consolidation)
     * @param seconds_elapsed Time since last maintenance
     */
    void maintenance(float seconds_elapsed);
    
    /**
     * Record expected activation (for later comparison)
     * @param node_id Node being predicted
     * @param expected_activation Predicted activation level
     */
    void record_prediction(uint64_t node_id, float expected_activation);
    
private:
    AtomicGraph& graph_;
    std::unordered_map<uint64_t, float> predictions_; // node_id -> expected activation
    std::unordered_map<uint64_t, float> actual_; // node_id -> observed activation
    float total_maintenance_time_;
};

} // namespace melvin


