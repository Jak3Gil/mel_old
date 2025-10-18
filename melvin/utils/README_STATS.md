# Melvin Stats - Quick Brain Snapshot

**Lightweight utility for instant brain state inspection.**

---

## What It Does

Shows complete brain statistics in seconds:

- Total nodes and connections
- EXACT vs LEAP breakdown
- Runtime state (activations, weights)
- Storage efficiency
- Disk footprint

**No reasoning, no testing - just a pure snapshot!**

---

## Usage

### Quick Check
```bash
make stats
```

### Direct Execution
```bash
./utils/melvin_stats
```

### Custom Paths
```bash
./utils/melvin_stats path/to/nodes.melvin path/to/edges.melvin
```

---

## Output

```
╔═══════════════════════════════════════════════════════╗
║  MELVIN BRAIN SNAPSHOT                                ║
╚═══════════════════════════════════════════════════════╝

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  BRAIN STATISTICS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🧠 Total Nodes:                41
🔗 Total Connections:          42

Connection Types:
  EXACT (taught):              42 (100.0%)
  LEAP (inferred):              0 (0.0%)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  RUNTIME STATE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

⚡ Activations:
  Active nodes:                 0 / 41 (0.0%)
  Total activations:            0
  Active edges:                 0 / 42 (0.0%)
  Total coactivations:          0

⚖️  Weights:
  Avg node weight:       0.000000
  Avg edge weight:       0.000000

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  STORAGE EFFICIENCY
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

💾 Disk Usage:
  Nodes file:               509 B
  Edges file:               344 B
  Total:                    853 B

📊 Per-Item:
  Bytes/node:                  12 bytes
  Bytes/edge:                   8 bytes

🎯 Efficiency:
  Utilization:                0.0% (active/total)
  Status:              Sparse (most nodes unused)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  SUMMARY
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Brain Size:          41 nodes, 42 connections
  Disk Footprint:      853 B
  Learning State:      0 active nodes, 0 total activations
  LEAP Inference:      0 inferred connections

╔═══════════════════════════════════════════════════════╗
║  ✅ SNAPSHOT COMPLETE                                 ║
╚═══════════════════════════════════════════════════════╝
```

---

## Information Provided

### Brain Statistics
- **Total Nodes** - Unique concepts in knowledge graph
- **Total Connections** - Relationships between nodes
- **EXACT** - Taught facts (direct from input)
- **LEAP** - Inferred connections (learned shortcuts)

### Runtime State
- **Active nodes** - Nodes with non-zero activation count
- **Total activations** - Sum of all activation events
- **Active edges** - Edges with non-zero coactivation count
- **Total coactivations** - Sum of all coactivation events
- **Weights** - Average adaptive weights

### Storage Efficiency
- **Disk usage** - Actual file sizes
- **Bytes per item** - Storage efficiency
- **Utilization** - Percentage of active nodes
- **Status** - Sparse/Moderate/Dense

---

## Use Cases

### 1. Quick Check
```bash
# Check brain state anytime
make stats
```

### 2. Before/After Comparison
```bash
# Before teaching
make stats > before.txt

# Teach knowledge
./demos/teach_knowledge

# After teaching
make stats > after.txt

# Compare
diff before.txt after.txt
```

### 3. Monitor Growth
```bash
# Check periodically
watch -n 5 'make stats'
```

### 4. CI/CD Integration
```bash
# Verify brain loaded correctly
make stats | grep "Total Nodes"
```

### 5. Debug Storage
```bash
# Check if persistence working
make stats
# Should show non-zero counts
```

---

## Interpreting Results

### Healthy Brain ✅
```
Total Nodes: 1000+
Total Connections: 2000+
EXACT: 40-60% (taught knowledge)
LEAP: 40-60% (inferred knowledge)
Active nodes: 10-50% (reasonable usage)
```

### Fresh Brain 🆕
```
Total Nodes: 0-100
Total Connections: 0-100
EXACT: 100% (only taught)
LEAP: 0% (no inference yet)
Active nodes: 0% (never used)
```

### Mature Brain 🧠
```
Total Nodes: 10,000+
Total Connections: 50,000+
EXACT: 20-30% (core facts)
LEAP: 70-80% (extensive inference)
Active nodes: 30-70% (well used)
```

---

## Performance

- **Load time:** < 100ms
- **Analysis:** Instant
- **Memory:** ~2-5 MB
- **CPU:** Minimal

**Ideal for frequent checks!**

---

## Comparison to Tests

| Tool | Duration | Purpose |
|------|----------|---------|
| `make stats` | < 1 second | Quick snapshot |
| `make test` | 5 seconds | System validation |
| `make growth` | 2-10 min | Growth tracking |
| `make longrun` | Hours | Endurance |

**Use stats for instant checks anytime!**

---

## Examples

### After Fresh Install
```
Total Nodes:        0
Total Connections:  0
```

### After Teaching
```
Total Nodes:        41
Total Connections:  42
EXACT: 42 (100%)
LEAP: 0 (0%)
```

### After LEAP Inference
```
Total Nodes:        41
Total Connections:  127
EXACT: 42 (33%)
LEAP: 85 (67%)
```

### After Extended Use
```
Total Nodes:        45
Total Connections:  150
Active nodes: 23 (51%)
Total activations: 1,234
```

---

## Integration

### In Scripts
```bash
#!/bin/bash
stats=$(./utils/melvin_stats)
nodes=$(echo "$stats" | grep "Total Nodes" | awk '{print $3}')
echo "Brain has $nodes nodes"
```

### In CI/CD
```yaml
- name: Check brain loaded
  run: |
    make stats
    make stats | grep -q "Total Nodes:.*[1-9]" || exit 1
```

### In Monitoring
```bash
# Cron job (every hour)
0 * * * * cd /path/to/melvin && make stats >> brain_history.log
```

---

## Troubleshooting

### "Failed to load brain"
```
Cause: Files don't exist
Fix: Run ./demos/teach_knowledge first
```

### Shows 0 nodes/edges
```
Cause: Empty brain or wrong path
Check: ls -lh data/*.melvin
```

### File sizes seem wrong
```
Cause: Corruption or partial write
Fix: Delete and rebuild brain
```

---

## Summary

**Quick brain snapshot utility:**

✅ Instant execution (< 1 second)  
✅ Complete statistics  
✅ EXACT vs LEAP breakdown  
✅ Runtime state info  
✅ Storage efficiency  
✅ No side effects (read-only)  

**Perfect for:**
- Quick checks during development
- Before/after comparisons
- CI/CD validation
- Monitoring scripts

---

*Last updated: October 13, 2025*

