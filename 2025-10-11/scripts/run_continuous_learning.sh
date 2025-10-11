#!/usr/bin/env bash
set -euo pipefail

echo "=== Melvin Continuous Learning Mode ==="
echo ""

# Create directories
mkdir -p logs data/inbox data/processed data/failed data/snapshots

# Build
echo "Building continuous learning watcher..."
make cl

# Run
echo ""
echo "Starting watcher (Press Ctrl+C to stop)..."
./bin/melvin_learn_watch

