#!/bin/bash
# Tag Milestone Commits
# Creates Git tags at important similarity thresholds with detailed notes
# Usage: ./tag_milestone.sh [milestone_name] [notes]

set -e

MILESTONE="${1}"
NOTES="${2:-Milestone achieved}"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║              🏆 TAG MILESTONE COMMIT                          ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Get current metrics
if [ ! -f diagnostics_history.csv ]; then
    echo "❌ Error: diagnostics_history.csv not found"
    exit 1
fi

CURRENT_SIM=$(tail -1 diagnostics_history.csv | cut -d',' -f5)
CURRENT_ENTROPY=$(tail -1 diagnostics_history.csv | cut -d',' -f4)
CURRENT_SUCCESS=$(tail -1 diagnostics_history.csv | cut -d',' -f6)
CURRENT_DATA=$(tail -1 diagnostics_history.csv | cut -d',' -f3)

echo "📊 Current Metrics:"
echo "   Context Similarity:  $CURRENT_SIM"
echo "   Entropy Reduction:   $CURRENT_ENTROPY"
echo "   Leap Success:        $(echo "$CURRENT_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%"
echo "   Data Size:           $CURRENT_DATA"
echo ""

# Auto-detect milestone if not specified
if [ -z "$MILESTONE" ]; then
    if (( $(echo "$CURRENT_SIM >= 0.75" | bc -l 2>/dev/null || echo 0) )); then
        MILESTONE="milestone_4_mastery_0.75"
        NOTES="Abstract concept mastery achieved"
    elif (( $(echo "$CURRENT_SIM >= 0.65" | bc -l 2>/dev/null || echo 0) )); then
        MILESTONE="milestone_3_causal_0.65"
        NOTES="Causal reasoning active"
    elif (( $(echo "$CURRENT_SIM >= 0.50" | bc -l 2>/dev/null || echo 0) )); then
        MILESTONE="milestone_2_coherence_0.50"
        NOTES="COHERENCE THRESHOLD CROSSED - First coherent sentences"
    elif (( $(echo "$CURRENT_SIM >= 0.35" | bc -l 2>/dev/null || echo 0) )); then
        MILESTONE="milestone_1_associations_0.35"
        NOTES="Basic associations established"
    elif (( $(echo "$CURRENT_SIM >= 0.25" | bc -l 2>/dev/null || echo 0) )); then
        MILESTONE="checkpoint_0.25"
        NOTES="Quarter-way to coherence threshold"
    else
        echo "⚠️  Current similarity ($CURRENT_SIM) not at milestone threshold"
        echo "   Milestones: 0.25, 0.35, 0.50, 0.65, 0.75"
        echo ""
        read -p "Tag anyway? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "❌ Cancelled"
            exit 0
        fi
        MILESTONE="checkpoint_$(echo "$CURRENT_SIM" | tr '.' '_')"
    fi
fi

echo "🏷️  Milestone: $MILESTONE"
echo "📝 Notes: $NOTES"
echo ""

# Archive current state
echo "📸 Creating milestone snapshot..."
SNAPSHOT_DIR="milestone_snapshots/$MILESTONE"
mkdir -p "$SNAPSHOT_DIR"

# Copy all diagnostic state
cp diagnostics_history.csv "$SNAPSHOT_DIR/"
cp leap_diagnostics.csv "$SNAPSHOT_DIR/" 2>/dev/null || true
cp leap_tuning_report.md "$SNAPSHOT_DIR/" 2>/dev/null || true
[ -f melvin_progress.png ] && cp melvin_progress.png "$SNAPSHOT_DIR/"
[ -f leap_tuning_results.txt ] && cp leap_tuning_results.txt "$SNAPSHOT_DIR/"

# Create milestone documentation
cat > "$SNAPSHOT_DIR/MILESTONE.md" << EOF
# 🏆 Milestone: $MILESTONE

**Date:** $(date)  
**Notes:** $NOTES

## Metrics Achieved

- **Context Similarity:** $CURRENT_SIM
- **Entropy Reduction:** $CURRENT_ENTROPY
- **Leap Success Rate:** $(echo "$CURRENT_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%
- **Data Size:** $CURRENT_DATA

## Improvement from Baseline

EOF

# Calculate improvement from baseline
if [ -f baseline_control/baseline_entry.csv ]; then
    BASELINE_SIM=$(cut -d',' -f5 baseline_control/baseline_entry.csv)
    BASELINE_ENTROPY=$(cut -d',' -f4 baseline_control/baseline_entry.csv)
    BASELINE_SUCCESS=$(cut -d',' -f6 baseline_control/baseline_entry.csv)
    
    SIM_DELTA=$(echo "$CURRENT_SIM - $BASELINE_SIM" | bc -l)
    ENTROPY_DELTA=$(echo "$CURRENT_ENTROPY - $BASELINE_ENTROPY" | bc -l)
    SUCCESS_DELTA=$(echo "($CURRENT_SUCCESS - $BASELINE_SUCCESS) * 100" | bc -l)
    
    cat >> "$SNAPSHOT_DIR/MILESTONE.md" << EOF
- **Similarity:** +$SIM_DELTA (from $BASELINE_SIM)
- **Entropy:** +$ENTROPY_DELTA (from $BASELINE_ENTROPY)
- **Success:** +$SUCCESS_DELTA% (from $(echo "$BASELINE_SUCCESS * 100" | bc -l)%)

## Configuration

EOF
else
    echo "- (Baseline not yet protected - run ./protect_baseline.sh)" >> "$SNAPSHOT_DIR/MILESTONE.md"
fi

# Extract current config from history
LAMBDA=$(tail -1 diagnostics_history.csv | cut -d',' -f7)
THRESHOLD=$(tail -1 diagnostics_history.csv | cut -d',' -f8)
LEARNING_RATE=$(tail -1 diagnostics_history.csv | cut -d',' -f9)

cat >> "$SNAPSHOT_DIR/MILESTONE.md" << EOF
- lambda_graph_bias: $LAMBDA
- leap_entropy_threshold: $THRESHOLD
- learning_rate_embeddings: $LEARNING_RATE

## Data Layers Ingested

EOF

# List data versions that contributed
if [ -d versioned_data ]; then
    echo "Data versions:" >> "$SNAPSHOT_DIR/MILESTONE.md"
    ls -t versioned_data/ | head -5 | while read version; do
        if [ -f "versioned_data/$version/metadata.json" ]; then
            DESC=$(grep "description" "versioned_data/$version/metadata.json" | cut -d'"' -f4)
            echo "- $version: $DESC" >> "$SNAPSHOT_DIR/MILESTONE.md"
        fi
    done
else
    echo "- Test graph only (no versioned data yet)" >> "$SNAPSHOT_DIR/MILESTONE.md"
fi

cat >> "$SNAPSHOT_DIR/MILESTONE.md" << EOF

## Reproducibility

**Git Commit:** \`$(git rev-parse HEAD 2>/dev/null || echo "not in git")\`  
**Diagnostic Report:** See \`leap_tuning_report.md\` in this directory  
**Full History:** See \`diagnostics_history.csv\` in this directory

To reproduce:
1. Check out commit: \`git checkout <commit_hash>\`
2. Load data versions up to this point
3. Run diagnostic: \`make run_diagnostic\`
4. Compare results to this milestone

## Next Steps

EOF

# Provide next steps based on milestone
if (( $(echo "$CURRENT_SIM >= 0.75" | bc -l 2>/dev/null || echo 0) )); then
    echo "- Achieve mastery (0.80+)" >> "$SNAPSHOT_DIR/MILESTONE.md"
    echo "- Fine-tune for specific domains" >> "$SNAPSHOT_DIR/MILESTONE.md"
elif (( $(echo "$CURRENT_SIM >= 0.65" | bc -l 2>/dev/null || echo 0) )); then
    echo "- Add Layer 4 data (abstract concepts)" >> "$SNAPSHOT_DIR/MILESTONE.md"
    echo "- Test philosophical reasoning" >> "$SNAPSHOT_DIR/MILESTONE.md"
elif (( $(echo "$CURRENT_SIM >= 0.50" | bc -l 2>/dev/null || echo 0) )); then
    echo "- Add Layer 3 data (causal reasoning)" >> "$SNAPSHOT_DIR/MILESTONE.md"
    echo "- Test multi-hop inference" >> "$SNAPSHOT_DIR/MILESTONE.md"
elif (( $(echo "$CURRENT_SIM >= 0.35" | bc -l 2>/dev/null || echo 0) )); then
    echo "- Add Layer 2 data (factual knowledge)" >> "$SNAPSHOT_DIR/MILESTONE.md"
    echo "- Push toward 0.50 coherence threshold" >> "$SNAPSHOT_DIR/MILESTONE.md"
else
    echo "- Continue Layer 1 data (conversations)" >> "$SNAPSHOT_DIR/MILESTONE.md"
    echo "- Build toward 0.35 basic associations" >> "$SNAPSHOT_DIR/MILESTONE.md"
fi

echo "✅ Milestone documented: $SNAPSHOT_DIR/MILESTONE.md"
echo ""

# Git tag
if [ -d .git ]; then
    echo "🏷️  Creating Git tag..."
    
    TAG_MSG="Milestone: $MILESTONE

$NOTES

Metrics:
- Context Similarity: $CURRENT_SIM
- Entropy Reduction: $CURRENT_ENTROPY
- Leap Success: $(echo "$CURRENT_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%

Configuration:
- lambda_graph_bias: $LAMBDA
- leap_entropy_threshold: $THRESHOLD
- learning_rate_embeddings: $LEARNING_RATE"

    # Create annotated tag
    git tag -a "$MILESTONE" -m "$TAG_MSG" 2>/dev/null && \
        echo "✅ Git tag created: $MILESTONE" || \
        echo "⚠️  Tag already exists or Git not available"
    
    echo ""
    echo "📜 Recent milestone tags:"
    git tag -l "milestone_*" "checkpoint_*" 2>/dev/null | tail -5 || echo "   (No previous milestone tags)"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 TO REFERENCE THIS MILESTONE:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "   In reports: \"Improvement from baseline: +$SIM_DELTA\""
echo "   In Git: git checkout $MILESTONE"
echo "   In docs: See milestone_snapshots/$MILESTONE/MILESTONE.md"
echo ""

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Milestone tagged. This moment in training is now permanent.  ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

