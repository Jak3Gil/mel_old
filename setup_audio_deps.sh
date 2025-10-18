#!/bin/bash

# ============================================================================
# Audio Subsystem - Dependency Installation Script
# ============================================================================

echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║             Installing Audio Subsystem Dependencies                      ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Check Python version
echo "🐍 Checking Python..."
PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2)
echo "   Python version: $PYTHON_VERSION"

# Check PortAudio
echo ""
echo "🎤 Checking PortAudio..."
if brew list portaudio &>/dev/null; then
    echo "   ✅ PortAudio already installed"
else
    echo "   📦 Installing PortAudio..."
    brew install portaudio
    echo "   ✅ PortAudio installed"
fi

# Install Python dependencies
echo ""
echo "📦 Installing Python dependencies..."
echo ""

# Create requirements file if it doesn't exist
cat > /tmp/audio_requirements.txt << 'EOF'
# Speech Recognition
openai-whisper

# Alternative: Vosk (lightweight, offline)
# vosk

# Audio Classification
# tensorflow>=2.0.0
# tensorflow-hub

# Audio Processing
librosa
soundfile
numpy

# Optional: Better audio handling
# scipy
# pydub
EOF

echo "   Installing from requirements..."
pip3 install -q openai-whisper librosa soundfile numpy

if [ $? -eq 0 ]; then
    echo "   ✅ Core dependencies installed"
else
    echo "   ⚠️  Some dependencies failed to install"
    echo "   Try: pip3 install --user openai-whisper librosa soundfile numpy"
fi

# Optional: TensorFlow (large download)
echo ""
read -p "📦 Install TensorFlow for ambient sound classification? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "   Installing TensorFlow (this may take a while)..."
    pip3 install -q tensorflow tensorflow-hub
    if [ $? -eq 0 ]; then
        echo "   ✅ TensorFlow installed"
    else
        echo "   ⚠️  TensorFlow installation failed"
    fi
else
    echo "   ⏭️  Skipping TensorFlow (can install later)"
fi

# Verify installations
echo ""
echo "✅ Verifying installations..."
echo ""

python3 << 'PYEOF'
import sys

def check_package(name, import_name=None):
    import_name = import_name or name
    try:
        __import__(import_name)
        print(f"   ✅ {name}")
        return True
    except ImportError:
        print(f"   ❌ {name} (not installed)")
        return False

packages = [
    ("Whisper", "whisper"),
    ("Librosa", "librosa"),
    ("SoundFile", "soundfile"),
    ("NumPy", "numpy"),
]

installed = sum(check_package(name, imp) for name, imp in packages)

print(f"\n   Installed: {installed}/{len(packages)}")

# Check optional
print("\n   Optional packages:")
check_package("TensorFlow", "tensorflow")
check_package("TensorFlow Hub", "tensorflow_hub")
PYEOF

# Test scripts
echo ""
echo "🧪 Testing recognition scripts..."
echo ""

# Make scripts executable
chmod +x melvin/scripts/recognize_speech.py 2>/dev/null
chmod +x melvin/scripts/classify_sound.py 2>/dev/null

# Test if Whisper works
python3 -c "import whisper; model = whisper.load_model('base'); print('   ✅ Whisper model loaded successfully')" 2>/dev/null || echo "   ⚠️  Whisper test failed"

echo ""
echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║                         Installation Complete                             ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""
echo "📝 Next steps:"
echo "   1. Test speech recognition:"
echo "      python3 melvin/scripts/recognize_speech.py test_audio.wav"
echo ""
echo "   2. Build audio subsystem:"
echo "      make -f Makefile.audio all"
echo ""
echo "   3. Run tests:"
echo "      ./test_audio_bridge"
echo ""
echo "   4. Integrate with Melvin:"
echo "      See docs/AUDIO_INTEGRATION.md"
echo ""

