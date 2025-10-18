#!/bin/bash
# Build all Melvin components

set -e

echo "╔═══════════════════════════════════════════════════════╗"
echo "║  MELVIN BUILD SCRIPT                                  ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""

cd "$(dirname "$0")/.."

echo "🔨 Building Melvin..."
make clean
make all

echo ""
echo "🔨 Building demos..."
make demos

echo ""
echo "✅ Build complete!"
echo ""
echo "Run with:"
echo "  ./melvin                    - Interactive mode"
echo "  ./melvin 'your question'    - Single query"
echo "  make run                    - Interactive mode (via Make)"
echo ""

