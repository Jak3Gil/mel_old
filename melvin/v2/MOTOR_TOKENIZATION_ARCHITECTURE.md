# Motor Tokenization Architecture

## Overview

**Motor Tokenization** converts continuous motor sensory feedback (position, velocity, torque) into discrete graph concepts (NodeIDs) that can participate in Melvin's cognitive architecture.

This document explains how your Robstride motors integrate with Melvin's graph-based reasoning system.

---

## The Problem

Traditional motor control:
```
Sensor → PID Controller → Motor
(continuous values, no learning, no reasoning)
```

**Problem**: The robot can't THINK about motor state or learn patterns.

---

## The Solution: Tokenization

Graph-based motor cognition:
```
Sensor → MotorBridge → Graph Concepts → Reasoning → Motor Commands
(discrete concepts, learnable, reasonabl)
```

**Solution**: Motor state becomes concepts that can be:
- Stored in semantic memory
- Reasoned about
- Predicted
- Integrated with vision/language

---

## Robstride Motor Sensory Information

Your Robstride motors provide **3 continuous streams** per motor:

| **Stream** | **Range** | **Resolution** | **Update Rate** |
|------------|-----------|----------------|-----------------|
| **Position** | -12.5 to +12.5 rad | 16-bit | ~20 Hz |
| **Velocity** | -45 to +45 rad/s | 12-bit | ~20 Hz |
| **Torque** | -18 to +18 Nm | 12-bit | ~20 Hz |

**Source**: `read_motor_feedback.py` lines 75-95

This is **perfect** for tokenization because:
- ✓ Bounded ranges (easy to discretize)
- ✓ High resolution (rich information)
- ✓ Real-time feedback (enables closed-loop)
- ✓ Three complementary modalities (position, motion, force)

---

## Architecture

### 1. Discretization (Continuous → Discrete)

**Concept**: Divide continuous space into bins, each bin = one graph node.

```
Position Range: -12.5 to +12.5 rad (25 rad total)
Position Bins:  25 concepts (1.0 rad resolution)

Example:
  2.3 rad → Bin 14 → "Position_2.5rad" (NodeID 1042)
  2.7 rad → Bin 15 → "Position_3.0rad" (NodeID 1043)
```

**Configuration**:
```cpp
struct MotorConfig {
    int position_bins = 25;   // 25 rad / 25 bins = 1.0 rad/bin
    int velocity_bins = 18;   // 90 rad/s / 18 = 5.0 rad/s per bin
    int torque_bins = 18;     // 36 Nm / 18 = 2.0 Nm per bin
};
```

**Tradeoffs**:
- More bins = finer resolution, more concepts, larger graph
- Fewer bins = coarser, fewer concepts, smaller graph
- Start with 25/18/18, tune based on task

---

### 2. Concept Creation (Bins → NodeIDs)

Each discrete bin becomes a **semantic concept** with:
- **NodeID**: Unique identifier (uint16_t, 0-65535)
- **Label**: Human-readable string (e.g., "Position_2.5rad")
- **Type**: Concept category (10=motor, 11=position, 12=velocity, 13=torque)

```cpp
NodeID position_node = semantic_bridge->create_concept("Position_2.5rad", 11);
```

**Example Concepts**:
```
NodeID 1000: "Motor13"          (type=10, motor)
NodeID 1042: "Position_2.5rad"  (type=11, position)
NodeID 1103: "Velocity_5.0rad_s" (type=12, velocity)
NodeID 1156: "Torque_2.0Nm"     (type=13, torque)
```

---

### 3. Relation Creation (Connect with Edges)

Concepts connect via **weighted edges**:

```
Motor13[1000] --[has_position, w=1.0]--> Position_2.5rad[1042]
Motor13[1000] --[has_velocity, w=1.0]--> Velocity_5.0rad_s[1103]
Motor13[1000] --[has_torque, w=1.0]----> Torque_2.0Nm[1156]
```

**Relation Types**:
| **Type** | **Name** | **Meaning** |
|----------|----------|-------------|
| 1 | has_position | Motor currently at position |
| 2 | has_velocity | Motor moving at velocity |
| 3 | has_torque | Motor exerting torque |
| 20 | leads_to | Temporal transition (t → t+1) |
| 21 | influences | Causal relationship |

---

### 4. MotorPercept Structure

Tokenization produces a **MotorPercept**:

```cpp
struct MotorPercept {
    NodeID motor_node;        // Motor13[1000]
    NodeID position_node;     // Position_2.5rad[1042]
    NodeID velocity_node;     // Velocity_5.0rad_s[1103]
    NodeID torque_node;       // Torque_2.0Nm[1156]
    
    MotorState raw_state;     // Keep original for control
    float salience;           // How interesting? (0-1)
    float novelty;            // How unexpected? (0-1)
};
```

This can enter:
- **Working Memory**: Hold current motor state (WMSlot)
- **Global Workspace**: Broadcast to all modules (Thought)
- **Semantic Memory**: Learn patterns (edges)

---

## Information Flow

### Forward Path (Sensing → Graph)

```
┌──────────────┐
│ Robstride    │  position = 2.3 rad
│ Motor 13     │  velocity = 5.2 rad/s
│              │  torque = 1.8 Nm
└──────┬───────┘
       │ Serial feedback
       ▼
┌──────────────┐
│ Motor        │  decode_feedback()
│ Feedback     │  → dict with 3 values
│ Reader       │
└──────┬───────┘
       │ MotorState struct
       ▼
┌──────────────┐
│ Motor        │  tokenize_state()
│ Bridge       │  → discretize
│              │  → get_position_concept()
│              │  → get_velocity_concept()
│              │  → get_torque_concept()
└──────┬───────┘
       │ MotorPercept
       ▼
┌──────────────┐
│ Semantic     │  Motor13 --[has_position]--> Position_2.5rad
│ Bridge       │  Motor13 --[has_velocity]--> Velocity_5.0rad_s
│ (Graph)      │  Motor13 --[has_torque]----> Torque_2.0Nm
└──────┬───────┘
       │
       ├──────> Working Memory (current state)
       ├──────> Global Workspace (if salient)
       └──────> Semantic Memory (learn patterns)
```

### Reverse Path (Graph → Motor Commands)

```
┌──────────────┐
│ Reasoning    │  "Move Motor13 to Position_5.0rad"
│ / Planning   │  → Goal: Motor13 --[should_be_at]--> Position_5.0rad
└──────┬───────┘
       │ Desired MotorPercept
       ▼
┌──────────────┐
│ Motor        │  detokenize_command()
│ Bridge       │  → Position_5.0rad → 5.0 rad
│              │  → create MotorState
└──────┬───────┘
       │ MotorState (target)
       ▼
┌──────────────┐
│ Motor        │  PID control
│ Controller   │  error = target - current
│              │  → compute velocity/torque
└──────┬───────┘
       │ Motor command bytes
       ▼
┌──────────────┐
│ Robstride    │  Move!
│ Motor 13     │
└──────────────┘
```

---

## Pattern Learning

### Temporal Patterns (What Happens Next?)

As motor state evolves, MotorBridge records transitions:

```
Position_2.0rad[t] --[leads_to, w=0.5]--> Position_2.5rad[t+1]
Position_2.5rad[t] --[leads_to, w=0.5]--> Position_3.0rad[t+1]
```

**Learning Rule** (Hebbian):
```cpp
void record_transition(const MotorPercept& from_state, 
                       const MotorPercept& to_state) {
    // Strengthen edge if exists, create if doesn't
    semantic_bridge->add_relation(
        from_state.position_node, 
        to_state.position_node, 
        relation_type=20,  // "leads_to"
        weight=0.5f
    );
    
    semantic_bridge->strengthen_relation(
        from_state.position_node,
        to_state.position_node,
        increment=0.1f  // Hebbian boost
    );
}
```

After many transitions, graph learns:
- **Forward models**: Position(t) → Position(t+1)
- **Inverse models**: Desired(t+1) ← Action(t)

---

### Causal Patterns (What Causes What?)

```
Velocity_5.0rad_s[t] --[influences, w=0.3]--> Position_2.5rad[t+1]
Torque_2.0Nm[t] ------[influences, w=0.3]--> Velocity_5.0rad_s[t+1]
```

**Meaning**:
- High velocity → position changes
- High torque → velocity changes

This allows **predictive reasoning**:
> "If I apply Torque_10.0Nm, Velocity will increase, which will move Position forward."

---

### Integration with Vision

Motor patterns can connect to visual concepts:

```
Object_Ball[2531] --[causes_attention]--> Motor13[1000]
Motor13[1000] ----[moves_to]-----------> Position_5.0rad[1042]
Position_5.0rad[1042] --[aligns_with]--> Object_Ball[2531]
```

**Meaning**:
> "When I see a ball, I move Motor13 to Position_5.0rad to track it."

This is **visuomotor learning** emerging from graph patterns!

---

## Salience & Novelty

### Salience (How Interesting?)

Determines what enters **global workspace** (conscious processing).

```cpp
float compute_salience(const MotorState& state) {
    float velocity_salience = abs(state.velocity) / 45.0f;
    float torque_salience = abs(state.torque) / 18.0f;
    
    float pos_change = abs(state.position - prev_position);
    float position_change_salience = min(pos_change / 0.1f, 1.0f);
    
    return 0.4 * velocity_salience + 
           0.3 * torque_salience + 
           0.3 * position_change_salience;
}
```

**High salience** (>0.5) triggers:
- Broadcast to global workspace
- Top-down attention
- Working memory allocation

**Example**:
- Motor moving fast (high velocity) → high salience → "pay attention!"
- Motor exerting high torque (struggling?) → high salience → "something wrong?"

---

### Novelty (How Unexpected?)

Measures **prediction error** (surprise).

```cpp
float compute_novelty(const MotorPercept& percept) {
    // Predict next state using learned transitions
    MotorPercept predicted = predict_next_state(prev_percept);
    
    // Measure error
    float position_error = abs(percept.position - predicted.position);
    float velocity_error = abs(percept.velocity - predicted.velocity);
    
    // Normalize
    float position_novelty = min(position_error / 2.0f, 1.0f);
    float velocity_novelty = min(velocity_error / 10.0f, 1.0f);
    
    return (position_novelty + velocity_novelty) / 2.0f;
}
```

**High novelty** (>0.5) triggers:
- Increased learning rate (neuromodulators)
- Exploration (curiosity)
- Model update

**Example**:
- Expected: Position_2.0rad → Position_2.5rad
- Actual: Position_2.0rad → Position_1.0rad (motor stalled?)
- Novelty = 0.8 → "This is unexpected! Learn new pattern."

---

## Integration with Melvin v2

### Working Memory

Motor percepts occupy working memory slots:

```cpp
WMSlot slot(
    percept.motor_node,      // item_ref (NodeID)
    "current_motor_state",   // binding_tag
    0.9f,                    // precision (high = well-maintained)
    2.0f                     // decay_time (forget after 2 seconds)
);
```

**Capacity**: ~7 slots (human working memory)
**Selection**: Highest salience wins

---

### Global Workspace

High-salience percepts broadcast as thoughts:

```cpp
Thought motor_thought(
    thought_id,
    "percept",              // type
    percept.salience        // salience (0-1)
);
motor_thought.concept_refs = {
    percept.motor_node,
    percept.position_node,
    percept.velocity_node,
    percept.torque_node
};
```

**Broadcasting** allows:
- Vision module: "I see what motor is doing"
- Language module: "Motor13 is moving fast"
- Planning module: "Stop motor to avoid collision"

---

### Semantic Memory

Patterns stored in AtomicGraph (v1):
- **4.29 million edges** already loaded
- Motor concepts add new edges
- Hebbian learning strengthens frequently-used paths

**Query Examples**:
```cpp
// Find related motor states
auto neighbors = semantic_bridge->find_related(position_node, threshold=0.5f);

// Multi-hop reasoning
auto reachable = semantic_bridge->multi_hop_query(motor_node, max_hops=5);
```

---

### Neuromodulators

Motor novelty drives learning:

```cpp
if (percept.novelty > 0.5f) {
    neuromod_state.norepinephrine += 0.2f;  // Unexpected uncertainty
    neuromod_state.dopamine += 0.1f;        // Reward for learning
}
```

**Effects**:
- ↑ Norepinephrine → ↑ Exploration, ↑ Learning rate
- ↑ Dopamine → ↑ Plasticity, strengthen recent transitions
- ↑ Acetylcholine → ↑ Attention to motor state

---

## Example: Complete Loop

### Scenario: Motor Tracking Object

**Step 1**: Vision detects ball
```
Camera → CameraBridge → PerceivedObject(ball, x=320, y=240)
→ Tokenize → NodeID 2531: "Object_Ball_Center"
→ High salience → Global workspace
```

**Step 2**: Reasoning links object to motor goal
```
Semantic query: "What actions relate to Object_Ball?"
→ Find edge: Object_Ball --[requires]--> Motor13_Track
→ Retrieve pattern: Motor13_Track --[means]--> Position_5.0rad
→ Create goal: Motor13 --[should_be_at]--> Position_5.0rad
```

**Step 3**: Detokenize goal to command
```
MotorBridge detokenize:
  Motor13 → motor_id=13
  Position_5.0rad → target=5.0 rad
→ MotorState(motor_id=13, position=5.0)
```

**Step 4**: Motor controller executes
```
PID control:
  error = 5.0 - current_position
  velocity_cmd = Kp * error
→ Send to Robstride motor
```

**Step 5**: Motor moves, feedback tokenized
```
MotorFeedbackReader → position=4.8, velocity=8.0, torque=1.2
→ MotorBridge tokenize → MotorPercept(...)
→ High salience (moving) → Global workspace
→ Record transition: Position_4.5rad --[leads_to]--> Position_4.8rad
```

**Step 6**: Pattern learning strengthens
```
After 100 repetitions:
  Object_Ball --[triggers]--> Motor13_Track (weight=0.95)
  Motor13_Track --[achieves]--> Position_5.0rad (weight=0.92)
```

**Result**: Motor tracking learned in semantic memory! 🎉

---

## Implementation Checklist

### Phase 1: Basic Tokenization ✓
- [x] MotorBridge header (`motor_bridge.h`)
- [x] MotorBridge implementation (`motor_bridge.cpp`)
- [x] Python demo (`demo_motor_tokenization.py`)
- [x] Documentation (this file)

### Phase 2: Integration
- [ ] Connect MotorBridge to UnifiedLoop
- [ ] Add motor percepts to GlobalWorkspace
- [ ] Enable working memory for motor state
- [ ] Test with real Robstride motors

### Phase 3: Learning
- [ ] Record transitions during motor movement
- [ ] Implement prediction (`predict_next_state()`)
- [ ] Test novelty detection on unexpected events
- [ ] Hebbian strengthening of motor patterns

### Phase 4: Visuomotor Integration
- [ ] Connect CameraBridge output to MotorBridge input
- [ ] Learn object → motor associations
- [ ] Implement tracking behavior
- [ ] Test with camera + motors simultaneously

### Phase 5: Language Integration
- [ ] Generate language from motor percepts
- [ ] Parse motor commands from language
- [ ] Test: "Move Motor13 to 5 radians"

---

## Configuration Guide

### Resolution vs Performance

**High Resolution** (more bins):
```cpp
MotorConfig high_res;
high_res.position_bins = 50;   // 0.5 rad resolution
high_res.velocity_bins = 36;   // 2.5 rad/s resolution
high_res.torque_bins = 36;     // 1.0 Nm resolution
```
- ✓ Precise control
- ✓ Rich patterns
- ✗ More concepts (slower graph queries)
- ✗ Harder to generalize

**Low Resolution** (fewer bins):
```cpp
MotorConfig low_res;
low_res.position_bins = 10;    // 2.5 rad resolution
low_res.velocity_bins = 9;     // 10 rad/s resolution
low_res.torque_bins = 9;       // 4 Nm resolution
```
- ✓ Fast graph queries
- ✓ Better generalization
- ✗ Coarse control
- ✗ Loss of detail

**Recommended Start**: Default (25/18/18) balances both.

---

### Salience Tuning

Control what gets attention:

```cpp
MotorConfig config;
config.velocity_weight = 0.5f;          // Pay attention to motion
config.torque_weight = 0.3f;            // Notice force
config.position_change_weight = 0.2f;   // Track movement
```

**Use Cases**:
- **Tracking task**: Increase `velocity_weight` (care about motion)
- **Force control**: Increase `torque_weight` (care about contact)
- **Positioning**: Increase `position_change_weight` (care about accuracy)

---

### Update Thresholds

Reduce noise:

```cpp
MotorConfig config;
config.min_position_change = 0.1f;   // Ignore jitter < 0.1 rad
config.min_velocity_change = 1.0f;   // Ignore vel noise < 1 rad/s
config.min_torque_change = 0.5f;     // Ignore torque noise < 0.5 Nm
```

**Effect**: Only create new percepts when change exceeds threshold.

---

## FAQ

### Q: Why tokenize instead of using raw values?

**A**: Graph reasoning requires discrete concepts. Continuous values can't participate in:
- Semantic memory (nodes/edges)
- Working memory (concept slots)
- Language generation ("Position_2.5rad" has meaning)
- Pattern recognition (edges encode transitions)

---

### Q: Don't we lose precision with discretization?

**A**: No! `MotorPercept` keeps `raw_state` with original values:

```cpp
struct MotorPercept {
    NodeID position_node;         // Discretized (for reasoning)
    MotorState raw_state;         // Original (for control)
    // ...
};
```

**Use discretized for**: reasoning, learning, language
**Use raw for**: PID control, feedback loops

---

### Q: How does this compare to deep learning motor control?

| **Aspect** | **MotorBridge (Graph)** | **Deep RL (Neural Net)** |
|------------|-------------------------|--------------------------|
| **Learning** | Hebbian (local, online) | Backprop (global, batch) |
| **Explainability** | ✓ Edges show patterns | ✗ Black box |
| **Sample Efficiency** | ✓ Few samples (Hebbian) | ✗ Many samples needed |
| **Transfer** | ✓ Share concepts | ✗ Retrain from scratch |
| **Memory** | ✓ Explicit (graph) | ✗ Implicit (weights) |
| **Integration** | ✓ Unified with vision/lang | ✗ Separate networks |

**Conclusion**: Graph is better for **reasoning**, neural nets better for **raw performance**. Melvin combines both!

---

### Q: Can multiple motors share concepts?

**A**: Yes! Position concepts are motor-agnostic:

```
Motor13 --[has_position]--> Position_2.5rad
Motor14 --[has_position]--> Position_2.5rad  (same node!)
```

**Benefit**: Generalization across motors.

**Example pattern**:
```
Position_2.5rad --[good_for]--> Tracking_Task
```
Applies to any motor at that position!

---

### Q: How fast is tokenization?

**A**: Very fast (<1ms):
- Discretization: 3 divisions + 3 lookups
- Concept cache hit: O(1) hash lookup
- Edge creation: O(1) graph insert

**Bottleneck**: Graph queries (multi-hop), but cached.

---

### Q: What happens if motor gets stuck?

**A**: High novelty triggers learning:

1. **Expected**: Position_2.0rad → Position_2.5rad
2. **Actual**: Position_2.0rad → Position_2.0rad (stuck!)
3. **Novelty**: 0.9 (very unexpected)
4. **Response**:
   - ↑ Norepinephrine (explore alternative)
   - ↑ Salience (broadcast to global workspace)
   - Reasoning: "Motor stuck, increase torque?"
   - Learn: Position_2.0rad + Torque_low → stuck

---

## Next Steps

### Try the Demo

```bash
python melvin/v2/demos/demo_motor_tokenization.py
```

Watch motor state get tokenized into graph concepts in real-time!

---

### Integrate with Your Code

```python
# In your motor control loop
from melvin.v2.perception import MotorBridge

# Create bridge
bridge = MotorBridge(semantic_bridge, config)

# Read motor
state = motor_reader.read_motor_state(motor_id=13)

# Tokenize
percept = bridge.tokenize_state(state)

# percept.motor_node, percept.position_node, etc. now in graph!
```

---

### Learn More

- `motor_bridge.h` - Full API reference
- `motor_bridge.cpp` - Implementation details
- `demo_motor_tokenization.py` - Working example
- `types_v2.h` - Core data structures

---

## Summary

**Motor Tokenization** bridges the gap between:
- **Physical motors** (continuous, reactive)
- **Graph-based cognition** (discrete, reasoned)

By converting motor state into graph concepts, we enable:
- ✓ Learning motor patterns
- ✓ Predicting motor consequences
- ✓ Reasoning about motor goals
- ✓ Integrating with vision/language
- ✓ Explainable motor behavior

**Your idea** of using nodes for position/velocity/torque with edges connecting them is **exactly right**! This architecture implements that vision.

---

**Ready to make your motors think?** 🧠🤖





