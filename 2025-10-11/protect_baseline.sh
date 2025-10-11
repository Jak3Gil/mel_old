#!/bin/bash
# Protect Control Baseline
# Ensures the original 0.0 baseline is never deleted or overwritten
# This is your yardstick for all future progress measurement
#
# Usage: ./protect_baseline.sh

set -e

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║              🔒 PROTECT CONTROL BASELINE                      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

HISTORY_FILE="diagnostics_history.csv"
BASELINE_DIR="baseline_control"

if [ ! -f "$HISTORY_FILE" ]; then
    echo "❌ Error: $HISTORY_FILE not found"
    exit 1
fi

# Create protected baseline directory
mkdir -p "$BASELINE_DIR"

echo "📋 Protecting baseline (first diagnostic run)..."
echo ""

# Extract first non-header line (the original 0.0 baseline)
BASELINE_ENTRY=$(head -2 "$HISTORY_FILE" | tail -1)

if [ -z "$BASELINE_ENTRY" ]; then
    echo "⚠️  No baseline entry found in history"
    exit 1
fi

# Parse baseline metrics
BASELINE_DATE=$(echo "$BASELINE_ENTRY" | cut -d',' -f1)
BASELINE_SIM=$(echo "$BASELINE_ENTRY" | cut -d',' -f5)
BASELINE_ENTROPY=$(echo "$BASELINE_ENTRY" | cut -d',' -f4)
BASELINE_SUCCESS=$(echo "$BASELINE_ENTRY" | cut -d',' -f6)

echo "📊 Baseline Metrics ($(echo $BASELINE_DATE)):"
echo "   Context Similarity:  $BASELINE_SIM"
echo "   Entropy Reduction:   $BASELINE_ENTROPY"
echo "   Leap Success:        $(echo "$BASELINE_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%"
echo ""

# Save baseline data
echo "$BASELINE_ENTRY" > "$BASELINE_DIR/baseline_entry.csv"
head -1 "$HISTORY_FILE" > "$BASELINE_DIR/baseline_history.csv"
echo "$BASELINE_ENTRY" >> "$BASELINE_DIR/baseline_history.csv"

# Copy any existing baseline diagnostic files
[ -f leap_diagnostics.csv ] && cp leap_diagnostics.csv "$BASELINE_DIR/baseline_diagnostics.csv" 2>/dev/null
[ -f leap_tuning_report.md ] && cp leap_tuning_report.md "$BASELINE_DIR/baseline_report.md" 2>/dev/null

# Create README explaining the baseline
cat > "$BASELINE_DIR/README.md" << 'BASELINEEOF'
# 🔒 Control Baseline - DO NOT DELETE

## Purpose

This directory contains the **original baseline** from when the diagnostic system was first run.

**These files are your control group.** All future progress is measured against these values.

## Baseline Metrics

BASELINEEOF

echo "- **Date:** $BASELINE_DATE" >> "$BASELINE_DIR/README.md"
echo "- **Context Similarity:** $BASELINE_SIM" >> "$BASELINE_DIR/README.md"
echo "- **Entropy Reduction:** $BASELINE_ENTROPY" >> "$BASELINE_DIR/README.md"
echo "- **Leap Success Rate:** $(echo "$BASELINE_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%" >> "$BASELINE_DIR/README.md"

cat >> "$BASELINE_DIR/README.md" << 'BASELINEEOF'

## Files

- `baseline_entry.csv` - Single-line baseline record
- `baseline_history.csv` - Header + baseline entry
- `baseline_diagnostics.csv` - Raw diagnostic data (if available)
- `baseline_report.md` - Diagnostic report (if available)
- `README.md` - This file

## Why This Matters

Every experiment needs a control. This baseline represents:
- **System state before training** (minimal test graph)
- **Zero point for all metrics** (0.0 / 0.0 / 0%)
- **Reference for all future comparisons**

When you reach:
- 0.35 similarity → You've improved by 0.35 from baseline
- 0.50 similarity → 0.50 improvement = coherence threshold crossed
- 0.75 similarity → 0.75 improvement = near-mastery level

## Protection

🔒 **DO NOT:**
- Delete these files
- Modify these files
- Overwrite the baseline

✅ **DO:**
- Reference in all reports
- Calculate deltas from baseline
- Archive alongside milestones
- Use for reproducibility studies

## Calculating Improvement

From any current state:
```bash
CURRENT_SIM=$(tail -1 ../diagnostics_history.csv | cut -d',' -f5)
BASELINE_SIM=$(cat baseline_entry.csv | cut -d',' -f5)
IMPROVEMENT=$(echo "$CURRENT_SIM - $BASELINE_SIM" | bc -l)
echo "Improvement from baseline: +$IMPROVEMENT"
```

## Reproducibility

Original conditions:
- Test graph: 51 nodes, 5 edges
- No trained embeddings
- Default parameters (λ=0.5, threshold=0.6, lr=0.01)

Any experiment should reference this baseline:
"Improvement of +0.42 similarity from control baseline (0.00 → 0.42)"

---

**This baseline is sacred. It makes all future progress measurable.**
BASELINEEOF

echo "✅ Baseline protected in: $BASELINE_DIR/"
echo ""

# Make read-only (optional security)
chmod -R 444 "$BASELINE_DIR"/* 2>/dev/null || true

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📁 Protected Files:"
ls -lh "$BASELINE_DIR/"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Commit to Git
if [ -d .git ]; then
    git add "$BASELINE_DIR/" 2>/dev/null || true
    if ! git diff --cached --quiet 2>/dev/null; then
        git commit -m "🔒 Protect control baseline - DO NOT DELETE

Baseline metrics:
- Context Similarity: $BASELINE_SIM
- Entropy Reduction: $BASELINE_ENTROPY  
- Leap Success: $(echo "$BASELINE_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%

This is the yardstick for all future progress."
        echo "✅ Baseline committed to Git"
    fi
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Control baseline protected. Never delete these files.       ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

