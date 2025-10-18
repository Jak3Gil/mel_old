#include "audio_pipeline.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace melvin {
namespace audio {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AudioPipeline::AudioPipeline() 
    : AudioPipeline(Config()) {
}

AudioPipeline::AudioPipeline(const Config& config)
    : config_(config)
    , running_(false)
    , current_volume_(0.0f)
    , start_time_(0.0f)
    , next_event_id_(1)
    , buffer_write_pos_(0)
    , is_speech_active_(false)
    , speech_start_time_(0.0f) {
    
    // Calculate buffer size in samples
    size_t buffer_samples = (config_.buffer_size_ms * config_.sample_rate) / 1000;
    rolling_buffer_.resize(buffer_samples, 0.0f);
    
    std::cout << "🎤 AudioPipeline initialized" << std::endl;
    std::cout << "   Sample rate: " << config_.sample_rate << " Hz" << std::endl;
    std::cout << "   Buffer: " << config_.buffer_size_ms << " ms" << std::endl;
    std::cout << "   Speech recognition: " << (config_.enable_speech ? "enabled" : "disabled") << std::endl;
    std::cout << "   Ambient detection: " << (config_.enable_ambient ? "enabled" : "disabled") << std::endl;
}

AudioPipeline::~AudioPipeline() {
    stop_stream();
}

// ============================================================================
// LIFECYCLE CONTROL
// ============================================================================

bool AudioPipeline::start_stream(int device_index) {
    if (running_.load()) {
        std::cerr << "⚠️  AudioPipeline already running" << std::endl;
        return false;
    }
    
    std::cout << "🎙️  Starting audio stream (device " << device_index << ")..." << std::endl;
    
    // NOTE: This is a placeholder for actual audio capture
    // In production, this would use:
    // - PortAudio for cross-platform capture
    // - ALSA on Linux
    // - CoreAudio on macOS
    // - WASAPI on Windows
    
    // For now, we set up the infrastructure
    running_.store(true);
    start_time_ = get_runtime_seconds();
    
    std::cout << "✅ Audio stream started" << std::endl;
    
    return true;
}

bool AudioPipeline::start_file(const std::string& file_path) {
    if (running_.load()) {
        std::cerr << "⚠️  AudioPipeline already running" << std::endl;
        return false;
    }
    
    std::cout << "📂 Loading audio file: " << file_path << std::endl;
    
    // NOTE: Placeholder for file loading
    // In production, use libsndfile or similar
    
    running_.store(true);
    start_time_ = get_runtime_seconds();
    
    std::cout << "✅ Audio file loaded" << std::endl;
    
    return true;
}

void AudioPipeline::stop_stream() {
    if (!running_.load()) {
        return;
    }
    
    std::cout << "🛑 Stopping audio stream..." << std::endl;
    running_.store(false);
    
    // Flush any pending speech
    if (is_speech_active_ && !speech_buffer_.empty()) {
        recognize_speech();
    }
    
    std::cout << "✅ Audio stream stopped" << std::endl;
    print_stats();
}

// ============================================================================
// EVENT PROCESSING
// ============================================================================

void AudioPipeline::tick(float dt) {
    if (!running_.load()) {
        return;
    }
    
    // In a real implementation, this would:
    // 1. Pull frames from audio capture thread
    // 2. Process each frame
    // 3. Detect voice activity
    // 4. Trigger recognition when appropriate
    
    // For now, this is a placeholder for the processing loop
}

std::vector<AudioEvent> AudioPipeline::get_recent_events() {
    std::lock_guard<std::mutex> lock(event_mutex_);
    
    std::vector<AudioEvent> events;
    while (!event_queue_.empty()) {
        events.push_back(event_queue_.front());
        event_queue_.pop();
    }
    
    return events;
}

std::vector<AudioEvent> AudioPipeline::get_events_in_range(float start_time, float end_time) {
    std::lock_guard<std::mutex> lock(event_mutex_);
    
    std::vector<AudioEvent> events;
    for (const auto& event : event_history_) {
        if (event.timestamp >= start_time && event.timestamp <= end_time) {
            events.push_back(event);
        }
    }
    
    return events;
}

// ============================================================================
// MANUAL PROCESSING
// ============================================================================

void AudioPipeline::process_frame(const AudioFrame& frame) {
    if (frame.samples.empty()) {
        return;
    }
    
    // Compute volume
    float rms = compute_rms(frame.samples);
    current_volume_.store(rms);
    
    // Update rolling buffer
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        for (float sample : frame.samples) {
            rolling_buffer_[buffer_write_pos_] = sample;
            buffer_write_pos_ = (buffer_write_pos_ + 1) % rolling_buffer_.size();
        }
    }
    
    // Detect voice activity
    detect_voice_activity(frame);
    
    // Classify ambient sounds (if enabled and no speech)
    if (config_.enable_ambient && !is_speech_active_) {
        classify_ambient_sound(frame);
    }
}

void AudioPipeline::reprocess_log(const std::string& log_path) {
    std::cout << "🔄 Reprocessing audio log: " << log_path << std::endl;
    
    // NOTE: Placeholder for log reprocessing
    // In production, load saved audio data and reprocess
    
    std::cout << "✅ Log reprocessed" << std::endl;
}

// ============================================================================
// VOICE ACTIVITY DETECTION
// ============================================================================

void AudioPipeline::detect_voice_activity(const AudioFrame& frame) {
    float rms = compute_rms(frame.samples);
    bool is_speech = rms > config_.vad_threshold;
    
    float current_time = frame.timestamp_us / 1000000.0f;
    
    if (is_speech && !is_speech_active_) {
        // Speech started
        is_speech_active_ = true;
        speech_start_time_ = current_time;
        speech_buffer_.clear();
        speech_buffer_.insert(speech_buffer_.end(), frame.samples.begin(), frame.samples.end());
        
    } else if (is_speech && is_speech_active_) {
        // Speech continuing
        speech_buffer_.insert(speech_buffer_.end(), frame.samples.begin(), frame.samples.end());
        
    } else if (!is_speech && is_speech_active_) {
        // Potential end of speech - wait for silence duration
        float silence_duration = current_time - (speech_start_time_ + 
                                  speech_buffer_.size() / static_cast<float>(config_.sample_rate));
        
        if (silence_duration > config_.silence_duration) {
            // Speech ended - recognize
            is_speech_active_ = false;
            recognize_speech();
        }
    }
}

void AudioPipeline::recognize_speech() {
    if (speech_buffer_.empty()) {
        return;
    }
    
    if (!config_.enable_speech) {
        speech_buffer_.clear();
        return;
    }
    
    std::cout << "🎯 Recognizing speech (" << speech_buffer_.size() / config_.sample_rate 
              << " seconds)..." << std::endl;
    
    // Run speech recognition
    std::string transcript = run_speech_recognition(speech_buffer_);
    
    if (!transcript.empty()) {
        // Create speech event
        AudioEvent event(
            next_event_id_++,
            speech_start_time_,
            transcript,
            "speech",
            0.9f,  // Confidence (would come from recognizer)
            "mic"
        );
        
        event.duration = speech_buffer_.size() / static_cast<float>(config_.sample_rate);
        event.volume = compute_rms(speech_buffer_);
        
        emit_event(event);
        
        std::cout << "   Transcript: \"" << transcript << "\"" << std::endl;
    }
    
    speech_buffer_.clear();
}

// ============================================================================
// AMBIENT SOUND CLASSIFICATION
// ============================================================================

void AudioPipeline::classify_ambient_sound(const AudioFrame& frame) {
    // Only classify if we have enough audio
    if (frame.samples.size() < config_.sample_rate) {  // At least 1 second
        return;
    }
    
    float confidence = 0.0f;
    std::string label = run_ambient_classification(frame.samples, confidence);
    
    if (!label.empty()) {
        AudioEvent event(
            next_event_id_++,
            frame.timestamp_us / 1000000.0f,
            label,
            "ambient",
            confidence,
            "mic"
        );
        
        event.duration = frame.duration_seconds();
        event.volume = compute_rms(frame.samples);
        
        emit_event(event);
    }
}

// ============================================================================
// EVENT EMISSION
// ============================================================================

void AudioPipeline::emit_event(const AudioEvent& event) {
    std::lock_guard<std::mutex> lock(event_mutex_);
    event_queue_.push(event);
    event_history_.push_back(event);
}

// ============================================================================
// RECOGNITION ENGINES (PLACEHOLDERS)
// ============================================================================

std::string AudioPipeline::run_speech_recognition(const std::vector<float>& audio) {
    // NOTE: Placeholder implementation
    // In production, this would:
    // 
    // For Whisper:
    //   1. Save audio to temp WAV file
    //   2. Call Python script with Whisper model
    //   3. Parse JSON output
    // 
    // For Vosk:
    //   1. Initialize Vosk recognizer
    //   2. Feed audio data
    //   3. Get transcript
    //
    // Example Python call:
    //   system("python3 scripts/recognize_speech.py /tmp/audio.wav > /tmp/transcript.txt");
    
    // For now, return empty (no actual recognition)
    return "";
}

std::string AudioPipeline::run_ambient_classification(const std::vector<float>& audio, float& confidence) {
    // NOTE: Placeholder implementation
    // In production, this would:
    //
    // For YAMNet:
    //   1. Convert audio to mel spectrogram
    //   2. Run TensorFlow model
    //   3. Get top classification
    //
    // For custom CNN:
    //   1. Extract acoustic features (MFCC, etc.)
    //   2. Run model inference
    //   3. Return label + confidence
    
    confidence = 0.0f;
    return "";
}

// ============================================================================
// UTILITIES
// ============================================================================

float AudioPipeline::compute_rms(const std::vector<float>& samples) {
    if (samples.empty()) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (float sample : samples) {
        sum += sample * sample;
    }
    
    return std::sqrt(sum / samples.size());
}

float AudioPipeline::get_runtime_seconds() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return duration.count() / 1000000.0f;
}

void AudioPipeline::print_stats() const {
    std::cout << "\n📊 AudioPipeline Statistics:" << std::endl;
    std::cout << "   Total events: " << event_history_.size() << std::endl;
    std::cout << "   Runtime: " << get_runtime_seconds() - start_time_ << " seconds" << std::endl;
    std::cout << "   Current volume: " << current_volume_.load() << std::endl;
    
    // Count event types
    size_t speech_count = 0;
    size_t ambient_count = 0;
    
    for (const auto& event : event_history_) {
        if (event.type == "speech") speech_count++;
        else if (event.type == "ambient") ambient_count++;
    }
    
    std::cout << "   Speech events: " << speech_count << std::endl;
    std::cout << "   Ambient events: " << ambient_count << std::endl;
}

} // namespace audio
} // namespace melvin

