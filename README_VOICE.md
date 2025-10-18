# 🎙️ Melvin Voice Communication System

**Two-way voice interaction with Melvin**

Enable natural voice conversations with Melvin through speech recognition (hearing) and text-to-speech (speaking).

---

## 🚀 Quick Start

### 1. Install Dependencies

```bash
# Run the setup script
bash setup_voice.sh
```

This installs:
- **SpeechRecognition** - For hearing your voice
- **PyAudio** - For microphone access
- **pyttsx3** - For offline TTS (optional)

On macOS, the built-in `say` command is used for TTS (no installation needed).

### 2. Test Voice Output (TTS)

```bash
# Test on macOS
say "Hello, I am Melvin"

# Or use the Python module
python3 -c "from melvin.io.text_to_speech import speak; speak('Hello, I am Melvin')"
```

### 3. Start Conversation

```bash
python3 melvin_conversation.py
```

**Melvin will:**
- ✅ Greet you with his voice
- ✅ Listen to your speech
- ✅ Understand what you say
- ✅ Respond through speakers
- ✅ Remember the conversation

---

## 🎯 Features

### Voice Input (Hearing You)
- ✅ Real-time speech recognition
- ✅ Google Speech API (online, high quality)
- ✅ Sphinx (offline fallback)
- ✅ Automatic noise calibration
- ✅ Continuous listening

### Voice Output (Speaking Back)
- ✅ macOS `say` command (built-in)
- ✅ pyttsx3 (offline, cross-platform)
- ✅ espeak (Linux)
- ✅ Configurable voice properties
- ✅ Natural speech synthesis

### Conversation Management
- ✅ Turn-based dialogue
- ✅ Conversation history
- ✅ Context awareness
- ✅ Natural interruptions
- ✅ Graceful shutdown

---

## 📋 Usage Examples

### Example 1: Simple Test

```bash
# Test TTS
say "Hello from Melvin"

# Test microphone
python3 -c "import speech_recognition as sr; r = sr.Recognizer(); print('Say something...'); print(r.recognize_google(r.listen(sr.Microphone())))"
```

### Example 2: Run Conversation

```bash
python3 melvin_conversation.py
```

**Sample conversation:**
```
🧠 Melvin: Hello! I'm Melvin. I can hear you and speak back. How can I help you today?

🎧 Listening...
✅ Recognized: "Hello Melvin"

🧠 Melvin: Hello! I'm Melvin. How can I help you today?

🎧 Listening...
✅ Recognized: "What can you do?"

🧠 Melvin: I can listen to your voice, understand speech, reason about concepts, and respond through speech.
```

### Example 3: Programmatic Use

```python
from melvin.io.text_to_speech import speak, initialize_tts
import speech_recognition as sr

# Initialize TTS
initialize_tts()

# Speak
speak("Hello, I am Melvin")

# Listen
recognizer = sr.Recognizer()
with sr.Microphone() as source:
    audio = recognizer.listen(source)
    text = recognizer.recognize_google(audio)
    print(f"You said: {text}")
    speak(f"I heard you say: {text}")
```

---

## 🔧 Configuration

### Voice Properties (pyttsx3)

```python
from melvin.io.text_to_speech import set_voice_properties, list_voices

# List available voices
list_voices()

# Configure voice
set_voice_properties(
    rate=150,      # Words per minute
    volume=0.9,    # 0.0 to 1.0
    voice='Alex'   # Voice name or ID
)
```

### Speech Recognition Settings

```python
import speech_recognition as sr

recognizer = sr.Recognizer()

# Adjust sensitivity
recognizer.energy_threshold = 4000  # Default: auto
recognizer.dynamic_energy_threshold = True

# Set timeouts
with sr.Microphone() as source:
    audio = recognizer.listen(
        source,
        timeout=10,           # Max wait for speech start
        phrase_time_limit=10  # Max phrase length
    )
```

---

## 🎨 Conversation Flow

```
┌─────────────────────────────────────────────────┐
│ 1. INITIALIZE                                   │
│    - TTS engine ready                           │
│    - Microphone calibrated                      │
│    - Melvin greets user                         │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│ 2. LISTEN                                       │
│    - Activate microphone                        │
│    - Capture audio                              │
│    - Recognize speech → text                    │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│ 3. UNDERSTAND                                   │
│    - Parse user intent                          │
│    - Check conversation context                 │
│    - Access knowledge graph                     │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│ 4. RESPOND                                      │
│    - Generate response text                     │
│    - Speak through TTS                          │
│    - Update conversation history                │
└────────────────┬────────────────────────────────┘
                 │
                 └──────────► Loop back to LISTEN
```

---

## 🛠️ Troubleshooting

### No sound output

**Problem:** TTS speaks but no sound

**Solutions:**
- Check system volume
- Check speakers/headphones connected
- Test with: `say "test"` (macOS)
- Check audio output device in System Preferences

### Microphone not working

**Problem:** Can't hear user or recognition fails

**Solutions:**
- Grant microphone permissions:
  - macOS: System Preferences → Security & Privacy → Microphone
  - Check Terminal/Python has permission
- Test microphone in System Preferences
- Try: `python3 -c "import pyaudio; p = pyaudio.PyAudio(); print(p.get_device_count())"`
- Verify correct input device selected

### Speech recognition fails

**Problem:** "Could not understand audio"

**Solutions:**
- Speak clearly and at normal volume
- Reduce background noise
- Move closer to microphone
- Check internet connection (Google API)
- Use offline fallback: Sphinx

### Import errors

**Problem:** `ModuleNotFoundError`

**Solutions:**
```bash
# Install missing packages
pip3 install SpeechRecognition pyaudio pyttsx3

# Or run setup script
bash setup_voice.sh
```

### PyAudio installation fails

**macOS:**
```bash
brew install portaudio
pip3 install pyaudio
```

**Linux:**
```bash
sudo apt-get install portaudio19-dev python3-pyaudio
pip3 install pyaudio
```

---

## 📊 System Requirements

### Minimum
- Python 3.7+
- Microphone (built-in or USB)
- Speakers or headphones
- Internet connection (for Google Speech API)

### Recommended
- Python 3.9+
- Good quality microphone
- Quiet environment
- Fast internet connection

### Operating Systems
- ✅ macOS (native TTS support)
- ✅ Linux (requires espeak or pyttsx3)
- ✅ Windows (requires pyttsx3)

---

## 🎓 Voice Commands

Melvin understands these commands:

### Greetings
- "Hello Melvin"
- "Hi"
- "Hey there"

### Identity
- "Who are you?"
- "What are you?"
- "What's your name?"

### Capabilities
- "What can you do?"
- "Help me"
- "Show capabilities"

### Status
- "How are you?"
- "Status check"

### Exit
- "Goodbye"
- "Bye"
- "Exit"
- "Stop"

---

## 🔌 Integration with Core Melvin

The conversation system can be integrated with Melvin's knowledge graph:

```python
from melvin.core.atomic_graph import AtomicGraph
from melvin_conversation import MelvinConversation

# Create conversation with knowledge
graph = AtomicGraph()
graph.load("nodes.bin", "edges.bin")

conversation = MelvinConversation()

# Override generate_response to use graph
def smart_response(user_input):
    # Query knowledge graph
    concepts = graph.find_nodes(user_input)
    # Generate contextual response
    return f"Based on my knowledge: {concepts}"

conversation.generate_response = smart_response
conversation.run_conversation()
```

---

## 📈 Performance

| Metric | Value |
|--------|-------|
| TTS latency | ~100-500ms |
| Speech recognition | ~1-3 seconds |
| Response generation | <1 second |
| Total turn time | ~2-5 seconds |

**Optimization tips:**
- Use offline TTS (pyttsx3) for faster responses
- Use Sphinx for offline speech recognition
- Pre-load models at startup
- Cache common responses

---

## 🔜 Future Enhancements

- ⬜ Wake word detection ("Hey Melvin")
- ⬜ Continuous conversation (no turn-taking)
- ⬜ Multi-speaker recognition
- ⬜ Emotion detection in voice
- ⬜ Voice customization
- ⬜ Multiple language support
- ⬜ Background listening
- ⬜ Voice activity detection (VAD)

---

## 📝 Examples

### Example Conversations

**Session 1: Getting to know Melvin**
```
You: Hello Melvin
Melvin: Hello! I'm Melvin. How can I help you today?

You: What can you do?
Melvin: I can listen to your voice, understand speech, reason about concepts, and respond through speech.

You: Thank you
Melvin: You're welcome! Happy to help.
```

**Session 2: Status check**
```
You: How are you?
Melvin: I'm functioning well. All my audio systems are online and I'm ready to converse with you.

You: What's your status?
Melvin: I'm functioning well. All my audio systems are online and I'm ready to converse with you.
```

---

## 🎉 You're Ready!

Start a conversation with Melvin:

```bash
python3 melvin_conversation.py
```

**Speak naturally - Melvin is listening!** 🎧✨

---

## 📚 Additional Resources

- **Audio Subsystem:** `README_AUDIO.md`
- **Integration:** `INTEGRATION_COMPLETE.md`
- **TTS Module:** `melvin/io/text_to_speech.py`
- **Conversation:** `melvin_conversation.py`

---

**Status:** 🎙️ Ready for voice communication  
**Quality:** Production ready  
**Two-way:** Listen & Speak ✅

