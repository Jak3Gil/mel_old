# Long-Run Soak Test

**Endurance test for Melvin's brain - run for hours or days to prove production readiness.**

---

## What This Test Proves

This test validates Melvin's stability and reliability over extended operation:

### 1. Endurance ✅
- Runs thousands of reasoning cycles without crashes
- No memory leaks or resource exhaustion
- Stable operation over hours/days

### 2. Learning Stability ✅
- Weights accumulate predictably
- No runaway learning (metric drift)
- Convergence to stable state

### 3. Persistence Integrity ✅
- Snapshots show only localized changes
- No data corruption
- Changes confined to active regions

### 4. Performance Consistency ✅
- Reasoning time stays stable
- No performance degradation
- Memory footprint constant

---

## How to Run

### Quick Test (100 cycles)
```bash
cd melvin
./tests/test_longrun 100
```

### Short Endurance (1000 cycles, ~10 minutes)
```bash
make longrun
```

### Long Endurance (Custom)
```bash
./tests/test_longrun 10000 500 100
# Args: cycles sleep_ms snapshot_every
```

### Production Soak (24 hours)
```bash
./tests/test_longrun 172800 500 1000 &
# 172800 cycles @ 500ms = ~24 hours
# Save PID: echo $! > longrun.pid
```

---

## Configuration

### Command Line Arguments

```bash
./tests/test_longrun [cycles] [sleep_ms] [verify_every]
```

**Parameters:**
- `cycles` - Total cycles to run (default: 1000)
- `sleep_ms` - Milliseconds between queries (default: 500)
- `verify_every` - Snapshot frequency (default: 50)

**Examples:**
```bash
./tests/test_longrun 100          # Quick: 100 cycles
./tests/test_longrun 1000 100     # Fast: 100ms sleep
./tests/test_longrun 5000 1000 25 # Long: snapshot every 25 cycles
```

---

## Output Files

**1. data/longrun_log.csv**
```csv
cycle,timestamp,query,hops,energy,avg_node_weight,avg_edge_weight,active_nodes,active_edges,time_ms
0,"Mon Oct 13 19:06:04 2025","what is fire?",0,0,0,0,0,0,0
...
```

**Columns:**
- `cycle` - Iteration number
- `timestamp` - When query was run
- `query` - Query text
- `hops` - Reasoning depth
- `energy` - Final energy
- `avg_node_weight` - Average node weight
- `avg_edge_weight` - Average edge weight
- `active_nodes` - Count of nodes with non-zero weight
- `active_edges` - Count of edges with non-zero weight
- `time_ms` - Query time

**2. data/nodes_verify.melvin** - Verification snapshot (nodes)

**3. data/edges_verify.melvin** - Verification snapshot (edges)

**4. data/longrun_analysis.png** - Visualization (after analysis)

---

## Analysis

### Run Verification
```bash
python3 scripts/verify_snapshots.py
```

**Checks:**
- Compares main storage with verification snapshot
- Detects corruption
- Analyzes byte-level changes
- Ensures changes are localized

### Run Analysis
```bash
python3 scripts/analyze_longrun.py
```

**Generates:**
- 4-subplot graph:
  1. Energy trends over time
  2. Weight accumulation
  3. Active nodes/edges growth
  4. Performance (rolling average)
- Summary statistics
- Drift detection

### Makefile Target
```bash
make longrun
```

Runs: test → verify → analyze (all in one command)

---

## Interpreting Results

### Good Signs ✅

**Energy Trends:**
- Stable or slight upward trend
- No sudden spikes or drops
- Different queries have consistent patterns

**Weight Evolution:**
- Gradual accumulation
- Converges to stable range
- Active count increases then plateaus

**Performance:**
- Time stays consistent (± 10%)
- No degradation over cycles
- Rolling average is flat

**Snapshots:**
- Small byte changes (< 1% of file)
- Changes localized to runtime regions
- No size mismatches

### Warning Signs ⚠️

**Metric Drift:**
- Energy doubles or halves
- Weights grow unbounded
- Active count keeps increasing

**Performance Degradation:**
- Time increases > 50%
- Memory usage grows
- Cycles per second decreases

**Snapshot Issues:**
- Large byte changes (> 10%)
- Changes throughout file
- Size mismatches

---

## Use Cases

### 1. Pre-Release Testing
```bash
./tests/test_longrun 10000 500 100
# Run overnight before major release
```

### 2. Continuous Integration
```bash
./tests/test_longrun 100 100 10
# Quick smoke test (~30 seconds)
```

### 3. Production Monitoring
```bash
./tests/test_longrun 172800 500 1000 &
# 24-hour soak test
```

### 4. Stress Testing
```bash
./tests/test_longrun 100000 0 500
# 100K cycles, no sleep (max speed)
```

### 5. Stability Verification
```bash
./tests/test_longrun 50000 1000 100
# 50K cycles, 1s sleep (~14 hours)
```

---

## Expected Performance

### Typical Results (1000 cycles)

**Duration:**
- With 500ms sleep: ~10 minutes
- With 100ms sleep: ~2 minutes
- No sleep: ~30 seconds

**Memory:**
- Initial: ~2-5 MB
- Final: ~2-5 MB (no growth)

**CPU:**
- With sleep: < 5% average
- No sleep: 10-20% average

**Disk I/O:**
- ~1-2 KB per cycle (auto-save)
- ~1 MB total for 1000 cycles

---

## Troubleshooting

### Test crashes
```
Check: Memory limits (ulimit -m)
Check: Disk space (df -h)
Fix: Reduce cycle count or increase sleep
```

### Verification fails
```
Expected: Small changes in runtime regions
Fix: Review longrun_log.csv for anomalies
Fix: Check for external processes modifying files
```

### Analysis script errors
```
Install: pip3 install pandas matplotlib
Check: data/longrun_log.csv exists
Check: File has valid CSV format
```

### Performance degrades
```
Check: Other processes consuming resources
Check: Disk I/O (iotop)
Fix: Increase sleep_ms
Fix: Reduce auto-save frequency
```

---

## Advanced Usage

### Resume After Crash

The test doesn't auto-resume, but you can:

1. Check last cycle in CSV:
```bash
tail -1 data/longrun_log.csv | cut -d',' -f1
```

2. Start from there (manually edit test):
```cpp
int start_cycle = 500;  // Last completed
for (int cycle = start_cycle; cycle < total_cycles; ++cycle) {
```

### Parallel Testing

Run multiple instances:
```bash
./tests/test_longrun 1000 > test1.log 2>&1 &
./tests/test_longrun 1000 > test2.log 2>&1 &
```

(Use different data directories to avoid conflicts)

### Custom Queries

Edit `test_longrun.cpp`:
```cpp
std::vector<std::string> queries = {
    "your custom query 1",
    "your custom query 2",
};
```

### Snapshot Comparison

Compare snapshots over time:
```bash
diff -u <(xxd data/nodes.melvin) <(xxd data/nodes_verify.melvin) | head -50
```

---

## Validation Criteria

The test is **successful** if:

1. **Completes without crashes** ✅
2. **Energy drift < 20%** (first 100 vs last 100) ✅
3. **Time drift < 50ms** (first 100 vs last 100) ✅
4. **Snapshot changes < 5%** of file size ✅
5. **Memory stable** (no continuous growth) ✅

**Pass:** All 5 criteria met  
**Warning:** 4/5 criteria met  
**Fail:** < 4 criteria met

---

## What This Proves

Running this test successfully for 1000+ cycles demonstrates:

✅ **Endurance** - No crashes over extended operation  
✅ **Stability** - Metrics don't drift or explode  
✅ **Integrity** - Persistence works correctly  
✅ **Performance** - Speed stays consistent  
✅ **Scalability** - Handles high volume  

**Result:** Melvin's brain is production-ready for 24/7 operation! 🚀

---

## Comparison to Short Test

| Metric | Short Test (25 samples) | Long-Run (1000 cycles) |
|--------|-------------------------|------------------------|
| Duration | 5 seconds | 10 minutes |
| Purpose | System integration | Endurance & stability |
| Cycles | 5 | 1000 |
| Snapshots | No | Yes (every 50) |
| Analysis | Validation checks | Drift detection |
| Use Case | Development | Pre-release |

**Both tests are complementary:**
- Short test: Quick validation during development
- Long-run: Thorough validation before production

---

*Last updated: October 13, 2025*

