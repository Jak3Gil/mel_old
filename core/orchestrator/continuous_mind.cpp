#include "continuous_mind.h"
#include <iostream>
#include <fstream>

namespace melvin {
namespace orchestrator {

ContinuousMind::ContinuousMind()
    : field_(),
      feedback_(field_),
      reflection_(field_, feedback_),
      turn_taking_(),
      emotion_modulator_(),
      goal_stack_(),
      running_(false),
      mode_(Mode::THINKING),
      last_tick_(std::chrono::high_resolution_clock::now()),
      last_perception_tick_(std::chrono::high_resolution_clock::now()),
      last_attention_tick_(std::chrono::high_resolution_clock::now()),
      last_reasoning_tick_(std::chrono::high_resolution_clock::now()),
      last_output_tick_(std::chrono::high_resolution_clock::now()),
      last_feedback_tick_(std::chrono::high_resolution_clock::now()),
      last_reflection_tick_(std::chrono::high_resolution_clock::now()),
      last_evolution_tick_(std::chrono::high_resolution_clock::now()),
      last_predicted_output_(-1),
      self_monitoring_gain_(0.2f) {
    
    // Apply genome to conversational components
    auto& genome_params = genome_.reasoning_params();
    genome_params.apply_to_turn_taking(&turn_taking_);
    genome_params.apply_to_emotional_modulator(&emotion_modulator_);
    genome_params.apply_to_goal_stack(&goal_stack_);
    
    std::cout << "🧠 Continuous Mind initialized" << std::endl;
    std::cout << "   Subsystem rates:" << std::endl;
    std::cout << "   - Perception: " << perception_rate_ << " Hz" << std::endl;
    std::cout << "   - Attention: " << attention_rate_ << " Hz" << std::endl;
    std::cout << "   - Reasoning: " << reasoning_rate_ << " Hz" << std::endl;
    std::cout << "   - Output: " << output_rate_ << " Hz" << std::endl;
    std::cout << "   - Feedback: " << feedback_rate_ << " Hz" << std::endl;
    std::cout << "   - Reflection: " << reflection_rate_ << " Hz" << std::endl;
    std::cout << "   - Evolution: " << evolution_rate_ << " Hz" << std::endl;
    std::cout << "   🗣️  Conversational components enabled (genome-configured)" << std::endl;
    std::cout << "      - Theta frequency: " << genome_params.theta_frequency << " Hz" << std::endl;
    std::cout << "      - Base tempo: " << genome_params.base_tempo << "x" << std::endl;
    std::cout << "      - Goal decay rate: " << genome_params.goal_decay_rate << std::endl;
}

ContinuousMind::~ContinuousMind() {
    stop();
}

void ContinuousMind::start() {
    if (running_.load()) {
        std::cout << "⚠️  Mind already running" << std::endl;
        return;
    }
    
    running_.store(true);
    mind_thread_ = std::make_unique<std::thread>(&ContinuousMind::run, this);
    
    std::cout << "✅ Continuous Mind started" << std::endl;
}

void ContinuousMind::stop() {
    if (!running_.load()) return;
    
    std::cout << "🛑 Stopping mind..." << std::endl;
    running_.store(false);
    
    if (mind_thread_ && mind_thread_->joinable()) {
        mind_thread_->join();
    }
    
    std::cout << "✅ Mind stopped" << std::endl;
}

void ContinuousMind::run() {
    std::cout << "🔄 Mind loop running at " << target_tick_rate_ << " Hz" << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t tick_count = 0;
    
    while (running_.load()) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_tick_).count();
        last_tick_ = now;
        
        // Run all subsystems asynchronously based on their own rates
        
        // Perception (30 Hz)
        float perception_dt = std::chrono::duration<float>(now - last_perception_tick_).count();
        if (perception_dt >= 1.0f / perception_rate_) {
            update_perception(perception_dt);
            last_perception_tick_ = now;
        }
        
        // Attention (10 Hz)
        float attention_dt = std::chrono::duration<float>(now - last_attention_tick_).count();
        if (attention_dt >= 1.0f / attention_rate_) {
            update_attention(attention_dt);
            last_attention_tick_ = now;
        }
        
        // Reasoning (20 Hz) - Core field dynamics
        float reasoning_dt = std::chrono::duration<float>(now - last_reasoning_tick_).count();
        if (reasoning_dt >= 1.0f / reasoning_rate_) {
            update_reasoning(reasoning_dt);
            last_reasoning_tick_ = now;
        }
        
        // Output (10 Hz)
        float output_dt = std::chrono::duration<float>(now - last_output_tick_).count();
        if (output_dt >= 1.0f / output_rate_) {
            update_output(output_dt);
            last_output_tick_ = now;
        }
        
        // Feedback (10 Hz)
        float feedback_dt = std::chrono::duration<float>(now - last_feedback_tick_).count();
        if (feedback_dt >= 1.0f / feedback_rate_) {
            update_feedback(feedback_dt);
            last_feedback_tick_ = now;
        }
        
        // Reflection (1 Hz)
        float reflection_dt = std::chrono::duration<float>(now - last_reflection_tick_).count();
        if (reflection_dt >= 1.0f / reflection_rate_) {
            update_reflection(reflection_dt);
            last_reflection_tick_ = now;
        }
        
        // Evolution (0.01 Hz - every 100 seconds)
        float evolution_dt = std::chrono::duration<float>(now - last_evolution_tick_).count();
        if (evolution_dt >= 1.0f / evolution_rate_) {
            update_evolution(evolution_dt);
            last_evolution_tick_ = now;
        }
        
        // Turn-taking controller (10 Hz - theta rhythm)
        update_turn_taking(dt);
        
        // Self-monitoring (continuous)
        update_self_monitoring(dt);
        
        // Compute actual tick rate
        tick_count++;
        if (tick_count % 100 == 0) {
            auto elapsed = std::chrono::duration<float>(now - start_time).count();
            actual_tick_rate_ = tick_count / elapsed;
        }
        
        // Sleep to maintain target rate
        auto tick_end = std::chrono::high_resolution_clock::now();
        float tick_duration = std::chrono::duration<float>(tick_end - now).count();
        float target_duration = 1.0f / target_tick_rate_;
        
        if (tick_duration < target_duration) {
            std::this_thread::sleep_for(
                std::chrono::microseconds(
                    static_cast<int>((target_duration - tick_duration) * 1000000)));
        }
    }
    
    std::cout << "🛑 Mind loop exited" << std::endl;
}

void ContinuousMind::update_perception(float dt) {
    // Only process input if listening
    if (turn_taking_.should_listen() || mode_.load() == Mode::LISTENING) {
        process_input_buffer();
    }
}

void ContinuousMind::update_attention(float dt) {
    // Update working context and attention weights
    // Working context automatically decays in field.tick()
    
    // Get current working concepts
    auto active_nodes = field_.get_working_context().get_active_nodes(7);
    
    // Bias field activations toward attended concepts
    for (int node_id : active_nodes) {
        float current = field_.get_activation(node_id);
        field_.set_activation(node_id, current * 1.1f);  // Slight boost
    }
}

void ContinuousMind::update_reasoning(float dt) {
    // Core field dynamics: decay, spread, Hopfield
    field_.tick(dt);
}

void ContinuousMind::update_output(float dt) {
    // Generate outputs based on mode AND turn-taking state
    bool should_output = (mode_.load() == Mode::ACTING || mode_.load() == Mode::THINKING);
    bool has_turn = turn_taking_.should_speak();
    
    if (should_output && has_turn) {
        generate_outputs();
    }
}

void ContinuousMind::update_feedback(float dt) {
    feedback_.process_feedback(dt);
}

void ContinuousMind::update_reflection(float dt) {
    reflection_.tick(dt);
    
    // Apply reflections to genome periodically
    static int reflection_apply_counter = 0;
    if (++reflection_apply_counter >= 100) {  // Every 100 reflection ticks
        reflection_.apply_reflections_to_genome();
        reflection_apply_counter = 0;
    }
}

void ContinuousMind::update_evolution(float dt) {
    // Evolution is handled by feedback coordinator
    // This just logs the event
    auto stats = feedback_.get_stats();
    std::cout << "🧬 Evolution step - Generation " << stats.evolution_generation 
              << ", Fitness: " << stats.current_fitness << std::endl;
}

void ContinuousMind::process_input_buffer() {
    std::vector<SensoryInput> inputs;
    {
        std::lock_guard<std::mutex> lock(input_mutex_);
        inputs = input_buffer_;
        input_buffer_.clear();
    }
    
    std::vector<int> input_nodes;
    std::string input_text;
    
    for (const auto& input : inputs) {
        switch (input.type) {
            case SensoryInput::Type::VISION:
            case SensoryInput::Type::AUDIO:
                {
                    // Generate node ID from hash of features
                    int node_id = std::hash<std::string>{}(
                        std::string(reinterpret_cast<const char*>(input.data.data()),
                                  input.data.size() * sizeof(float))) % 1000000;
                    
                    input_nodes.push_back(node_id);
                    
                    float energy = 10.0f;  // TODO: Get from genome
                    field_.inject_energy(node_id, energy, input.data);
                    
                    // Post sensory feedback message
                    fields::FieldMessage msg(
                        fields::MessageType::SENSORY_INPUT,
                        0, node_id, energy);
                    msg.data = input.data;
                    field_.post_message(msg);
                }
                break;
                
            case SensoryInput::Type::TEXT:
                input_text = input.text_data;
                // TODO: Tokenize and inject
                break;
                
            case SensoryInput::Type::MOTOR:
                // TODO: Inject motor feedback
                break;
        }
    }
    
    // Update conversation goal stack with new input
    if (!input_nodes.empty()) {
        goal_stack_.update_from_input(input_nodes, input_text);
    }
}

void ContinuousMind::generate_outputs() {
    // Get high-energy nodes
    auto active_concepts = get_active_concepts();
    
    if (active_concepts.empty()) return;
    
    // Get context from conversation goals
    auto context_nodes = goal_stack_.get_context_nodes(20);
    active_concepts.insert(active_concepts.end(), context_nodes.begin(), context_nodes.end());
    
    // Compute emotional state
    float field_energy = field_.get_total_energy();
    // TODO: Get actual metrics from field
    float confidence = 0.7f;  // Placeholder
    float novelty = 0.3f;
    float coherence = 0.8f;
    
    // Store for stats
    last_confidence_ = confidence;
    last_novelty_ = novelty;
    last_coherence_ = coherence;
    
    // Create output
    Output output;
    output.type = Output::Type::TEXT;
    output.concept_ids = active_concepts;
    output.timestamp = std::chrono::high_resolution_clock::now();
    output.confidence = confidence;
    output.novelty = novelty;
    
    // Generate with emotional modulation
    output.text = generate_conversational_response(
        active_concepts, confidence, novelty, coherence
    );
    
    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        output_buffer_.push_back(output);
        output_history_.push_back(output);
        if (output_history_.size() > 50) {
            output_history_.pop_front();
        }
    }
    
    // Self-monitoring: process output feedback
    process_output_feedback(output);
}

void ContinuousMind::configure_for_mode() {
    switch (mode_.load()) {
        case Mode::LISTENING:
            perception_rate_ = 60.0f;    // High sensory intake
            attention_rate_ = 20.0f;
            reasoning_rate_ = 30.0f;
            output_rate_ = 1.0f;         // Low output
            reflection_rate_ = 0.1f;     // Low reflection
            break;
            
        case Mode::THINKING:
            perception_rate_ = 10.0f;
            attention_rate_ = 20.0f;
            reasoning_rate_ = 30.0f;     // High reasoning
            output_rate_ = 5.0f;
            reflection_rate_ = 1.0f;     // High reflection
            break;
            
        case Mode::ACTING:
            perception_rate_ = 30.0f;
            attention_rate_ = 10.0f;
            reasoning_rate_ = 20.0f;
            output_rate_ = 30.0f;        // High output
            reflection_rate_ = 0.1f;
            break;
            
        case Mode::SLEEPING:
            perception_rate_ = 0.0f;     // No input
            attention_rate_ = 0.0f;
            reasoning_rate_ = 5.0f;      // Slow dynamics
            output_rate_ = 0.0f;         // No output
            reflection_rate_ = 5.0f;     // High reflection/consolidation
            break;
    }
}

void ContinuousMind::set_mode(Mode mode) {
    mode_.store(mode);
    configure_for_mode();
    
    std::cout << "🔄 Mode changed to: ";
    switch (mode) {
        case Mode::LISTENING: std::cout << "LISTENING"; break;
        case Mode::THINKING: std::cout << "THINKING"; break;
        case Mode::ACTING: std::cout << "ACTING"; break;
        case Mode::SLEEPING: std::cout << "SLEEPING"; break;
    }
    std::cout << std::endl;
}

void ContinuousMind::inject_vision(const std::vector<float>& visual_features) {
    SensoryInput input;
    input.type = SensoryInput::Type::VISION;
    input.data = visual_features;
    input.timestamp = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(input_mutex_);
    input_buffer_.push_back(input);
}

void ContinuousMind::inject_audio(const std::vector<float>& audio_features) {
    SensoryInput input;
    input.type = SensoryInput::Type::AUDIO;
    input.data = audio_features;
    input.timestamp = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(input_mutex_);
    input_buffer_.push_back(input);
}

void ContinuousMind::inject_text(const std::string& text) {
    SensoryInput input;
    input.type = SensoryInput::Type::TEXT;
    input.text_data = text;
    input.timestamp = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(input_mutex_);
    input_buffer_.push_back(input);
}

void ContinuousMind::inject_motor_feedback(int motor_id, float position, float reward) {
    SensoryInput input;
    input.type = SensoryInput::Type::MOTOR;
    input.motor_id = motor_id;
    input.data = {position, reward};
    input.timestamp = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(input_mutex_);
    input_buffer_.push_back(input);
    
    // Also add to sensory feedback
    feedback::SensoryFeedback::SensoryEvent event(
        feedback::SensoryFeedback::SensoryEvent::Modality::MOTOR,
        input.data, reward);
    feedback_.sensory().add_event(event);
}

// ============================================================
// AUDIO LEARNING (Phase 6)
// ============================================================

void ContinuousMind::learn_from_user_speech(
    const std::string& transcribed_text,
    const std::vector<float>& whisper_embedding,
    const std::vector<float>& mel_features
) {
    // 1. Process text normally to activate semantic concepts
    inject_text(transcribed_text);
    
    // 2. Give the field a moment to activate (in real system, this happens asynchronously)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // 3. Get activated concept IDs
    auto active_concepts = field_.get_working_context().get_active_nodes(20);
    std::vector<uint64_t> concept_ids;
    for (int node : active_concepts) {
        concept_ids.push_back(static_cast<uint64_t>(node));
    }
    
    // 4. Learn audio-semantic association
    audio_layer_.learn_from_speech_input(
        transcribed_text,
        whisper_embedding,
        mel_features,
        concept_ids,
        1.0f  // temporal_proximity = 1.0 (simultaneous)
    );
    
    std::cout << "🎙️  Learned from user speech: \"" << transcribed_text << "\" → " 
              << concept_ids.size() << " concepts" << std::endl;
}

void ContinuousMind::learn_from_own_speech(
    const std::string& spoken_text,
    const std::vector<float>& mel_features
) {
    // 1. Get currently active concepts (what triggered this response)
    auto active_concepts = field_.get_working_context().get_active_nodes(20);
    std::vector<uint64_t> concept_ids;
    for (int node : active_concepts) {
        concept_ids.push_back(static_cast<uint64_t>(node));
    }
    
    // 2. Learn audio-semantic association from output
    audio_layer_.learn_from_speech_output(
        spoken_text,
        mel_features,
        concept_ids,
        1.0f  // temporal_proximity = 1.0
    );
    
    std::cout << "🔊 Learned from own speech: \"" << spoken_text << "\" → " 
              << concept_ids.size() << " concepts" << std::endl;
}

std::vector<int> ContinuousMind::get_active_concepts() {
    return field_.get_working_context().get_active_nodes(10);
}

std::string ContinuousMind::generate_text_output(int max_tokens) {
    auto active = get_active_concepts();
    
    std::string output = "Active concepts: ";
    for (size_t i = 0; i < std::min((size_t)max_tokens, active.size()); ++i) {
        output += std::to_string(active[i]) + " ";
    }
    
    return output;
}

std::vector<float> ContinuousMind::generate_audio_output(float duration_sec) {
    // TODO: Implement audio generation from active concepts
    return std::vector<float>();
}

std::vector<float> ContinuousMind::generate_motor_command(int motor_id) {
    // TODO: Implement motor command generation
    return std::vector<float>();
}

ContinuousMind::Stats ContinuousMind::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    Stats stats;
    stats.total_nodes = total_nodes_;
    stats.total_edges = total_edges_;
    stats.active_nodes = field_.get_active_node_count();
    stats.total_energy = field_.get_total_energy();
    stats.coherence = field_.get_coherence();
    
    auto feedback_stats = feedback_.get_stats();
    stats.avg_prediction_error = feedback_stats.avg_prediction_error;
    stats.avg_reward = feedback_stats.avg_sensory_reward;
    stats.evolution_generation = feedback_stats.evolution_generation;
    
    auto reflection_stats = reflection_.get_stats();
    stats.reflections_count = reflection_stats.total_reflections;
    
    stats.current_mode = mode_.load();
    stats.tick_rate = actual_tick_rate_;
    
    // Conversational stats
    stats.is_speaking = turn_taking_.should_speak();
    stats.is_listening = turn_taking_.should_listen();
    stats.current_topic = goal_stack_.get_current_topic();
    
    auto goal_stats = goal_stack_.get_stats();
    stats.active_goals = goal_stats.active_goals;
    
    stats.emotional_confidence = last_confidence_;
    stats.emotional_novelty = last_novelty_;
    
    // Audio learning stats (Phase 6)
    auto audio_stats = audio_layer_.get_stats();
    stats.audio_nodes_learned = audio_stats.total_audio_nodes;
    stats.audio_semantic_links = audio_stats.total_associations;
    stats.audio_learning_confidence = audio_stats.average_confidence;
    
    // Check if we can self-generate audio (need at least some concepts active)
    auto active_concept_ids = field_.get_working_context().get_active_nodes(10);
    std::vector<uint64_t> concept_ids_u64;
    for (int id : active_concept_ids) {
        concept_ids_u64.push_back(static_cast<uint64_t>(id));
    }
    stats.can_self_generate_audio = audio_layer_.can_self_generate_audio(concept_ids_u64);
    
    return stats;
}

void ContinuousMind::save_state(const std::string& path) {
    std::cout << "💾 Saving mind state to: " << path << std::endl;
    // TODO: Implement serialization
    std::cout << "⚠️  Save not yet implemented" << std::endl;
}

void ContinuousMind::load_state(const std::string& path) {
    std::cout << "📂 Loading mind state from: " << path << std::endl;
    // TODO: Implement deserialization
    std::cout << "⚠️  Load not yet implemented" << std::endl;
}

// ============================================================================
// CONVERSATIONAL METHODS (Phase 1-4 Implementation)
// ============================================================================

void ContinuousMind::update_turn_taking(float dt) {
    // Get field metrics for turn-taking decision
    float field_energy = field_.get_total_energy();
    // TODO: Compute actual gamma power from field variance
    float gamma_power = 0.5f;  // Placeholder
    
    // Update turn-taking state
    turn_taking_.update(dt, field_energy, gamma_power);
    
    // Sync mode with turn-taking state
    if (turn_taking_.should_speak() && mode_.load() == Mode::LISTENING) {
        // Natural transition to speaking/acting
        set_mode(Mode::ACTING);
    }
    else if (turn_taking_.should_listen() && mode_.load() == Mode::ACTING) {
        // Natural transition back to listening
        set_mode(Mode::LISTENING);
    }
}

void ContinuousMind::update_self_monitoring(float dt) {
    // Process output history for self-monitoring
    std::vector<Output> recent_outputs;
    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        if (output_history_.size() > 5) {
            recent_outputs.assign(
                output_history_.end() - 5,
                output_history_.end()
            );
        } else {
            recent_outputs.assign(
                output_history_.begin(),
                output_history_.end()
            );
        }
    }
    
    // Re-inject recent outputs at attenuated gain (corollary discharge)
    for (const auto& output : recent_outputs) {
        auto now = std::chrono::high_resolution_clock::now();
        auto age = std::chrono::duration<float>(now - output.timestamp).count();
        
        // Only monitor very recent outputs (< 2 seconds)
        if (age < 2.0f) {
            float age_factor = 1.0f - (age / 2.0f);
            float monitoring_strength = self_monitoring_gain_ * age_factor;
            
            for (int node_id : output.concept_ids) {
                // Weak re-injection for self-monitoring
                field_.inject_energy(node_id, 1.0f * monitoring_strength, std::vector<float>());
            }
        }
    }
}

void ContinuousMind::process_output_feedback(const Output& output) {
    // Phase 1: Self-monitoring feedback loop
    
    // Re-inject output nodes at attenuated gain
    for (int node_id : output.concept_ids) {
        field_.inject_energy(
            node_id,
            2.0f * self_monitoring_gain_,  // 20% of normal
            std::vector<float>()  // No embedding
        );
    }
    
    // Predict what we should have said vs what we actually said
    if (!output.concept_ids.empty() && last_predicted_output_ >= 0) {
        int actual_output = output.concept_ids[0];
        
        if (last_predicted_output_ != actual_output) {
            // Prediction error in own speech!
            std::cout << "   🔍 [Self-Monitor] Output prediction error: "
                      << "expected " << last_predicted_output_
                      << ", said " << actual_output << "\n";
            
            // Apply prediction error learning
            // TODO: Create and apply PredictionError structure
        }
    }
    
    // Store prediction for next cycle
    if (!output.concept_ids.empty()) {
        last_predicted_output_ = output.concept_ids[0];
    }
}

std::string ContinuousMind::generate_conversational_response(
    const std::vector<int>& context_nodes,
    float confidence,
    float novelty,
    float coherence
) {
    // Phase 3: Emotional modulation
    
    // Compute prosody parameters
    auto prosody = emotion_modulator_.compute_prosody(
        confidence, novelty, coherence
    );
    
    // Generate base answer (concept list for now)
    std::string base_answer = "Concepts: ";
    int count = 0;
    for (int node_id : context_nodes) {
        if (count >= 8) break;
        base_answer += std::to_string(node_id) + " ";
        count++;
    }
    
    // Add hedging based on confidence
    std::string modulated_answer = emotion_modulator_.add_hedging(
        base_answer,
        prosody.hedge_probability
    );
    
    // Add emotional tone
    modulated_answer = emotion_modulator_.add_emotional_tone(
        modulated_answer,
        confidence,
        novelty
    );
    
    // Log emotional state
    std::string emotion = emotion_modulator_.get_emotional_state(
        confidence, novelty, coherence
    );
    std::cout << "   😊 [Emotion] State: " << emotion
              << " (conf=" << confidence
              << ", nov=" << novelty << ")\n";
    
    return modulated_answer;
}

bool ContinuousMind::should_speak() const {
    return turn_taking_.should_speak();
}

bool ContinuousMind::should_listen() const {
    return turn_taking_.should_listen();
}

std::string ContinuousMind::get_current_topic() const {
    return goal_stack_.get_current_topic();
}

} // namespace orchestrator
} // namespace melvin

