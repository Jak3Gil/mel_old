#!/bin/bash
# ╔═══════════════════════════════════════════════════════════════════════════╗
# ║  START OLLAMA CONTINUOUS LEARNING                                         ║
# ║  Ollama → Melvin → Global Storage (nodes.bin/edges.bin)                  ║
# ╚═══════════════════════════════════════════════════════════════════════════╝

set -e

echo ""
echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║                                                                           ║"
echo "║       🧠⚡ MELVIN CONTINUOUS LEARNING - OLLAMA MODE ⚡🧠                   ║"
echo "║                                                                           ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Check if Ollama is running
echo "Checking Ollama..."
if ! curl -s http://localhost:11434/api/tags >/dev/null 2>&1; then
    echo "❌ ERROR: Ollama is not running!"
    echo ""
    echo "To start Ollama:"
    echo "  1. Install from: https://ollama.ai"
    echo "  2. Run: ollama serve"
    echo "  3. Pull model: ollama pull llama3.2"
    echo ""
    exit 1
fi
echo "✅ Ollama is running!"
echo ""

# Build direct feeder if needed
if [ ! -f "direct_fact_feeder" ]; then
    echo "Building direct fact feeder..."
    g++ -std=c++20 -O3 -I. direct_fact_feeder.cpp \
        melvin/core/storage.cpp \
        melvin/core/learning.cpp \
        melvin/core/leap_inference.cpp \
        melvin/core/adaptive_weighting.cpp \
        melvin/core/gnn_predictor.cpp \
        melvin/core/energy_field.cpp \
        -o direct_fact_feeder
    echo "✅ Built!"
    echo ""
fi

# Make Python script executable
chmod +x ollama_continuous_learning.py

# Check Python dependencies
echo "Checking Python dependencies..."
if ! python3 -c "import requests" 2>/dev/null; then
    echo "Installing requests..."
    pip3 install requests
fi
echo "✅ Ready!"
echo ""

# Run the continuous learning
echo "═══════════════════════════════════════════════════════════════════════════"
echo "Starting continuous learning mode..."
echo "═══════════════════════════════════════════════════════════════════════════"
echo ""
echo "⚠️  Press Ctrl+C to stop gracefully"
echo ""

python3 ollama_continuous_learning.py

echo ""
echo "✅ Learning session complete!"
echo ""

