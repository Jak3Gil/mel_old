# Graph Growth Tracker

**Monitor and visualize how Melvin's brain expands over time.**

---

## What This Test Tracks

This test monitors Melvin's knowledge graph growth during reasoning:

### 1. Brain Expansion
- Node count over time
- Edge count over time
- Growth rates (nodes/edges per cycle)

### 2. Connection Types
- EXACT connections (taught facts)
- LEAP connections (inferred shortcuts)
- Type distribution changes

### 3. Learning Patterns
- How fast the graph grows
- LEAP inference effectiveness
- Knowledge consolidation

### 4. Performance
- Time per cycle
- Scaling characteristics
- Resource usage

---

## How to Run

### Quick Test (100 cycles)
```bash
./tests/test_graph_growth 100
```

### Standard Growth Test (500 cycles)
```bash
make growth
```

### Extended Test (Custom)
```bash
./tests/test_graph_growth 1000 100 50 false
# Args: cycles sleep_ms snapshot_every verbose
```

### Verbose Mode (See Growth Live)
```bash
./tests/test_graph_growth 500 100 50 true
# Shows node/edge count after each cycle
```

---

## Configuration

### Command Line Arguments

```bash
./tests/test_graph_growth [cycles] [sleep_ms] [snapshot_every] [verbose]
```

**Parameters:**
- `cycles` - Total cycles to run (default: 500)
- `sleep_ms` - Milliseconds between cycles (default: 100)
- `snapshot_every` - Snapshot frequency (default: 50)
- `verbose` - Show per-cycle details (default: false)

**Examples:**
```bash
./tests/test_graph_growth 100              # Quick test
./tests/test_graph_growth 1000 50          # Fast (50ms sleep)
./tests/test_graph_growth 500 100 25       # Snapshot every 25
./tests/test_graph_growth 100 100 10 true  # Verbose mode
```

---

## Output Files

**1. data/graph_growth.csv**
```csv
cycle,nodes,edges,exact,leap,time_ms
0,41,42,42,0,0
1,41,42,42,0,0
10,42,45,42,3,5
...
```

**Columns:**
- `cycle` - Iteration number
- `nodes` - Total node count
- `edges` - Total edge count
- `exact` - EXACT connection count
- `leap` - LEAP connection count
- `time_ms` - Cycle time

**2. data/nodes_growth_snapshot.melvin** - Growth snapshot (nodes)

**3. data/edges_growth_snapshot.melvin** - Growth snapshot (edges)

**4. data/graph_growth.png** - Visualization (4 subplots)

---

## Analysis

### Run Analysis
```bash
python3 scripts/analyze_graph_growth.py
```

### Makefile Target
```bash
make growth        # Runs test + analysis
make growth CYCLES=1000  # Custom cycle count
```

---

## Graph Visualization

The analysis generates a 4-subplot graph:

### 1. Total Nodes & Edges
- Shows overall brain size over time
- Upward curves = brain is growing
- Flat lines = stable (no new knowledge)

### 2. Connection Types
- EXACT (taught facts) - solid line
- LEAP (inferred) - dashed line
- Shows balance between taught and inferred

### 3. Growth Rates
- Nodes per cycle (rolling average)
- Edges per cycle (rolling average)
- Shows acceleration or deceleration

### 4. LEAP Percentage
- LEAP as % of total edges
- Shows inference effectiveness
- Higher % = more automated learning

---

## Interpreting Results

### Good Signs ✅

**Brain Growing:**
- Node count increases
- Edge count increases faster (good!)
- LEAP connections appear

**LEAP Inference Working:**
- LEAP count > 0
- LEAP percentage 10-30% (healthy balance)
- LEAP grows over time

**Performance Stable:**
- Time per cycle consistent
- No degradation with more edges
- Scales well

**Growth Controlled:**
- No runaway expansion
- Growth rate stabilizes
- Converges to plateau

### Warning Signs ⚠️

**No Growth:**
- Counts stay flat
- May indicate:
  - Queries don't require new knowledge
  - LEAP inference not running
  - Graph already saturated

**Runaway Growth:**
- Counts increase exponentially
- May indicate:
  - LEAP creating too many connections
  - No deduplication
  - Feedback loop

**Performance Degradation:**
- Time increases significantly
- May indicate:
  - O(n²) algorithm somewhere
  - Memory thrashing
  - I/O bottleneck

---

## Use Cases

### 1. Validate LEAP Inference
```bash
# Before: Teach knowledge
./demos/teach_knowledge

# Run growth tracker
./tests/test_graph_growth 500

# Expect: LEAP connections appear
```

### 2. Measure Learning Speed
```bash
# Track how fast Melvin learns
make growth CYCLES=1000

# Check: Growth rates in analysis
```

### 3. Test Scalability
```bash
# Start with small graph
make growth CYCLES=10000

# Monitor: Performance stays stable
```

### 4. Debug Growth Issues
```bash
# Verbose mode shows details
./tests/test_graph_growth 100 100 10 true

# See: Node/edge count after each cycle
```

---

## Expected Patterns

### Scenario 1: Static Knowledge (Current)
```
Nodes: Flat line (no new concepts)
Edges: Flat line (no new connections)
EXACT: Constant (taught facts)
LEAP: Zero or very low (no inference)
```

**Interpretation:** Graph is stable, queries use existing knowledge.

### Scenario 2: Active LEAP Inference
```
Nodes: Slow growth (rare new concepts)
Edges: Steady growth (LEAP adds shortcuts)
EXACT: Constant (only teaching adds these)
LEAP: Growing (inference working!)
```

**Interpretation:** Healthy learning, LEAP creating useful connections.

### Scenario 3: Teaching + Reasoning
```
Nodes: Steady growth (new concepts)
Edges: Fast growth (new facts + inference)
EXACT: Growing (taught facts)
LEAP: Growing faster (inference accelerates)
```

**Interpretation:** Aggressive learning, both teaching and inference active.

---

## Validation Criteria

The test performs 4 automatic checks:

1. **Brain is growing** (nodes or edges increased)
2. **LEAP inference working** (LEAP count > 0)
3. **Performance stable** (time change < 10ms)
4. **Growth controlled** (no runaway expansion)

**Pass:** 4/4 checks → Healthy growth ✅  
**Warning:** 2-3/4 checks → Investigate patterns ⚠️  
**Fail:** < 2/4 checks → System issue ❌

---

## Troubleshooting

### No growth detected
```
Cause: Queries use existing knowledge only
Fix: Add queries requiring inference
Fix: Enable LEAP auto-creation
```

### No LEAP connections
```
Cause: LEAP inference not enabled
Check: demos/create_leaps executable
Fix: Run LEAP inference manually
```

### Runaway growth
```
Cause: LEAP creating too many connections
Fix: Tune LEAP threshold
Fix: Add deduplication
```

### Performance degrades
```
Cause: Graph too large for current algorithm
Check: O(n²) operations
Fix: Optimize lookup structures
Fix: Add indexing
```

---

## Advanced Usage

### Compare Growth Across Runs

```bash
# Run 1
./tests/test_graph_growth 500
mv data/graph_growth.csv data/growth_run1.csv

# Run 2 (after tuning)
./tests/test_graph_growth 500
mv data/graph_growth.csv data/growth_run2.csv

# Compare
diff data/growth_run1.csv data/growth_run2.csv
```

### Monitor Real-Time

```bash
# Terminal 1: Run test
./tests/test_graph_growth 10000 1000 100 true

# Terminal 2: Watch growth
watch -n 5 'tail -1 data/graph_growth.csv'
```

### Export for Analysis

```bash
# CSV can be opened in Excel, R, etc.
open data/graph_growth.csv

# Or analyze with pandas
python3 -c "import pandas as pd; df = pd.read_csv('data/graph_growth.csv'); print(df.describe())"
```

---

## Integration with Other Tests

### Workflow: Growth → Longrun
```bash
# Track initial growth
make growth CYCLES=500

# Then endurance test
make longrun CYCLES=1000

# Compare: Growth should plateau in longrun
```

### Workflow: Teach → Growth
```bash
# Teach knowledge
./demos/teach_knowledge

# Track growth during reasoning
make growth

# Expect: EXACT stays constant, LEAP may grow
```

### Workflow: LEAP → Growth
```bash
# Create LEAP connections
./demos/create_leaps

# Track impact on reasoning
make growth

# Expect: Edge count increases, LEAP non-zero
```

---

## Performance Benchmarks

### Typical Results (500 cycles)

**Duration:**
- With 100ms sleep: ~2 minutes
- With 500ms sleep: ~10 minutes
- No sleep: ~30 seconds

**Growth (with LEAP inference enabled):**
- Nodes: +0-5 per 500 cycles (rare)
- Edges: +10-50 per 500 cycles (LEAP shortcuts)
- LEAP: +10-50 connections

**Performance:**
- Time/cycle: < 5ms
- No degradation
- Scales linearly

---

## What This Proves

Running this test successfully demonstrates:

✅ **Graph tracking works** (counts accurate)  
✅ **LEAP inference can be monitored** (type breakdown)  
✅ **Growth is measurable** (rates calculated)  
✅ **Performance scales** (no degradation)  
✅ **Snapshots capture state** (verification possible)  

**Result:** Can visualize and monitor Melvin's brain expansion! 📈

---

*Last updated: October 13, 2025*

