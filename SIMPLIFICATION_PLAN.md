# Code Simplification Plan: Biological Emergence

## Philosophy: Change = Need - Cost

Instead of hardcoding where every neuron should go, we let connections emerge from actual usage patterns, constrained by biological costs.

## Current Problems

### Over-Engineering
- Multiple activation fields (3+ different implementations)
- Multiple reflection controllers (3 duplicates)
- Complex cognitive engine with 50+ hardcoded mechanisms
- Hardcoded reasoning modes, strategies, intents
- Pre-designed graph structures
- Fixed parameters that should adapt

### Biological Principles Missing
- **Need-based connection formation**: Connections should only form when information value > maintenance cost
- **Cost-driven pruning**: Unused connections should weaken and disappear
- **Energy constraints**: System should have limited energy budget
- **Self-organization**: Structure emerges from usage, not design

## New Architecture

### Single Core: `emergent_graph.h` + `emergent_intelligence.h`

Replaces:
- `unified_intelligence.cpp` (simplified)
- `cognitive_engine.cpp` (removed - mechanisms emerge)
- `unified_reasoning_engine.cpp` (simplified)
- `reflection_controller_dynamic.cpp` (removed - reflection emerges)
- `spreading_activation.cpp` (simplified - just spreading)
- Multiple field implementations (unified)

### Key Principles

1. **Connection Formation**: Edge A→B forms when nodes A and B co-activate AND need > cost
2. **Connection Strengthening**: Edge weight increases proportionally to (need - cost)
3. **Connection Weakening**: Edge weight decreases when cost > need
4. **Connection Pruning**: Edge removed when strength < threshold
5. **Node Creation**: New node created when novel concept encountered and energy available

### Need Signal Components
- **Information Value**: How much does this connection improve predictions?
- **Utility**: How useful is this information?
- **Novelty**: Is this new/interesting?
- **Frequency**: How often do these nodes co-activate?

### Cost Signal Components
- **Energy Consumption**: Metabolic cost of maintaining connection
- **Memory Cost**: Storage overhead
- **Computation Cost**: CPU/time to traverse edge
- **Opportunity Cost**: Energy that could be used elsewhere

## Migration Plan

### Phase 1: Create Emergent Core
- [x] `emergent_graph.h` - Core need-cost graph
- [ ] `emergent_graph.cpp` - Implementation
- [x] `emergent_intelligence.h` - Simplified intelligence
- [ ] `emergent_intelligence.cpp` - Implementation

### Phase 2: Simplify Unified Intelligence
- [ ] Replace `unified_intelligence.cpp` with calls to `emergent_intelligence`
- [ ] Remove hardcoded strategies and modes
- [ ] Let reasoning emerge from graph structure

### Phase 3: Remove Duplicates
- [ ] Remove duplicate activation fields
- [ ] Remove duplicate reflection controllers  
- [ ] Remove cognitive_engine.cpp (keep only what's actually used)
- [ ] Consolidate reasoning components

### Phase 4: Simplify Cognitive OS
- [ ] Remove hardcoded service budgets (let them emerge)
- [ ] Remove fixed schedules (adapt to need)
- [ ] Simplify to just: graph + energy management

## Components to Keep
- `graph_storage.h` (interface only)
- `emergent_graph.h` (new core)
- `emergent_intelligence.h` (new core)
- `cognitive_os.h` (simplified)
- Crossmodal binding (if used)

## Components to Remove/Simplify
- `cognitive_engine.cpp` - Too complex, mechanisms should emerge
- `unified_reasoning_engine.cpp` - Merge into emergent_intelligence
- `reflection_controller_dynamic.cpp` - Reflection emerges from need-cost
- Multiple activation field implementations
- Hardcoded intent classification (can emerge from usage)
- Complex genome system (simplify to just key parameters)

## Benefits
1. **Simpler codebase**: One core graph system instead of 10+ components
2. **True adaptation**: Structure changes based on actual usage
3. **Biological realism**: Follows actual neural principles
4. **Easier to reason about**: Need - cost is intuitive
5. **Self-organizing**: No manual tuning required
6. **Scalable**: Grows/shrinks based on constraints

## Example: Connection Formation

```
// Old way (hardcoded):
graph[word_a].push_back({word_b, 0.5f});  // We decide connection exists

// New way (emergent):
// Node A and B co-activate:
activate({A, B});

// After spreading:
update_connections();
// Edge A→B forms automatically if:
//   need = information_value + utility > cost = energy + memory
// Edge weight = strength proportional to (need - cost)
```

## Example: Answer Generation

```
// Old way (complex pipeline):
1. Classify intent
2. Select strategy based on intent
3. Activate nodes
4. Spread with strategy-specific parameters
5. Score with genome weights
6. Generate with templates

// New way (emergent):
1. Activate query nodes
2. Spread activation (connections emerge naturally)
3. Answer = most active nodes (no templates, no modes)
4. If wrong → adjust need/cost for connections used
```

## Next Steps
1. Implement `emergent_graph.cpp`
2. Implement `emergent_intelligence.cpp`
3. Test with simple queries
4. Migrate existing graph data
5. Gradually remove old components

