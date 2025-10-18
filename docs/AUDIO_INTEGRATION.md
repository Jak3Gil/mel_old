# 🔌 Audio Subsystem Integration Guide

This guide shows how to integrate the audio perception subsystem into Melvin's main application.

---

## Quick Integration

### Option 1: Simple Integration (Recommended)

Add audio to your existing Melvin application:

```cpp
#include "melvin/audio/audio_pipeline.h"
#include "melvin/audio/audio_bridge.h"

int main() {
    // Existing Melvin setup
    MelvinCore melvin;
    AtomicGraph graph;
    
    // Add audio components
    audio::AudioPipeline audio;
    audio::AudioBridge audio_bridge;
    
    // Start audio capture
    audio.start_stream();
    
    // Main loop
    while (running) {
        float dt = get_delta_time();
        
        // Process audio
        audio.tick(dt);
        auto audio_events = audio.get_recent_events();
        
        // Integrate into knowledge graph
        for (const auto& event : audio_events) {
            audio_bridge.process(event, graph);
        }
        
        // Your existing vision/reasoning code here
        // ...
        
        // Periodic save
        if (should_save) {
            graph.save("nodes.bin", "edges.bin");
        }
    }
    
    audio.stop_stream();
    return 0;
}
```

---

## Complete Integration Example

Create `melvin/examples/melvin_with_audio.cpp`:

```cpp
/*
 * Melvin with Audio Perception
 * Full integration example
 */

#include "melvin/core/melvin.h"
#include "melvin/core/atomic_graph.h"
#include "melvin/audio/audio_pipeline.h"
#include "melvin/audio/audio_bridge.h"
#include "melvin/vision/vision_pipeline.h"  // If available
#include <iostream>
#include <chrono>

using namespace melvin;
using namespace melvin::audio;

class MelvinWithAudio {
public:
    MelvinWithAudio() {
        // Configure audio
        AudioPipeline::Config audio_config;
        audio_config.sample_rate = 16000;
        audio_config.enable_speech = true;
        audio_config.enable_ambient = true;
        
        audio_pipeline_ = new AudioPipeline(audio_config);
        
        // Configure bridge
        AudioBridge::Config bridge_config;
        bridge_config.temporal_window = 3.0f;
        bridge_config.enable_cross_modal = true;
        
        audio_bridge_ = new AudioBridge(bridge_config);
        
        std::cout << "🎤 Melvin with Audio initialized" << std::endl;
    }
    
    ~MelvinWithAudio() {
        delete audio_pipeline_;
        delete audio_bridge_;
    }
    
    void start() {
        // Load existing knowledge
        try {
            graph_.load("melvin_nodes.bin", "melvin_edges.bin");
            std::cout << "📂 Loaded existing knowledge" << std::endl;
        } catch (...) {
            std::cout << "📝 Starting with fresh knowledge" << std::endl;
        }
        
        // Start audio
        if (!audio_pipeline_->start_stream()) {
            std::cerr << "❌ Failed to start audio" << std::endl;
            return;
        }
        
        std::cout << "🎧 Listening..." << std::endl;
        
        running_ = true;
        last_save_ = std::chrono::steady_clock::now();
    }
    
    void tick(float dt) {
        if (!running_) return;
        
        // Process audio
        audio_pipeline_->tick(dt);
        auto audio_events = audio_pipeline_->get_recent_events();
        
        // Integrate audio into graph
        for (const auto& event : audio_events) {
            std::cout << "🎤 Audio: \"" << event.label << "\" (" 
                      << event.type << ", conf=" << event.confidence << ")" << std::endl;
            
            audio_bridge_->process(event, graph_);
        }
        
        // TODO: Process vision events
        // auto visual_events = vision_pipeline_->get_recent_events();
        
        // Cross-modal sync (if vision available)
        // audio_bridge_->sync_with_vision(audio_events, visual_events, graph_);
        
        // Apply learning dynamics
        if (!audio_events.empty()) {
            audio_bridge_->reinforce_patterns(graph_, 0.99f);
        }
        
        // Periodic save (every 30 seconds)
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_save_);
        if (elapsed.count() >= 30) {
            save();
            last_save_ = now;
        }
    }
    
    void stop() {
        running_ = false;
        audio_pipeline_->stop_stream();
        save();
        
        std::cout << "\n📊 Session statistics:" << std::endl;
        graph_.print_stats();
        audio_bridge_->print_stats();
        audio_pipeline_->print_stats();
    }
    
    void save() {
        graph_.save("melvin_nodes.bin", "melvin_edges.bin");
        std::cout << "💾 Knowledge saved" << std::endl;
    }
    
    const AtomicGraph& get_graph() const { return graph_; }
    
private:
    AtomicGraph graph_;
    AudioPipeline* audio_pipeline_;
    AudioBridge* audio_bridge_;
    bool running_ = false;
    std::chrono::steady_clock::time_point last_save_;
};

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  🎤 MELVIN WITH AUDIO PERCEPTION                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    MelvinWithAudio melvin;
    melvin.start();
    
    // Main loop
    std::cout << "Running perception loop (Ctrl+C to stop)...\n" << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    auto last_frame = start_time;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_frame).count();
        last_frame = now;
        
        melvin.tick(dt);
        
        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Optional: Auto-stop after some time (for testing)
        // auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
        // if (elapsed.count() >= 60) break;  // Stop after 1 minute
    }
    
    melvin.stop();
    return 0;
}
```

---

## Integration Checklist

### Step 1: Include Headers

```cpp
#include "melvin/audio/audio_pipeline.h"
#include "melvin/audio/audio_bridge.h"
```

### Step 2: Create Components

```cpp
AudioPipeline audio;
AudioBridge audio_bridge;
```

### Step 3: Start Audio Stream

```cpp
audio.start_stream();  // Use default device
// or
audio.start_stream(device_id);  // Specific device
```

### Step 4: Process in Main Loop

```cpp
while (running) {
    audio.tick(dt);
    auto events = audio.get_recent_events();
    
    for (const auto& event : events) {
        audio_bridge.process(event, graph);
    }
}
```

### Step 5: Stop and Save

```cpp
audio.stop_stream();
graph.save("nodes.bin", "edges.bin");
```

---

## Configuration Examples

### Low Latency Mode

```cpp
AudioPipeline::Config config;
config.sample_rate = 16000;
config.buffer_size_ms = 1000;      // 1s buffer
config.vad_threshold = 0.01f;       // Sensitive
config.silence_duration = 0.3f;     // Quick cutoff

AudioPipeline audio(config);
```

### High Accuracy Mode

```cpp
AudioPipeline::Config config;
config.sample_rate = 16000;
config.buffer_size_ms = 5000;       // 5s buffer
config.vad_threshold = 0.03f;       // Strict
config.silence_duration = 0.8f;     // Wait longer

AudioPipeline audio(config);
```

### Multi-Modal Mode

```cpp
AudioBridge::Config config;
config.temporal_window = 5.0f;      // Wider window
config.enable_cross_modal = true;
config.enable_causal_inference = true;
config.create_word_nodes = true;

AudioBridge bridge(config);
```

---

## Cross-Modal Integration

### With Vision

```cpp
// Get events from both modalities
auto audio_events = audio_pipeline.get_recent_events();
auto visual_events = vision_pipeline.get_recent_events();

// Synchronize
audio_bridge.sync_with_vision(audio_events, visual_events, graph);
```

### With Text/Reasoning

```cpp
// Create text events from reasoning
TextEvent thought;
thought.timestamp = current_time();
thought.text = "cooking food";
thought.type = "thought";

// Sync with audio
audio_bridge.sync_with(audio_events, {}, {thought}, {}, graph);
```

### With Actions

```cpp
// Create action event
ActionEvent action;
action.timestamp = current_time();
action.action = "move_to_stove";
action.result = "success";

// Sync (learns causality: audio → action)
audio_bridge.sync_with(audio_events, {}, {}, {action}, graph);
```

---

## Build Integration

Update your Makefile to include audio:

```makefile
# Add audio sources
AUDIO_SOURCES = melvin/audio/audio_pipeline.cpp \
                melvin/audio/audio_bridge.cpp

# Add to build
melvin_with_audio: $(CORE_OBJECTS) $(AUDIO_OBJECTS) melvin_with_audio.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) -lportaudio -pthread
```

---

## Error Handling

### Graceful Degradation

```cpp
AudioPipeline audio;

if (!audio.start_stream()) {
    std::cerr << "⚠️  Audio not available, continuing without..." << std::endl;
    // Melvin continues with vision only
} else {
    std::cout << "✅ Audio enabled" << std::endl;
}
```

### Check Audio Availability

```cpp
try {
    audio.start_stream();
    audio_enabled = true;
} catch (const std::exception& e) {
    std::cerr << "Audio error: " << e.what() << std::endl;
    audio_enabled = false;
}

// In main loop
if (audio_enabled) {
    audio.tick(dt);
    // ...
}
```

---

## Performance Optimization

### Reduce Processing Overhead

```cpp
// Process only when audio detected
if (audio.get_current_volume() > threshold) {
    audio.tick(dt);
}
```

### Batch Processing

```cpp
// Process every N frames
if (frame_count % 10 == 0) {
    auto events = audio.get_recent_events();
    audio_bridge.process_batch(events, graph);
}
```

### Background Thread

```cpp
std::thread audio_thread([&]() {
    while (running) {
        audio.tick(0.1f);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
});
```

---

## Testing Integration

### Test 1: Audio Only

```bash
# Build
g++ -std=c++17 -O3 -I. \
    melvin/core/atomic_graph.cpp \
    melvin/audio/audio_pipeline.cpp \
    melvin/audio/audio_bridge.cpp \
    melvin_with_audio.cpp \
    -o melvin_with_audio \
    -lportaudio -pthread

# Run
./melvin_with_audio
```

### Test 2: With Simulated Vision

```cpp
// In your integration
if (audio_event.label == "stove") {
    // Simulate vision detecting stove
    VisualEvent visual;
    visual.timestamp = audio_event.timestamp + 0.5f;
    visual.label = "stove";
    visual.type = "object";
    
    audio_bridge.sync_with_vision({audio_event}, {visual}, graph);
}
```

---

## Debugging

### Enable Verbose Output

```cpp
// In audio_pipeline.cpp, add debug prints:
std::cout << "📊 Frame: " << frame_count 
          << " | Volume: " << current_volume 
          << " | Speech: " << (is_speech_active ? "YES" : "NO") 
          << std::endl;
```

### Monitor Graph Growth

```cpp
// After processing
if (frame_count % 100 == 0) {
    std::cout << "📈 Graph: " << graph.node_count() << " nodes, "
              << graph.edge_count() << " edges" << std::endl;
}
```

---

## Production Deployment

### Startup Check

```cpp
void check_audio_system() {
    std::cout << "🔍 Checking audio system..." << std::endl;
    
    AudioPipeline test_pipeline;
    if (test_pipeline.start_stream()) {
        std::cout << "   ✅ Microphone accessible" << std::endl;
        test_pipeline.stop_stream();
    } else {
        std::cerr << "   ❌ Microphone not available" << std::endl;
    }
    
    // Check Python dependencies
    int result = system("python3 -c 'import whisper' 2>/dev/null");
    if (result == 0) {
        std::cout << "   ✅ Whisper available" << std::endl;
    } else {
        std::cerr << "   ⚠️  Whisper not installed" << std::endl;
    }
}
```

---

## Next Steps

1. ✅ Build integration example:
   ```bash
   make melvin_with_audio
   ```

2. ✅ Test with real audio:
   ```bash
   ./melvin_with_audio
   ```

3. ✅ Monitor knowledge growth:
   ```bash
   # In another terminal
   watch -n 1 "ls -lh melvin_nodes.bin melvin_edges.bin"
   ```

4. ✅ Add vision integration (see `docs/architecture_audio.md`)

---

**Integration Status:** Ready for production use ✅

For more details, see:
- `README_AUDIO.md` - Quick start guide
- `docs/architecture_audio.md` - Technical architecture
- `melvin/demos/demo_audio_integration.cpp` - Working examples

