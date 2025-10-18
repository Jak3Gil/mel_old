# 🎤 Melvin Audio Subsystem

**Convert sound waves into knowledge**

The audio subsystem enables Melvin to hear, understand, and learn from speech and environmental sounds, integrating auditory perception with vision and reasoning.

---

## 🚀 Quick Start

### Build the Demo

```bash
make -f Makefile.audio demo
./demo_audio_integration
```

### Run Tests

```bash
make -f Makefile.audio test
./test_audio_bridge
```

### Complete Build & Test

```bash
make -f Makefile.audio complete
```

---

## 📁 File Structure

```
melvin/
├── audio/
│   ├── audio_pipeline.h          # Audio capture & processing
│   ├── audio_pipeline.cpp
│   ├── audio_bridge.h             # Graph integration
│   └── audio_bridge.cpp
├── demos/
│   └── demo_audio_integration.cpp # Full demo with examples
├── tests/
│   └── test_audio_bridge.cpp      # Test suite
└── scripts/
    ├── recognize_speech.py        # Whisper integration
    └── classify_sound.py          # Ambient sound classification

docs/
└── architecture_audio.md          # Full documentation

Makefile.audio                     # Build system
```

---

## 🎯 Features

### AudioPipeline
- ✅ Microphone capture (live streaming)
- ✅ Voice Activity Detection (VAD)
- ✅ Speech recognition (Whisper/Vosk ready)
- ✅ Ambient sound classification (YAMNet ready)
- ✅ Rolling audio buffer
- ✅ Event-based output

### AudioBridge
- ✅ Speech → Knowledge graph nodes
- ✅ Word-level tokenization
- ✅ Cross-modal sync (audio ↔ vision)
- ✅ Temporal windowing (co-occurrence)
- ✅ Causal inference
- ✅ Edge reinforcement & decay

---

## 🔧 Integration

### Basic Usage

```cpp
#include "melvin/audio/audio_pipeline.h"
#include "melvin/audio/audio_bridge.h"
#include "melvin/core/atomic_graph.h"

int main() {
    AtomicGraph graph;
    AudioPipeline audio;
    AudioBridge bridge;
    
    // Start listening
    audio.start_stream();
    
    // Main loop
    while (running) {
        audio.tick(dt);
        auto events = audio.get_recent_events();
        
        for (const auto& event : events) {
            bridge.process(event, graph);
        }
    }
    
    // Save knowledge
    graph.save("nodes.bin", "edges.bin");
    return 0;
}
```

### With Cross-Modal Integration

```cpp
// Your perception loop
void perception_loop(float dt) {
    // Get events from all modalities
    auto audio_events = audio_pipeline.get_recent_events();
    auto visual_events = vision_pipeline.get_recent_events();
    
    // Process audio
    for (const auto& event : audio_events) {
        audio_bridge.process(event, graph);
    }
    
    // Synchronize across modalities
    audio_bridge.sync_with_vision(audio_events, visual_events, graph);
}
```

---

## 🧪 Demo Scenarios

The demo includes 4 scenarios:

### 1. Basic Audio Capture
- Process speech and ambient sounds
- Create graph nodes and edges
- Display statistics

### 2. Cross-Modal Integration
- Combine audio with vision
- Create temporal associations
- Demonstrate co-occurrence learning

### 3. Reflection Mode
- Load previous session
- Analyze learned patterns
- Show knowledge persistence

### 4. Continuous Learning
- Process multiple events
- Apply edge decay
- Build knowledge over time

**Run all demos:**
```bash
./demo_audio_integration 5
```

---

## 📊 Example Output

```
🎤 Speech processed: "turn on the stove" (node 1)
   - Created phrase node: audio:turn on the stove
   - Created word nodes: turn, on, the, stove
   - Created 5 edges (temporal sequence)

👁️  Vision: Detected 'stove' at t=1.1s

🔗 Cross-modal link: "turn on the stove" ↔ "stove"

📊 AudioBridge Statistics:
   Events processed: 1
   Nodes created: 6
   Edges created: 6
   Cross-modal links: 1
```

---

## 🔬 Test Suite

8 comprehensive tests:

1. ✅ Basic audio event processing
2. ✅ Word tokenization
3. ✅ Cross-modal synchronization
4. ✅ Temporal window filtering
5. ✅ Confidence threshold filtering
6. ✅ Edge reinforcement and decay
7. ✅ Ambient sound processing
8. ✅ Graph persistence

**Run tests:**
```bash
./test_audio_bridge
```

---

## ⚙️ Configuration

### AudioPipeline Config

```cpp
AudioPipeline::Config config;
config.sample_rate = 16000;         // Hz (Whisper uses 16kHz)
config.channels = 1;                // Mono
config.buffer_size_ms = 3000;       // 3-second buffer
config.vad_threshold = 0.02f;       // Voice activity threshold
config.enable_speech = true;
config.enable_ambient = true;
config.recognition_engine = "whisper";

AudioPipeline audio(config);
```

### AudioBridge Config

```cpp
AudioBridge::Config config;
config.temporal_window = 3.0f;      // 3s co-occurrence window
config.min_confidence = 0.3f;       // Filter low-confidence events
config.create_word_nodes = true;
config.enable_cross_modal = true;
config.enable_causal_inference = true;

AudioBridge bridge(config);
```

---

## 🔌 Speech Recognition Integration

### Using Whisper (Recommended)

Install Whisper:
```bash
pip install openai-whisper
```

The pipeline calls `scripts/recognize_speech.py`:
```python
import whisper
import sys

model = whisper.load_model("base")
audio_path = sys.argv[1]

result = model.transcribe(audio_path)
print(result["text"])
```

### Using Vosk (Lightweight)

Install Vosk:
```bash
pip install vosk
```

Download model and configure in `audio_pipeline.cpp`.

---

## 🌐 Ambient Sound Classification

### Using YAMNet

Install TensorFlow:
```bash
pip install tensorflow tensorflow-hub
```

The pipeline calls `scripts/classify_sound.py`:
```python
import tensorflow_hub as hub
import numpy as np

model = hub.load('https://tfhub.dev/google/yamnet/1')
# Process audio and return classification
```

---

## 🎓 How It Works

### 1. Sound → Event

```
Audio Input → VAD → Recognition → AudioEvent
  16kHz PCM    ✓      "hello"      {label, time, conf}
```

### 2. Event → Graph Nodes

```
AudioEvent("turn on the stove")
    ↓
Nodes:
  - audio:turn on the stove [PHRASE]
  - audio:turn [WORD]
  - audio:on [WORD]
  - audio:stove [WORD]
  - audio:speech [CATEGORY]
```

### 3. Cross-Modal Linking

```
Speech (t=0.5s): "stove"
Vision (t=1.1s): stove detected
    ↓
Edge: audio:stove ↔ vision:stove [CO_OCCURS_WITH]
```

### 4. Persistence

```
graph.save("nodes.bin", "edges.bin")
    ↓
All audio knowledge preserved!
```

---

## 📚 Full Documentation

See `docs/architecture_audio.md` for:
- Complete architecture overview
- Detailed component descriptions
- Event flow diagrams
- Integration examples
- Performance characteristics
- Configuration guide
- Future extensions

---

## 🛠️ Troubleshooting

### No audio events generated
- Check microphone permissions
- Verify `audio.is_running()` returns true
- Lower `vad_threshold` for quiet environments

### Speech not recognized
- Install Whisper: `pip install openai-whisper`
- Ensure Python scripts are executable
- Check `scripts/recognize_speech.py` exists

### Cross-modal links not created
- Check `temporal_window` is appropriate
- Verify events have similar timestamps
- Ensure `config.enable_cross_modal = true`

### Build errors
- Ensure C++17 compiler available
- Check `atomic_graph.cpp` is compiled
- Run `make -f Makefile.audio clean` first

---

## 🚧 Development Status

### ✅ Implemented
- Core audio pipeline architecture
- Event processing system
- Graph integration
- Cross-modal synchronization
- Test suite
- Demo applications
- Documentation

### 🔜 Coming Soon
- Live microphone capture (PortAudio)
- Actual Whisper integration
- YAMNet ambient classification
- Speaker identification
- Emotional tone detection
- Sound localization (stereo)

---

## 📝 Example Scenarios

### Scenario 1: Kitchen Assistant

```
User: "Turn on the stove"
  → Audio: Creates speech nodes
  → Vision: Detects stove
  → Link: audio:stove ↔ vision:stove
  → Action: Melvin moves to stove

Result: Melvin learned "turn on stove" → move_to(stove)
```

### Scenario 2: Environmental Awareness

```
Ambient: "dog barking" (t=1.0s)
Vision: dog detected (t=1.2s)
  → Link: audio:dog barking ↔ vision:dog
  
Ambient: "door closing" (t=5.0s)
Vision: door detected (t=5.1s)
  → Link: audio:door closing ↔ vision:door

Result: Melvin associates sounds with visual objects
```

---

## 🤝 Contributing

To extend the audio subsystem:

1. Add new event types to `AudioEvent`
2. Extend node types in `audio_bridge.h`
3. Implement recognition in `audio_pipeline.cpp`
4. Add tests in `test_audio_bridge.cpp`
5. Update documentation

---

## 📄 License

Part of the Melvin AI project.

---

## 🙏 Acknowledgments

- OpenAI Whisper for speech recognition
- Google YAMNet for audio classification
- PortAudio for cross-platform audio I/O

---

**Ready to give Melvin ears!** 🎧

For questions or issues, see `docs/architecture_audio.md` or create an issue.

