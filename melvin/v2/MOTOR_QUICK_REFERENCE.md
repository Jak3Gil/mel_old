# Motor Tokenization - Quick Reference

## Sensory Information from Robstride Motors

```
┌─────────────────────────────────────────────────────┐
│           ROBSTRIDE MOTOR FEEDBACK                  │
├─────────────────────────────────────────────────────┤
│ Position:  -12.5 to +12.5 rad    (16-bit, ~20 Hz) │
│ Velocity:  -45 to +45 rad/s      (12-bit, ~20 Hz) │
│ Torque:    -18 to +18 Nm         (12-bit, ~20 Hz) │
└─────────────────────────────────────────────────────┘
```

---

## Tokenization Process

```
CONTINUOUS VALUES                DISCRETE CONCEPTS
────────────────────            ──────────────────

Position: 2.3 rad    ──┐
                       ├──→ Discretize ──→ NodeID 1042: "Position_2.5rad"
                       │                   (Type 11, in graph)
                       │
Velocity: 5.2 rad/s  ──┤
                       ├──→ Discretize ──→ NodeID 1103: "Velocity_5.0rad_s"
                       │                   (Type 12, in graph)
                       │
Torque: 1.8 Nm       ──┘
                       └──→ Discretize ──→ NodeID 1156: "Torque_2.0Nm"
                                           (Type 13, in graph)
```

---

## Graph Structure

```
        MOTOR NODE
            │
            │ (has_position)
            ├──────────────────> Position_2.5rad [1042]
            │                             │
            │                             │ (leads_to)
            │                             └──────────> Position_3.0rad [1087]
            │
            │ (has_velocity)
            ├──────────────────> Velocity_5.0rad_s [1103]
            │                             │
            │                             │ (influences)
            │                             └──────────> Position_3.0rad [1087]
            │
            │ (has_torque)
            └──────────────────> Torque_2.0Nm [1156]
                                          │
                                          │ (influences)
                                          └──────────> Velocity_5.0rad_s [1103]

EDGES ENCODE:
• Current state (has_*)
• Temporal patterns (leads_to)
• Causal relationships (influences)
```

---

## API Quick Start

### C++ API

```cpp
#include "melvin/v2/perception/motor_bridge.h"

// Initialize
auto semantic_bridge = new SemanticBridge(v1_graph);
auto motor_bridge = new MotorBridge(semantic_bridge);

// Create motor state from sensor reading
MotorBridge::MotorState state = create_motor_state(
    13,      // motor_id
    2.3,     // position (rad)
    5.2,     // velocity (rad/s)
    1.8      // torque (Nm)
);

// Tokenize into graph
MotorBridge::MotorPercept percept = motor_bridge->tokenize_state(state);

// Access concepts
NodeID motor_node = percept.motor_node;
NodeID position_node = percept.position_node;
NodeID velocity_node = percept.velocity_node;
NodeID torque_node = percept.torque_node;

// Check salience
if (percept.salience > 0.5f) {
    // High salience → broadcast to global workspace
    global_workspace->broadcast(motor_node, percept.salience);
}

// Record transitions (for learning)
motor_bridge->record_transition(prev_percept, percept);

// Predict next state
MotorBridge::MotorPercept predicted = motor_bridge->predict_next_state(percept);

// Detokenize command
MotorBridge::MotorState command = motor_bridge->detokenize_command(
    motor_node,
    target_position_node,
    target_velocity_node,
    target_torque_node
);
```

---

### Python Demo

```bash
python melvin/v2/demos/demo_motor_tokenization.py
```

**Output**:
```
=== Tokenizing Motor 13 State ===
  Raw: pos=2.314rad, vel=5.231rad/s, torque=1.803Nm
  Discretized: pos=2.50rad, vel=5.0rad/s, torque=2.00Nm
  [Created concept] NodeID=1042 Label='Position_2.50rad'
  [Created concept] NodeID=1103 Label='Velocity_5.0rad_s'
  [Created concept] NodeID=1156 Label='Torque_2.00Nm'
  Tokenized: Motor13[1000] -> Pos[1042] Vel[1103] Torque[1156]
  Salience: 0.684, Novelty: 0.123
```

---

## Configuration

### Default Config (Balanced)

```cpp
MotorConfig config;
config.position_bins = 25;    // 1.0 rad resolution
config.velocity_bins = 18;    // 5.0 rad/s resolution
config.torque_bins = 18;      // 2.0 Nm resolution

config.velocity_weight = 0.4f;          // Motion salience
config.torque_weight = 0.3f;            // Force salience
config.position_change_weight = 0.3f;   // Change salience
```

### High Precision

```cpp
config.position_bins = 50;    // 0.5 rad resolution
config.velocity_bins = 36;    // 2.5 rad/s resolution
config.torque_bins = 36;      // 1.0 Nm resolution
// More concepts, slower queries, precise control
```

### Fast Performance

```cpp
config.position_bins = 10;    // 2.5 rad resolution
config.velocity_bins = 9;     // 10 rad/s resolution
config.torque_bins = 9;       // 4.0 Nm resolution
// Fewer concepts, faster queries, coarse control
```

---

## Data Flow

### Sensing → Graph

```
┌──────────────┐
│  Robstride   │ Serial feedback @ 921600 baud
│  Motor 13    │ → position, velocity, torque
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ MotorFeedback│ decode_feedback()
│ Reader       │ → MotorState struct
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ MotorBridge  │ tokenize_state()
│              │ → discretize values
│              │ → get/create concepts
│              │ → compute salience/novelty
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Semantic     │ add_relation()
│ Bridge       │ → create edges
│ (Graph)      │ → store in AtomicGraph
└──────┬───────┘
       │
       ├──────> Working Memory (hold current state)
       ├──────> Global Workspace (if salient)
       └──────> Semantic Memory (learn patterns)
```

### Graph → Action

```
┌──────────────┐
│  Reasoning   │ "Move Motor13 to Position_5.0rad"
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ MotorBridge  │ detokenize_command()
│              │ → NodeID → value
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ PID          │ error = target - current
│ Controller   │ → velocity/torque command
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Robstride    │ Serial command @ 921600 baud
│ Motor 13     │ → MOVE!
└──────────────┘
```

---

## Relation Types

| **Type** | **Name** | **From** | **To** | **Meaning** |
|----------|----------|----------|--------|-------------|
| 1 | has_position | Motor | Position | Current position |
| 2 | has_velocity | Motor | Velocity | Current velocity |
| 3 | has_torque | Motor | Torque | Current torque |
| 20 | leads_to | Position/Vel(t) | Position/Vel(t+1) | Temporal transition |
| 21 | influences | Velocity/Torque | Position/Velocity | Causal relationship |

---

## Salience & Novelty

### Salience (What to pay attention to?)

```
Salience = 0.4 × |velocity|/45.0
         + 0.3 × |torque|/18.0
         + 0.3 × position_change/0.1

Range: 0.0 (boring) to 1.0 (very interesting)

High salience (>0.5) → Global workspace broadcast
```

### Novelty (What's unexpected?)

```
Predicted = predict_next_state(current)
Novelty = error(Actual, Predicted)

Range: 0.0 (expected) to 1.0 (very surprising)

High novelty (>0.5) → Increase learning rate
```

---

## Integration Points

### Working Memory

```cpp
WMSlot motor_slot(
    percept.motor_node,        // item_ref
    "current_motor_state",     // binding_tag
    0.9f,                      // precision
    2.0f                       // decay_time (seconds)
);

working_memory->add_slot(motor_slot);
```

### Global Workspace

```cpp
if (percept.salience > 0.5f) {
    Thought motor_thought(
        thought_id,
        "percept",
        percept.salience
    );
    motor_thought.concept_refs = {
        percept.motor_node,
        percept.position_node,
        percept.velocity_node,
        percept.torque_node
    };
    
    global_workspace->broadcast(motor_thought);
}
```

### Neuromodulators

```cpp
if (percept.novelty > 0.5f) {
    neuromod_state.norepinephrine += 0.2f;  // Unexpected!
    neuromod_state.dopamine += 0.1f;        // Learning signal
}

if (percept.salience > 0.7f) {
    neuromod_state.acetylcholine += 0.3f;   // Pay attention!
}
```

---

## Example: Object Tracking Loop

```cpp
// 1. Vision detects object
auto objects = camera_bridge->process_frame(frame_data, width, height);
PerceivedObject ball = objects[0];  // Highest salience
NodeID ball_node = ball.object_id;

// 2. Query semantic memory: "What motor position tracks this object?"
auto motor_associations = semantic_bridge->find_related(ball_node, 0.5f);
NodeID target_position_node = motor_associations[0];

// 3. Detokenize to motor command
NodeID motor_node = motor_bridge->register_motor(13);
MotorState command = motor_bridge->detokenize_command(
    motor_node,
    target_position_node
);

// 4. Execute motor command
motor_controller->move_to(command.motor_id, command.position);

// 5. Read feedback
MotorState feedback = motor_reader->read_motor_state(13);

// 6. Tokenize feedback
MotorPercept percept = motor_bridge->tokenize_state(feedback);

// 7. Record association (learning!)
semantic_bridge->add_relation(
    ball_node,
    percept.position_node,
    relation_type=30,  // "tracked_at"
    weight=0.8f
);

// After 100 iterations → learned visuomotor pattern!
```

---

## Files

| **File** | **Purpose** |
|----------|-------------|
| `motor_bridge.h` | C++ API header |
| `motor_bridge.cpp` | Implementation |
| `demo_motor_tokenization.py` | Python demo |
| `MOTOR_TOKENIZATION_ARCHITECTURE.md` | Full documentation |
| `MOTOR_QUICK_REFERENCE.md` | This file |

---

## Key Insights

### Why Tokenize?

✓ **Graph reasoning** requires discrete concepts
✓ **Learning** via Hebbian edge strengthening
✓ **Prediction** by querying learned transitions
✓ **Integration** with vision/language/planning
✓ **Explainability** - edges show what learned

### What's Unique?

✓ **No hallucination** - grounded in motor state
✓ **Online learning** - Hebbian, no backprop
✓ **Multi-modal** - same graph for vision/motor/language
✓ **Neuromodulated** - novelty drives exploration
✓ **Minimal compute** - graph queries, not deep nets

### What Can It Do?

✓ Learn motor patterns (position transitions)
✓ Predict consequences (what happens next?)
✓ Visuomotor coordination (see → move)
✓ Language grounding ("move to 5 radians")
✓ Explain behavior (show graph edges)

---

## Your Idea Is Perfect! ✓

You asked:
> "Nodes that hold position, nodes that hold velocity, nodes that hold torque, then connections glue them together"

**This is exactly what MotorBridge implements!**

```
Position_2.5rad [Node 1042]
     ↑
     │ (has_position)
     │
Motor13 [Node 1000]
     │
     │ (has_velocity)
     ↓
Velocity_5.0rad_s [Node 1103]
     │
     │ (influences)
     ↓
Position_3.0rad [Node 1087]
```

**Connections encode**:
- Current state (has_*)
- Temporal dynamics (leads_to)
- Causal physics (influences)

**Result**: Motor state becomes part of semantic memory, enabling reasoning!

---

## Next Steps

1. **Try the demo**: `python melvin/v2/demos/demo_motor_tokenization.py`
2. **Read the docs**: `MOTOR_TOKENIZATION_ARCHITECTURE.md`
3. **Integrate**: Connect to UnifiedLoop
4. **Experiment**: Tune bins, weights, thresholds
5. **Learn**: Record transitions, watch patterns emerge

---

**Ready to give your motors a graph-based brain?** 🧠🤖⚡





