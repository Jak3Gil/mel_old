# 🎤 Melvin Audio Perception & Integration Architecture

## Overview

The **Audio Subsystem** converts sound waves into knowledge graph nodes, enabling Melvin to:

- **Hear and understand** speech and environmental sounds
- **Associate audio with vision** and other modalities
- **Learn causal relationships** from temporal patterns
- **Build semantic understanding** through cross-modal integration
- **Reflect and improve** from past auditory experiences

---

## System Architecture

### 1. Component Hierarchy

```
┌─────────────────────────────────────────────────────────┐
│                    Main Application                     │
│                     (melvin/main.cpp)                   │
└───────────────┬─────────────────────────────────────────┘
                │
    ┌───────────┴───────────┬─────────────────────────────┐
    │                       │                             │
┌───▼───────────┐   ┌───────▼─────────┐   ┌─────────────▼─────┐
│ AudioPipeline │   │  AudioBridge    │   │  AtomicGraph      │
│  (Capture &   │──▶│  (Integration)  │──▶│  (Knowledge Base) │
│   Processing) │   └─────────────────┘   └───────────────────┘
└───────────────┘           │
        │                   │
        │          ┌────────┴────────┐
        │          │                 │
        ▼          ▼                 ▼
┌──────────┐  ┌────────┐      ┌──────────┐
│ Speech   │  │ Vision │      │   Text   │
│ Recogn.  │  │ Events │      │  Events  │
└──────────┘  └────────┘      └──────────┘
```

---

## Core Components

### AudioPipeline

**Responsibility:** Capture, process, and analyze raw audio data.

**Features:**
- Microphone capture (via PortAudio/ALSA/CoreAudio)
- Voice Activity Detection (VAD)
- Speech recognition (Whisper/Vosk integration)
- Ambient sound classification (YAMNet/CNN)
- Rolling audio buffer (configurable window)
- Event emission (discrete audio events)

**Key Methods:**
```cpp
bool start_stream(int device_index = -1);
void process_frame(const AudioFrame& frame);
std::vector<AudioEvent> get_recent_events();
void tick(float dt);
```

**Configuration:**
```cpp
AudioPipeline::Config config;
config.sample_rate = 16000;         // 16kHz (Whisper standard)
config.buffer_size_ms = 3000;       // 3-second rolling buffer
config.vad_threshold = 0.02f;       // Voice activity threshold
config.enable_speech = true;
config.enable_ambient = true;
config.recognition_engine = "whisper";
```

---

### AudioBridge

**Responsibility:** Integrate audio events into the knowledge graph.

**Features:**
- Convert AudioEvent → Graph nodes
- Word-level tokenization (create nodes per word)
- Cross-modal synchronization (audio ↔ vision ↔ text)
- Temporal windowing (link events close in time)
- Causal inference (detect cause-effect patterns)
- Edge reinforcement and decay

**Key Methods:**
```cpp
std::vector<uint64_t> process(const AudioEvent& event, AtomicGraph& graph);
void sync_with_vision(const std::vector<AudioEvent>& audio_events,
                      const std::vector<VisualEvent>& visual_events,
                      AtomicGraph& graph);
void infer_causality(AtomicGraph& graph);
void reinforce_patterns(AtomicGraph& graph, float decay_factor);
```

**Configuration:**
```cpp
AudioBridge::Config config;
config.temporal_window = 3.0f;      // 3-second co-occurrence window
config.min_confidence = 0.3f;       // Filter low-confidence events
config.create_word_nodes = true;    // Create individual word nodes
config.enable_cross_modal = true;   // Enable audio-vision linking
config.enable_causal_inference = true;
```

---

## Data Flow

### 1. Sound → Event → Node

```
┌──────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────┐
│  Sound   │ ──▶│ AudioPipeline│ ──▶│ AudioBridge  │ ──▶│  Graph   │
│  Waves   │    │  (Process)   │    │  (Convert)   │    │  Nodes   │
└──────────┘    └──────────────┘    └──────────────┘    └──────────┘
                      │                    │
                      ▼                    ▼
                ┌──────────┐         ┌──────────┐
                │AudioEvent│         │  Edges   │
                │ - label  │         │  (Links) │
                │ - type   │         └──────────┘
                │ - time   │
                └──────────┘
```

### 2. Speech Processing Example

**Input:** "turn on the stove"

**Step 1: AudioPipeline detects speech**
```cpp
AudioEvent {
    id = 1,
    timestamp = 0.5s,
    label = "turn on the stove",
    type = "speech",
    confidence = 0.95,
    source = "mic"
}
```

**Step 2: AudioBridge creates nodes**
```
Nodes created:
  - audio:turn on the stove [AUDIO_PHRASE]
  - audio:turn [AUDIO_WORD]
  - audio:on [AUDIO_WORD]
  - audio:the [AUDIO_WORD]
  - audio:stove [AUDIO_WORD]
  - audio:speech [AUDIO_CATEGORY]

Edges created:
  - audio:turn on the stove → audio:speech [INSTANCE_OF]
  - audio:turn → audio:turn on the stove [INSTANCE_OF]
  - audio:on → audio:turn on the stove [INSTANCE_OF]
  - audio:turn → audio:on [TEMPORAL_NEXT]
  - audio:on → audio:the [TEMPORAL_NEXT]
  - audio:the → audio:stove [TEMPORAL_NEXT]
```

**Step 3: Cross-modal sync (if vision detects stove)**
```
Vision event (t=1.1s): "stove" detected

AudioBridge creates:
  - audio:stove ↔ vision:stove [CO_OCCURS_WITH]
```

---

## Node & Edge Types

### Audio Node Types

```cpp
enum AudioNodeType : uint8_t {
    AUDIO_WORD = 10,        // Individual word ("stove")
    AUDIO_PHRASE = 11,      // Multi-word phrase ("turn on the stove")
    AUDIO_SOUND = 12,       // Ambient sound ("dog barking")
    AUDIO_CATEGORY = 13,    // Category ("speech", "ambient")
    AUDIO_SPEAKER = 14,     // Speaker identity (future)
    AUDIO_EMOTION = 15,     // Emotional tone (future)
    AUDIO_LOCATION = 16     // Sound source location (future)
};
```

### Audio Edge Types

```cpp
enum AudioRelation : uint8_t {
    SPOKEN_AT = 10,         // Word spoken at timestamp
    HEARD_AS = 11,          // Sound interpreted as concept
    FOLLOWS_SPEECH = 12,    // Temporal sequence in speech
    CAUSED_BY_SOUND = 13,   // Causal relationship
    SOUNDS_LIKE = 14        // Similarity between sounds
};
```

### Standard Edge Types (from AtomicGraph)

```cpp
enum Relation : uint8_t {
    INSTANCE_OF = 0,        // Word → Phrase
    CO_OCCURS_WITH = 1,     // Audio ↔ Vision
    OBSERVED_AS = 2,        // Sound → Concept
    NEAR = 3,               // Spatial proximity
    TEMPORAL_NEXT = 4,      // Sequential speech
    NAMED = 5               // Label assignment
};
```

---

## Cross-Modal Synchronization

### Temporal Window Algorithm

```cpp
// Events are linked if they occur within temporal_window
bool is_temporally_close(float t1, float t2) {
    return abs(t1 - t2) <= config.temporal_window;  // Default: 3.0s
}
```

### Example: Audio + Vision Integration

**Timeline:**
```
t=0.0s: (nothing)
t=0.5s: 🎤 "turn on the stove" (speech)
t=1.1s: 👁️  stove detected (vision)
t=2.0s: 🎤 "dog barking" (ambient)
t=5.0s: 👁️  door detected (vision)
```

**Synchronization (temporal_window = 3.0s):**
```cpp
sync_with_vision(audio_events, visual_events, graph);

// Creates links:
// - "turn on the stove" ↔ stove (0.6s apart, within window)
// - "dog barking" ↔ stove (0.9s apart, within window)
// - door not linked (3.0s apart, outside window)
```

---

## Integration Examples

### Example 1: Basic Integration

```cpp
#include "melvin/audio/audio_pipeline.h"
#include "melvin/audio/audio_bridge.h"
#include "melvin/core/atomic_graph.h"

int main() {
    // Create components
    AtomicGraph graph;
    AudioPipeline audio;
    AudioBridge bridge;
    
    // Start audio capture
    audio.start_stream();
    
    // Main loop
    while (running) {
        // Process audio
        audio.tick(dt);
        
        // Get events
        auto events = audio.get_recent_events();
        
        // Integrate into graph
        for (const auto& event : events) {
            bridge.process(event, graph);
        }
        
        // Periodic save
        if (should_save) {
            graph.save("nodes.bin", "edges.bin");
        }
    }
    
    audio.stop_stream();
    return 0;
}
```

### Example 2: Cross-Modal Integration

```cpp
// In your main perception loop:
void perception_loop(float dt) {
    // Update all modalities
    audio_pipeline.tick(dt);
    vision_pipeline.tick(dt);
    
    // Get recent events
    auto audio_events = audio_pipeline.get_recent_events();
    auto visual_events = vision_pipeline.get_recent_events();
    
    // Process audio
    for (const auto& event : audio_events) {
        audio_bridge.process(event, graph);
    }
    
    // Cross-modal sync
    audio_bridge.sync_with_vision(audio_events, visual_events, graph);
    
    // Apply learning dynamics
    audio_bridge.reinforce_patterns(graph, 0.99f);  // 1% decay per tick
}
```

### Example 3: Reflection Mode

```cpp
void reflect_on_past_audio(const std::string& session_log) {
    AtomicGraph graph;
    AudioPipeline reflection;
    AudioBridge bridge;
    
    // Load past session
    reflection.reprocess_log(session_log);
    
    // Reprocess all events
    auto events = reflection.get_events_in_range(0.0f, 1000.0f);
    for (const auto& event : events) {
        bridge.process(event, graph);
    }
    
    // Analyze patterns
    bridge.infer_causality(graph);
    
    // Save refined knowledge
    graph.save("reflected_nodes.bin", "reflected_edges.bin");
}
```

---

## Event Trace Example

### Scenario: User asks Melvin to turn on the stove

```
═══════════════════════════════════════════════════════════
TIME    EVENT                           GRAPH UPDATE
═══════════════════════════════════════════════════════════
0.0s    (quiet)                         -

0.5s    🎤 Speech: "turn on the stove"  Nodes: +6 (phrase, 4 words, category)
                                        Edges: +5 (word seq, category)

1.1s    👁️  Vision: stove detected      Nodes: +1 (vision:stove)
                                        Edges: +1 (audio:stove ↔ vision:stove)

1.5s    🤖 Action: move_to(stove)       Nodes: +1 (action:move_to)
                                        Edges: +1 (audio:stove → action:move_to)

2.0s    🔊 Ambient: "gas ignition"      Nodes: +2 (sound, category)
                                        Edges: +2 (sound → action [CAUSAL])

═══════════════════════════════════════════════════════════

LEARNED PATTERN:
  "turn on the stove" 
    → stove (vision)
    → move_to(stove) (action)
    → "gas ignition" (ambient)
    
CAUSAL CHAIN:
  Speech → Vision → Action → Sound (success)
```

---

## Reflection Cycle

Melvin can reflect on past audio to strengthen patterns:

```
┌────────────────────────────────────────────────────┐
│ PHASE 1: LIVE PERCEPTION                           │
│ - Capture audio in real-time                       │
│ - Create initial nodes/edges                       │
│ - Save session log                                 │
└─────────────────┬──────────────────────────────────┘
                  │
                  ▼
┌────────────────────────────────────────────────────┐
│ PHASE 2: REFLECTION (offline)                      │
│ - Reload audio log                                 │
│ - Reprocess with full context                      │
│ - Identify repeated patterns                       │
│ - Strengthen reliable associations                 │
└─────────────────┬──────────────────────────────────┘
                  │
                  ▼
┌────────────────────────────────────────────────────┐
│ PHASE 3: CONSOLIDATION                             │
│ - Merge new patterns into main graph               │
│ - Apply decay to weak edges                        │
│ - Prune redundant nodes                            │
│ - Save refined knowledge                           │
└────────────────────────────────────────────────────┘
```

---

## Persistence

All audio events become **permanent knowledge**:

```cpp
// After processing events
atomic_graph.save("nodes_audio.bin", "edges_audio.bin");

// Later session
atomic_graph.load("nodes_audio.bin", "edges_audio.bin");
// All audio associations restored!
```

**File Format:**
- `nodes_audio.bin`: Binary serialized nodes (40 bytes each)
- `edges_audio.bin`: Binary serialized edges (24 bytes each)

---

## Performance Characteristics

### Memory Usage

| Component | Memory per Item | Example (1 hour) |
|-----------|----------------|------------------|
| Node | 40 bytes | ~10K nodes = 400 KB |
| Edge | 24 bytes | ~50K edges = 1.2 MB |
| Audio Buffer | 2 bytes/sample | 3s @ 16kHz = 96 KB |
| Total | - | ~2 MB |

### Latency

| Operation | Typical Latency |
|-----------|----------------|
| Frame processing | <1 ms |
| Speech recognition (Whisper) | 100-500 ms |
| Ambient classification | 50-200 ms |
| Graph integration | <5 ms |
| Cross-modal sync | <10 ms |

### Throughput

- **Real-time audio:** 16 kHz, mono, 32-bit float
- **Event rate:** ~1-10 events/second (speech + ambient)
- **Graph updates:** ~100-500 nodes/edges per minute

---

## Configuration Guide

### Quick Start (Default Config)

```cpp
AudioPipeline audio;  // Uses defaults
AudioBridge bridge;   // Uses defaults
```

### Low Latency (Fast Response)

```cpp
AudioPipeline::Config config;
config.buffer_size_ms = 1000;      // 1s buffer
config.vad_threshold = 0.01f;      // Sensitive VAD
config.silence_duration = 0.3f;    // Quick cutoff
AudioPipeline audio(config);
```

### High Accuracy (Better Recognition)

```cpp
AudioPipeline::Config config;
config.buffer_size_ms = 5000;      // 5s buffer
config.vad_threshold = 0.03f;      // Stricter VAD
config.silence_duration = 0.8f;    // Wait longer
config.recognition_engine = "whisper";  // Best quality
AudioPipeline audio(config);
```

### Multi-Modal Learning

```cpp
AudioBridge::Config config;
config.temporal_window = 5.0f;     // Wider sync window
config.enable_cross_modal = true;
config.enable_causal_inference = true;
config.edge_decay_rate = 0.995f;   // Slower decay
AudioBridge bridge(config);
```

---

## Future Extensions

### 1. Speaker Identification
```cpp
AudioEvent {
    ...
    speaker_id = "user_1",  // Distinguish user vs Melvin
    emotion = "happy",      // Emotional tone
}
```

### 2. Sound Localization
```cpp
AudioEvent {
    ...
    location = {x: 2.5, y: 1.0},  // Spatial position
    direction = 45.0f,             // Degrees
}
```

### 3. Continuous Streaming
```cpp
audio.set_mode(AudioPipeline::CONTINUOUS);  // Always listening
audio.set_wake_word("hey melvin");          // Activation phrase
```

### 4. Multi-Language Support
```cpp
config.recognition_language = "es";  // Spanish
config.recognition_language = "zh";  // Chinese
```

---

## Troubleshooting

### No audio events generated
- Check microphone permissions
- Verify `audio.is_running()` returns true
- Lower `vad_threshold` for quiet environments
- Check `audio.get_current_volume()` > 0

### Speech not recognized
- Ensure speech recognition engine installed
- Check logs for recognition errors
- Increase `buffer_size_ms` for longer phrases
- Verify `config.enable_speech = true`

### Cross-modal links not created
- Check `temporal_window` is appropriate
- Verify events have similar timestamps
- Ensure `config.enable_cross_modal = true`
- Check event confidence thresholds

---

## References

- **Whisper:** OpenAI's speech recognition model
- **Vosk:** Lightweight offline speech recognition
- **YAMNet:** Google's audio classification CNN
- **PortAudio:** Cross-platform audio I/O library

---

**Last Updated:** October 16, 2025
**Version:** 1.0
**Author:** Melvin AI Development Team

