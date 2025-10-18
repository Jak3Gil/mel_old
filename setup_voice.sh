#!/bin/bash

# ============================================================================
# Voice Communication Setup for Melvin
# Installs dependencies for two-way voice communication
# ============================================================================

echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║             Setting Up Melvin Voice Communication                         ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Check Python version
echo "🐍 Checking Python..."
PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2)
echo "   Python version: $PYTHON_VERSION"

# Check OS
OS=$(uname -s)
echo ""
echo "💻 Operating System: $OS"

# Install text-to-speech
echo ""
echo "🎙️ Installing Text-to-Speech..."
if [ "$OS" = "Darwin" ]; then
    echo "   ✅ macOS 'say' command available (built-in)"
    echo "   Testing TTS..."
    say "Text to speech initialized" 2>/dev/null && echo "   ✅ TTS working" || echo "   ⚠️  TTS test failed"
else
    echo "   Installing pyttsx3..."
    pip3 install --user pyttsx3
fi

# Install speech recognition
echo ""
echo "🎧 Installing Speech Recognition..."
echo "   Installing SpeechRecognition..."
pip3 install --user SpeechRecognition

# Install PyAudio (for microphone input)
echo ""
echo "🎤 Installing PyAudio..."
if [ "$OS" = "Darwin" ]; then
    # macOS
    echo "   Installing PortAudio (if not already installed)..."
    brew list portaudio &>/dev/null || brew install portaudio
    echo "   Installing PyAudio..."
    pip3 install --user pyaudio
elif [ "$OS" = "Linux" ]; then
    # Linux
    echo "   Installing system dependencies..."
    sudo apt-get update
    sudo apt-get install -y portaudio19-dev python3-pyaudio
    pip3 install --user pyaudio
else
    echo "   Installing PyAudio..."
    pip3 install --user pyaudio
fi

# Optional: Install pyttsx3 for offline TTS
echo ""
echo "🔊 Installing offline TTS (optional)..."
pip3 install --user pyttsx3 2>/dev/null && echo "   ✅ pyttsx3 installed" || echo "   ⚠️  pyttsx3 installation failed (optional)"

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
    ("SpeechRecognition", "speech_recognition"),
    ("PyAudio", "pyaudio"),
]

installed = sum(check_package(name, imp) for name, imp in packages)

print(f"\n   Core packages: {installed}/{len(packages)}")

# Check optional
print("\n   Optional packages:")
check_package("pyttsx3", "pyttsx3")

PYEOF

# Test microphone access
echo ""
echo "🎤 Testing microphone access..."
python3 << 'PYEOF'
try:
    import speech_recognition as sr
    r = sr.Recognizer()
    with sr.Microphone() as source:
        print("   ✅ Microphone accessible")
except Exception as e:
    print(f"   ⚠️  Microphone test failed: {e}")
PYEOF

# Final summary
echo ""
echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║                         Installation Complete                             ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""
echo "📝 Next steps:"
echo "   1. Test TTS:"
echo "      python3 melvin/io/text_to_speech.py"
echo ""
echo "   2. Start conversation:"
echo "      python3 melvin_conversation.py"
echo ""
echo "   3. Check microphone permissions in System Preferences"
echo ""

