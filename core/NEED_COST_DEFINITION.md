# Need and Cost Definitions

## Current Implementation

### NEED Signal (Why this connection is valuable)

**Formula:**
```
need = 0.4 * information_value + 0.3 * utility + 0.3 * novelty
```

**Components:**

1. **Information Value** (40% weight)
   - Measures: How much does this connection improve predictions?
   - Currently: `edge.information_value` or falls back to `coactivations / 50.0`
   - Biological analog: Predictive coding value, reduces prediction error

2. **Utility** (30% weight)
   - Measures: How often is this connection actually used?
   - Currently: `min(1.0, coactivations / 100.0)`
   - Biological analog: Frequency of synaptic use (use-dependent strengthening)

3. **Novelty** (30% weight)
   - Measures: How recently was this connection used?
   - Currently: `exp(-time_since_use / 50.0)` (exponential decay)
   - Biological analog: Recency effects, working memory trace

**Problem:** These are somewhat arbitrary weights and heuristics.

---

### COST Signal (Why maintaining this connection is expensive)

**Formula:**
```
cost = energy_consumption + memory_cost + computation_cost
```

**Components:**

1. **Energy Consumption**
   - Measures: Metabolic cost of maintaining this connection
   - Currently: `edge.energy_consumption * edge.strength`
   - Base: `0.001f` per edge
   - Biological analog: Synaptic maintenance cost (ATP, protein synthesis)

2. **Memory Cost**
   - Measures: Storage overhead
   - Currently: Fixed `0.001f` per edge
   - Biological analog: Synaptic structure storage

3. **Computation Cost**
   - Measures: CPU/time to traverse this edge during activation spread
   - Currently: `0.001 * edge.strength`
   - Biological analog: Action potential propagation cost

**Problem:** Costs are mostly static. Real biological costs vary with:
- Connection age (older = more expensive?)
- Distance (long-range = more expensive)
- Myelination (efficiency varies)

---

## Biological Accuracy Issues

### Current Issues:

1. **Need components are abstract** - "information_value" isn't directly measurable
2. **Cost is too simple** - doesn't account for:
   - Structural costs (synaptic size, dendritic spines)
   - Metabolic efficiency (some synapses are more efficient)
   - Competition (limited resources, winner-take-all)
3. **No feedback from success** - need should increase when connection leads to successful predictions
4. **No local competition** - connections compete for limited resources at each node

---

## Proposed Improvements

### Better NEED Definition:

```cpp
float compute_need(int from_id, int to_id) const {
    // 1. PREDICTIVE VALUE: Does this connection reduce prediction error?
    float prediction_improvement = compute_prediction_error_reduction(from_id, to_id);
    
    // 2. FREQUENCY: How often do these nodes co-activate?
    float frequency = edge.coactivations / (current_time_ + 1.0f);
    
    // 3. SUCCESS RATE: How often does using this connection lead to correct answers?
    float success_rate = edge.successful_uses / (edge.total_uses + 1.0f);
    
    // 4. INFORMATION GAIN: How much information does this connection carry?
    // (Mutual information between source and target)
    float information_gain = compute_mutual_information(from_id, to_id);
    
    // Weighted combination
    return 0.3 * prediction_improvement + 
           0.2 * frequency + 
           0.3 * success_rate + 
           0.2 * information_gain;
}
```

### Better COST Definition:

```cpp
float compute_cost(int from_id, int to_id) const {
    const auto& edge = get_edge(from_id, to_id);
    
    // 1. METABOLIC COST: Energy to maintain synapse
    // Stronger connections = more proteins = more energy
    float metabolic_cost = edge.strength * BASE_METABOLIC_COST;
    
    // 2. STRUCTURAL COST: Synaptic size, dendritic real estate
    // Larger synapses consume more resources
    float structural_cost = edge.strength * STRUCTURAL_OVERHEAD;
    
    // 3. OPPORTUNITY COST: Energy that could be used elsewhere
    // High cost if system is near energy limit
    float opportunity_cost = (current_energy_used_ / total_energy_budget_) * OPPORTUNITY_FACTOR;
    
    // 4. DISTANCE COST: Longer connections = more expensive (if we track distances)
    // float distance_cost = compute_path_length(from_id, to_id) * DISTANCE_FACTOR;
    
    return metabolic_cost + structural_cost + opportunity_cost;
}
```

---

## Alternative: Direct Biological Metrics

Instead of abstract "information value", use measurable quantities:

### NEED = Measurable Benefits

1. **Prediction Accuracy**: % of times this connection correctly predicts next activation
2. **Reward Signal**: How often using this connection leads to positive feedback
3. **Mutual Information**: Statistical dependency between source and target
4. **Exploration Value**: How much this connection enables novel discoveries

### COST = Measurable Expenditures

1. **Energy Consumed**: Actual energy units spent maintaining/traversing
2. **Memory Footprint**: Bytes used to store edge
3. **Time Cost**: Milliseconds spent during traversal
4. **Resource Competition**: How much this edge blocks other potential connections

---

## Questions for Refinement

1. **Should need/cost be learned?** Instead of fixed formulas, let the system learn optimal weights through experience?

2. **Should costs be dynamic?** Energy cost might decrease as connection becomes more efficient (myelination analog)?

3. **Should we track success/failure explicitly?** Add counters for when connections lead to correct vs incorrect answers?

4. **Should we use mutual information?** Measure statistical dependency between nodes (more biologically grounded)?

5. **Should costs compete locally?** Each node has limited "connection budget" - connections compete for space?

What would you like to change or improve in these definitions?

