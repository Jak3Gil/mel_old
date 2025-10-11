#!/bin/bash
# Archive Diagnostics Weekly
# Commits diagnostic state to Git for full learning curve replay
# Usage: ./archive_diagnostics.sh [commit_message]

set -e

COMMIT_MSG="${1:-Weekly diagnostic snapshot $(date +%Y-%m-%d)}"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║              📦 ARCHIVE WEEKLY DIAGNOSTICS                    ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check if in git repo
if [ ! -d .git ]; then
    echo "⚠️  Not in a Git repository. Initializing..."
    git init
    git add .gitignore 2>/dev/null || echo "*~" > .gitignore
    git add .gitignore
    git commit -m "Initial commit" 2>/dev/null || true
fi

# Create diagnostic snapshot directory
SNAPSHOT_DIR="diagnostic_snapshots"
mkdir -p "$SNAPSHOT_DIR"

# Date stamp
DATE=$(date +%Y%m%d_%H%M%S)

# Copy current diagnostic state
echo "📸 Creating snapshot: $DATE"

cp diagnostics_history.csv "$SNAPSHOT_DIR/history_$DATE.csv" 2>/dev/null || true
cp leap_diagnostics.csv "$SNAPSHOT_DIR/leap_diagnostics_$DATE.csv" 2>/dev/null || true
cp leap_tuning_report.md "$SNAPSHOT_DIR/report_$DATE.md" 2>/dev/null || true
[ -f melvin_progress.png ] && cp melvin_progress.png "$SNAPSHOT_DIR/progress_$DATE.png"
[ -f leap_tuning_results.txt ] && cp leap_tuning_results.txt "$SNAPSHOT_DIR/tuning_$DATE.txt"

echo "✅ Snapshot saved to $SNAPSHOT_DIR/"

# Extract current metrics for commit message
CURRENT_SIM=$(tail -1 diagnostics_history.csv | cut -d',' -f5 2>/dev/null || echo "0.000")
CURRENT_ENTROPY=$(tail -1 diagnostics_history.csv | cut -d',' -f4 2>/dev/null || echo "0.000")
CURRENT_SUCCESS=$(tail -1 diagnostics_history.csv | cut -d',' -f6 2>/dev/null || echo "0.0")

# Enhanced commit message with metrics
FULL_MSG="$COMMIT_MSG

Metrics:
- Context Similarity: $CURRENT_SIM
- Entropy Reduction: $CURRENT_ENTROPY
- Leap Success: $(echo "$CURRENT_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%

Files: diagnostics_history.csv, leap_diagnostics.csv, leap_tuning_report.md"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📝 Git Commit:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "$FULL_MSG"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Stage diagnostic files
git add diagnostics_history.csv 2>/dev/null || true
git add leap_diagnostics.csv 2>/dev/null || true
git add leap_tuning_report.md 2>/dev/null || true
git add "$SNAPSHOT_DIR/" 2>/dev/null || true

# Commit
if git diff --cached --quiet 2>/dev/null; then
    echo "ℹ️  No changes to commit (diagnostics unchanged)"
else
    git commit -m "$FULL_MSG"
    echo "✅ Committed to Git"
fi

# Show recent commits
echo ""
echo "📜 Recent diagnostic commits:"
git log --oneline --grep="Metrics:" -n 5 2>/dev/null || echo "   (No previous diagnostic commits)"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Diagnostic state archived. Learning curve preserved.         ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Optional: Create summary of all snapshots
echo "📊 All snapshots:"
ls -lh "$SNAPSHOT_DIR/" | tail -n +2 || true
echo ""

