#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

namespace melvin {
namespace audio {

// ============================================================================
// AUDIO EVENT STRUCTURES
// ============================================================================

/**
 * AudioEvent - Base structure for all sound-related detections
 */
struct AudioEvent {
    uint64_t id;                // Unique event identifier
    float timestamp;            // Time in seconds since pipeline start
    std::string label;          // e.g., "turn on the stove", "dog barking"
    std::string type;           // "speech", "ambient", "system", "music"
    float confidence;           // 0.0 to 1.0
    std::string source;         // "mic", "file", "reflection"
    
    // Optional metadata
    float volume;               // 0.0 to 1.0 (RMS amplitude)
    float duration;             // Length of sound event in seconds
    std::vector<float> features; // Optional acoustic features
    
    AudioEvent() 
        : id(0), timestamp(0.0f), confidence(0.0f), 
          volume(0.0f), duration(0.0f) {}
    
    AudioEvent(uint64_t event_id, float ts, const std::string& lbl, 
               const std::string& t, float conf, const std::string& src)
        : id(event_id), timestamp(ts), label(lbl), type(t), 
          confidence(conf), source(src), volume(0.0f), duration(0.0f) {}
};

// ============================================================================
// AUDIO FRAME STRUCTURE
// ============================================================================

/**
 * AudioFrame - Raw audio data buffer
 */
struct AudioFrame {
    std::vector<float> samples;  // Normalized audio samples [-1.0, 1.0]
    uint64_t timestamp_us;       // Microsecond timestamp
    uint32_t sample_rate;        // Samples per second (e.g., 16000)
    uint8_t channels;            // Number of audio channels (1=mono, 2=stereo)
    
    AudioFrame() : timestamp_us(0), sample_rate(16000), channels(1) {}
    
    size_t frame_size() const { return samples.size(); }
    float duration_seconds() const { 
        return sample_rate > 0 ? static_cast<float>(samples.size()) / sample_rate : 0.0f; 
    }
};

// ============================================================================
// AUDIO PIPELINE CLASS
// ============================================================================

/**
 * AudioPipeline - Captures, processes, and analyzes audio input
 * 
 * Responsibilities:
 * - Capture live microphone input or load from file
 * - Maintain rolling audio buffer
 * - Detect voice activity (VAD)
 * - Recognize speech (via Whisper/Vosk)
 * - Classify ambient sounds (via YAMNet/CNN)
 * - Generate AudioEvent objects
 * 
 * Design:
 * - Non-blocking: Audio capture runs in background thread
 * - Event-based: Outputs discrete events to queue
 * - Modular: Easy to swap recognition engines
 */
class AudioPipeline {
public:
    /**
     * Configuration for audio pipeline
     */
    struct Config {
        uint32_t sample_rate = 16000;     // Hz (Whisper uses 16kHz)
        uint8_t channels = 1;             // Mono by default
        size_t buffer_size_ms = 3000;     // 3 seconds rolling buffer
        float vad_threshold = 0.02f;      // Voice activity detection threshold
        float silence_duration = 0.5f;    // Seconds of silence to split utterances
        bool enable_speech = true;        // Enable speech recognition
        bool enable_ambient = true;       // Enable ambient sound classification
        std::string recognition_engine = "whisper";  // "whisper", "vosk", "none"
        std::string ambient_model = "yamnet";        // "yamnet", "none"
    };
    
    AudioPipeline();
    explicit AudioPipeline(const Config& config);
    ~AudioPipeline();
    
    // ========================================================================
    // LIFECYCLE CONTROL
    // ========================================================================
    
    /**
     * Start audio capture from microphone
     * @param device_index Audio device index (-1 for default)
     * @return true if successful
     */
    bool start_stream(int device_index = -1);
    
    /**
     * Start audio processing from file
     * @param file_path Path to audio file
     * @return true if successful
     */
    bool start_file(const std::string& file_path);
    
    /**
     * Stop audio capture and processing
     */
    void stop_stream();
    
    /**
     * Check if pipeline is running
     */
    bool is_running() const { return running_.load(); }
    
    // ========================================================================
    // EVENT PROCESSING
    // ========================================================================
    
    /**
     * Process audio (call this in main loop)
     * @param dt Delta time in seconds
     */
    void tick(float dt);
    
    /**
     * Get all events since last call
     * @return Vector of new audio events
     */
    std::vector<AudioEvent> get_recent_events();
    
    /**
     * Get events in time range
     * @param start_time Start timestamp
     * @param end_time End timestamp
     * @return Vector of audio events in range
     */
    std::vector<AudioEvent> get_events_in_range(float start_time, float end_time);
    
    // ========================================================================
    // MANUAL PROCESSING
    // ========================================================================
    
    /**
     * Process a single audio frame
     * @param frame Audio frame to process
     */
    void process_frame(const AudioFrame& frame);
    
    /**
     * Load and reprocess audio log for reflection
     * @param log_path Path to audio log file
     */
    void reprocess_log(const std::string& log_path);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    float get_current_volume() const { return current_volume_.load(); }
    size_t get_event_count() const { return event_history_.size(); }
    float get_runtime_seconds() const;
    
    void print_stats() const;

private:
    // Configuration
    Config config_;
    
    // State
    std::atomic<bool> running_;
    std::atomic<float> current_volume_;
    float start_time_;
    uint64_t next_event_id_;
    
    // Audio buffer
    std::vector<float> rolling_buffer_;
    size_t buffer_write_pos_;
    std::mutex buffer_mutex_;
    
    // Event queue
    std::queue<AudioEvent> event_queue_;
    std::vector<AudioEvent> event_history_;
    std::mutex event_mutex_;
    
    // Voice Activity Detection
    bool is_speech_active_;
    float speech_start_time_;
    std::vector<float> speech_buffer_;
    
    // Private methods
    void detect_voice_activity(const AudioFrame& frame);
    void recognize_speech();
    void classify_ambient_sound(const AudioFrame& frame);
    void emit_event(const AudioEvent& event);
    float compute_rms(const std::vector<float>& samples);
    
    // Placeholder methods for recognition engines
    // These will call external libraries or Python scripts
    std::string run_speech_recognition(const std::vector<float>& audio);
    std::string run_ambient_classification(const std::vector<float>& audio, float& confidence);
};

} // namespace audio
} // namespace melvin

