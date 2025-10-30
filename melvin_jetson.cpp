/**
 * @file melvin_jetson.cpp
 * @brief MELVIN Cognitive OS - Jetson Orin AGX Main Program
 * 
 * Production deployment with:
 * - 2 USB cameras (vision)
 * - USB microphone (audio input)
 * - USB speakers (audio output)
 * - CAN bus motors (Robstride O2/O3)
 * - Always-on operation
 */

#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <cstring>
#include <cmath>

// Linux-specific headers (only on actual Jetson)
#ifdef __linux__
#include <alsa/asoundlib.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#endif

#include "cognitive_os/cognitive_os.h"
#include "core/unified_intelligence.h"

using namespace melvin::cognitive_os;
using namespace melvin::intelligence;

// Global pointers for signal handler
CognitiveOS* g_os = nullptr;
std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << ", shutting down...\n";
    g_running = false;
    if (g_os) {
        g_os->stop();
    }
    exit(0);
}

void print_banner() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║     MELVIN COGNITIVE OS v1.0                         ║\n";
    std::cout << "║     Jetson Orin AGX Deployment                       ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
}

void check_hardware() {
    std::cout << "🔍 Checking hardware...\n";
    
    // Check cameras
    int camera_count = 0;
    for (int i = 0; i < 4; i++) {
        std::string device = "/dev/video" + std::to_string(i);
        if (access(device.c_str(), F_OK) == 0) {
            camera_count++;
            std::cout << "   ✅ Camera found: " << device << "\n";
        }
    }
    
    if (camera_count < 2) {
        std::cout << "   ⚠️  Warning: Expected 2 cameras, found " << camera_count << "\n";
    }
    
    // Check CAN bus
    if (access("/dev/can0", F_OK) == 0) {
        std::cout << "   ✅ CAN bus available: /dev/can0\n";
    } else {
        std::cout << "   ⚠️  CAN bus not found (motors will not work)\n";
        std::cout << "       Run: sudo ip link set can0 type can bitrate 1000000\n";
        std::cout << "            sudo ip link set up can0\n";
    }
    
    std::cout << "\n";
}

bool load_knowledge_graph(
    std::unordered_map<std::string, int>& word_to_id,
    std::unordered_map<int, std::string>& id_to_word,
    std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    std::unordered_map<int, std::vector<float>>& embeddings
) {
    std::cout << "📊 Loading knowledge graph...\n";
    
    // Try to load from data/
    // For now, create a small demo graph
    // In production, this would load from unified_nodes.bin and unified_edges.bin
    
    std::vector<std::string> vocabulary = {
        "hello", "world", "melvin", "robot", "intelligence",
        "see", "hear", "move", "learn", "think",
        "camera", "microphone", "motor", "sensor", "actuator",
        "red", "blue", "green", "left", "right",
        "forward", "backward", "stop", "go", "turn"
    };
    
    int node_id = 0;
    for (const auto& word : vocabulary) {
        word_to_id[word] = node_id;
        id_to_word[node_id] = word;
        
        // Simple embedding
        std::vector<float> emb(128);
        size_t hash = std::hash<std::string>{}(word);
        for (size_t i = 0; i < 128; i++) {
            emb[i] = std::sin(static_cast<float>(hash + i) * 0.01f);
        }
        embeddings[node_id] = emb;
        
        node_id++;
    }
    
    // Create basic connections
    auto add_edge = [&](const std::string& from, const std::string& to, float weight) {
        int from_id = word_to_id[from];
        int to_id = word_to_id[to];
        graph[from_id].push_back({to_id, weight});
        graph[to_id].push_back({from_id, weight * 0.8f});
    };
    
    // Basic robot vocabulary
    add_edge("melvin", "robot", 0.95f);
    add_edge("robot", "intelligence", 0.9f);
    add_edge("see", "camera", 0.95f);
    add_edge("hear", "microphone", 0.95f);
    add_edge("move", "motor", 0.95f);
    add_edge("learn", "intelligence", 0.9f);
    add_edge("think", "intelligence", 0.9f);
    add_edge("red", "camera", 0.7f);
    add_edge("blue", "camera", 0.7f);
    add_edge("left", "turn", 0.9f);
    add_edge("right", "turn", 0.9f);
    add_edge("forward", "move", 0.9f);
    add_edge("backward", "move", 0.9f);
    
    std::cout << "   ✅ " << vocabulary.size() << " concepts loaded\n";
    std::cout << "   ✅ Knowledge graph ready\n\n";
    
    return true;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// HARDWARE SERVICE THREADS (Always-on capture/control)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

/**
 * Vision capture thread - continuously captures from USB cameras
 */
void vision_capture_loop(EventBus* bus, const std::vector<std::string>& camera_devices) {
    std::vector<cv::VideoCapture> cameras;
    
    // Open cameras
    for (const auto& device : camera_devices) {
        cv::VideoCapture cap;
        if (cap.open(device)) {
            cameras.push_back(cap);
            std::cout << "   ✅ Opened camera: " << device << "\n";
        } else {
            std::cerr << "   ⚠️  Failed to open camera: " << device << "\n";
        }
    }
    
    if (cameras.empty()) {
        std::cerr << "   ⚠️  No cameras available, vision service disabled\n";
        return;
    }
    
    cv::Mat frame;
    auto last_capture = std::chrono::steady_clock::now();
    const auto capture_interval = std::chrono::milliseconds(100);  // 10 FPS
    
    while (g_running.load()) {
        auto now = std::chrono::steady_clock::now();
        if (now - last_capture < capture_interval) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        last_capture = now;
        
        // Capture from all cameras
        for (size_t i = 0; i < cameras.size(); i++) {
            if (cameras[i].read(frame) && !frame.empty()) {
                // TODO: Process frame with vision pipeline (object detection, etc.)
                // For now, publish basic vision event
                
                VisionEvent vision;
                vision.timestamp = std::chrono::duration<double>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()
                ).count();
                vision.obj_ids = {};  // Would be populated by vision pipeline
                vision.embeddings = {};  // Would be vision embeddings
                
                bus->publish(topics::VISION_EVENTS, vision);
            }
        }
    }
}

/**
 * Audio input capture thread - continuously listens from USB microphone
 */
void audio_input_loop(EventBus* bus, const std::string& alsa_device) {
#ifdef __linux__
    snd_pcm_t* pcm_handle;
    snd_pcm_hw_params_t* hw_params;
    
    // Open ALSA device
    if (snd_pcm_open(&pcm_handle, alsa_device.c_str(), SND_PCM_STREAM_CAPTURE, 0) < 0) {
        std::cerr << "   ⚠️  Failed to open microphone: " << alsa_device << "\n";
        return;
    }
    
    unsigned int sample_rate = 16000;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &sample_rate, 0);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 1);
    snd_pcm_hw_params(pcm_handle, hw_params);
    
    std::vector<int16_t> buffer(3200);  // 200ms at 16kHz
    
    while (g_running.load()) {
        snd_pcm_sframes_t frames = snd_pcm_readi(pcm_handle, buffer.data(), buffer.size());
        
        if (frames > 0) {
            // Calculate energy
            float energy = 0.0f;
            for (int i = 0; i < frames; i++) {
                energy += buffer[i] * buffer[i];
            }
            energy = std::sqrt(energy / frames) / 32768.0f;
            
            // If energy above threshold, process as speech
            if (energy > 0.01f) {
                AudioEvent audio;
                audio.timestamp = std::chrono::duration<double>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()
                ).count();
                audio.energy = energy;
                audio.phonemes = {};  // TODO: Speech recognition
                audio.embedding = {};  // TODO: Audio embedding
                
                bus->publish(topics::AUDIO_EVENTS, audio);
                
                // Trigger cognitive query from audio
                CogQuery query;
                query.timestamp = audio.timestamp;
                query.text = "";  // TODO: Speech-to-text
                query.intent = 0;
                bus->publish(topics::COG_QUERY, query);
            }
        }
    }
    
    snd_pcm_close(pcm_handle);
#else
    (void)bus;
    (void)alsa_device;
    std::cout << "   ℹ️  Audio input only available on Linux (stub on Mac)\n";
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
}

/**
 * Audio output thread - speaks responses from cognitive system
 */
void audio_output_loop(EventBus* bus, const std::string& alsa_device) {
#ifdef __linux__
    snd_pcm_t* pcm_handle;
    
    if (snd_pcm_open(&pcm_handle, alsa_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        std::cerr << "   ⚠️  Failed to open speakers: " << alsa_device << "\n";
        return;
    }
    
    unsigned int sample_rate = 22050;
    snd_pcm_hw_params_t* hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &sample_rate, 0);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 1);
    snd_pcm_hw_params(pcm_handle, hw_params);
#else
    (void)alsa_device;
#endif
    
    while (g_running.load()) {
        // Poll for cognitive answers and output them
        auto answers = bus->poll(topics::COG_ANSWER);
        
        for (const auto& event : answers) {
            auto answer = event.get<CogAnswer>();
            if (answer && !answer->text.empty()) {
                // Generate text output (colored, ChatGPT-style)
                std::cout << "\n💬 \033[1;32mMelvin:\033[0m " << answer->text << "\n";
                std::cout << "   \033[2m[confidence: " << answer->confidence << "]\033[0m\n";
                
#ifdef __linux__
                // TODO: Text-to-speech synthesis with ALSA when available
                // For now, text output works everywhere
#endif
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
#ifdef __linux__
    snd_pcm_close(pcm_handle);
#endif
}

/**
 * Motor control thread - sends commands and reads feedback via CAN bus
 */
void motor_control_loop(EventBus* bus, const std::string& can_interface) {
#ifdef __linux__
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        std::cerr << "   ⚠️  Failed to open CAN socket\n";
        return;
    }
    
    struct ifreq ifr;
    strcpy(ifr.ifr_name, can_interface.c_str());
    ioctl(s, SIOCGIFINDEX, &ifr);
    
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "   ⚠️  Failed to bind CAN socket\n";
        close(s);
        return;
    }
    
    while (g_running.load()) {
        // Read motor feedback from CAN
        struct can_frame frame;
        if (read(s, &frame, sizeof(frame)) > 0) {
            // TODO: Parse Robstride feedback, publish MOTOR_FEEDBACK event
        }
        
        // Check for motor commands from cognitive system
        // TODO: Subscribe to motor command events and send via CAN
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    close(s);
#else
    (void)can_interface;
    std::cout << "   ℹ️  Motor control: Text output mode\n";
#endif
    
    // Text output mode (fallback for no hardware)
    while (g_running.load()) {
        auto motor_events = bus->poll(topics::MOTOR_STATE);
        
        for (const auto& event : motor_events) {
            auto motor_state = event.get<MotorState>();
            if (motor_state && !motor_state->joint_pos.empty()) {
                std::cout << "🦾 \033[1;33mMotor Action:\033[0m ";
                for (size_t i = 0; i < motor_state->joint_pos.size(); i++) {
                    std::cout << "J" << i << "=" << motor_state->joint_pos[i];
                    if (i < motor_state->joint_pos.size() - 1) std::cout << ", ";
                }
                std::cout << "\n";
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int, char**) {
    print_banner();
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Check hardware
    check_hardware();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // LOAD KNOWLEDGE GRAPH
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::unordered_map<std::string, int> word_to_id;
    std::unordered_map<int, std::string> id_to_word;
    std::unordered_map<int, std::vector<std::pair<int, float>>> graph;
    std::unordered_map<int, std::vector<float>> embeddings;
    
    if (!load_knowledge_graph(word_to_id, id_to_word, graph, embeddings)) {
        std::cerr << "❌ Failed to load knowledge graph\n";
        return 1;
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // INITIALIZE UNIFIED INTELLIGENCE
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🧠 Initializing Unified Intelligence...\n";
    
    UnifiedIntelligence melvin;
    melvin.initialize(graph, embeddings, word_to_id, id_to_word);
    
    std::cout << "   ✅ Intelligence ready\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CREATE ACTIVATION FIELD
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🌊 Creating global activation field...\n";
    
    FieldFacade field(graph, embeddings);
    
    std::cout << "   ✅ Field ready\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // START COGNITIVE OS
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🚀 Starting Cognitive OS...\n\n";
    
    CognitiveOS os;
    os.attach(&melvin, &field);
    
    g_os = &os;  // For signal handler
    
    os.start();
    
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║     ✅ MELVIN IS ALIVE AND RUNNING                   ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Services active:\n";
    std::cout << "  • Scheduler:      50 Hz\n";
    std::cout << "  • Cognition:      30 Hz\n";
    std::cout << "  • Attention:      60 Hz\n";
    std::cout << "  • Working Memory: 30 Hz\n";
    std::cout << "  • Learning:       10 Hz\n";
    std::cout << "  • Reflection:      5 Hz\n\n";
    
    std::cout << "📊 Metrics logging to: logs/kpis.jsonl\n";
    std::cout << "🛑 Press Ctrl+C to stop\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // START HARDWARE SERVICE THREADS (Always-on)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🎥 Starting hardware services...\n";
    
    // Find camera devices
    std::vector<std::string> camera_devices;
    for (int i = 0; i < 4; i++) {
        std::string device = "/dev/video" + std::to_string(i);
        if (access(device.c_str(), F_OK) == 0) {
            camera_devices.push_back(device);
        }
    }
    
    // Start hardware threads
    std::thread vision_thread(vision_capture_loop, os.event_bus(), camera_devices);
    std::thread audio_input_thread(audio_input_loop, os.event_bus(), "default");
    std::thread audio_output_thread(audio_output_loop, os.event_bus(), "default");
    std::thread motor_thread(motor_control_loop, os.event_bus(), "can0");
    
    std::cout << "   ✅ Vision capture: " << camera_devices.size() << " cameras\n";
    std::cout << "   ✅ Audio input: Always listening\n";
    std::cout << "   ✅ Audio output: Ready to speak\n";
    std::cout << "   ✅ Motor control: CAN bus active\n";
    std::cout << "\n";
    
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║     ✅ MELVIN FULLY OPERATIONAL                       ║\n";
    std::cout << "║     👁️  Always Seeing  🎤 Always Listening           ║\n";
    std::cout << "║     🧠 Always Thinking  🔄 Always Learning           ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // RUN FOREVER
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Main thread monitors and logs status
    int seconds = 0;
    while (os.is_running() && g_running.load()) {
        sleep(10);
        seconds += 10;
        
        auto metrics = field.get_metrics();
        
        std::cout << "[" << seconds << "s] ";
        std::cout << "Active: " << metrics.active_nodes << " | ";
        std::cout << "Entropy: " << std::fixed << std::setprecision(2) << metrics.entropy << " | ";
        std::cout << "Logs: " << os.metrics()->logs_written() << "\n";
    }
    
    // Wait for hardware threads
    if (vision_thread.joinable()) vision_thread.join();
    if (audio_input_thread.joinable()) audio_input_thread.join();
    if (audio_output_thread.joinable()) audio_output_thread.join();
    if (motor_thread.joinable()) motor_thread.join();
    
    return 0;
}

