#!/bin/bash
# Quick start script for first-time setup

echo "╔═══════════════════════════════════════════════════════╗"
echo "║  MELVIN QUICK START                                   ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""

cd "$(dirname "$0")/.."

echo "🚀 Setting up Melvin..."
echo ""

# Create data directories
echo "📁 Creating data directories..."
mkdir -p data/snapshots
mkdir -p data/inbox
echo "✅ Directories created"
echo ""

# Build Melvin
echo "🔨 Building Melvin..."
make clean && make all
echo "✅ Build complete"
echo ""

# Run a quick test
echo "🧪 Running quick test..."
echo "What is fire?" | ./melvin
echo ""

echo "╔═══════════════════════════════════════════════════════╗"
echo "║  READY TO GO!                                         ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""
echo "Try these commands:"
echo "  ./melvin                    - Interactive mode"
echo "  ./melvin 'your question'    - Single query"
echo "  make run_demo_reasoning     - Run reasoning demo"
echo "  make run_demo_learning      - Run learning demo"
echo ""

