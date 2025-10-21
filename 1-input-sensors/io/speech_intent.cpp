#include "speech_intent.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace melvin {
namespace io {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

SpeechIntent::SpeechIntent(AtomicGraph& graph)
    : graph_(graph) {
    
    // Create or get Melvin's agent node
    melvin_agent_id_ = graph_.get_or_create_node("melvin_agent", SPEAKER_AGENT);
    
    std::cout << "🧠 SpeechIntent initialized (Melvin's cognitive speech)" << std::endl;
}

SpeechIntent::~SpeechIntent() {
}

// ============================================================================
// SPEECH OUTPUT PROCESSING
// ============================================================================

uint64_t SpeechIntent::process_output(const std::string& text, uint64_t concept_root) {
    std::cout << "\n🎙️ Processing speech output: \"" << text << "\"" << std::endl;
    
    // Create utterance node for the complete phrase
    std::string speech_id = generate_speech_id();
    uint64_t utterance_id = graph_.get_or_create_node("utterance:" + text, UTTERANCE);
    
    // Create speech output node (represents the actual vocalization)
    uint64_t speech_node_id = graph_.get_or_create_node("speech:" + speech_id, SPEECH_OUTPUT);
    
    // Link utterance to speech output
    graph_.add_edge(utterance_id, speech_node_id, SPOKEN_AS, 1.0f);
    
    // Mark as self-produced
    graph_.add_edge(speech_node_id, melvin_agent_id_, SELF_PRODUCED, 1.0f);
    graph_.add_edge(utterance_id, melvin_agent_id_, UTTERED_BY, 1.0f);
    
    // Tokenize into words and create word nodes
    auto words = tokenize(text);
    uint64_t prev_word_id = 0;
    
    for (const auto& word : words) {
        // Create word node
        uint64_t word_id = graph_.get_or_create_node("spoken:" + word, SPOKEN_WORD);
        
        // Link word to utterance
        graph_.add_edge(word_id, utterance_id, Relation::INSTANCE_OF, 1.0f);
        
        // Get or create concept for this word
        std::string concept = get_concept(word);
        uint64_t concept_id = graph_.get_or_create_node(concept, 0);  // CONCEPT type
        
        // Link word to its meaning
        graph_.add_edge(word_id, concept_id, DERIVES_FROM, 1.0f);
        
        // Temporal chain (word sequence)
        if (prev_word_id > 0) {
            graph_.add_edge(prev_word_id, word_id, Relation::TEMPORAL_NEXT, 1.0f);
        }
        
        prev_word_id = word_id;
        word_count_++;
    }
    
    // Link to concept root if provided
    if (concept_root > 0) {
        graph_.add_edge(utterance_id, concept_root, DERIVES_FROM, 2.0f);
        std::cout << "   ↳ Linked to concept node " << concept_root << std::endl;
    }
    
    // Link to previous speech (temporal continuity)
    if (last_speech_id_ > 0) {
        graph_.add_edge(last_speech_id_, speech_node_id, Relation::TEMPORAL_NEXT, 1.0f);
    }
    
    // Update tracking
    last_speech_id_ = speech_node_id;
    speech_count_++;
    
    // Record for self-recognition
    recent_speech_.push_back({
        speech_node_id,
        text,
        get_current_timestamp()
    });
    
    // Cleanup old records
    cleanup_old_records(get_current_timestamp());
    
    std::cout << "   ✅ Created speech graph:" << std::endl;
    std::cout << "      Utterance: " << utterance_id << std::endl;
    std::cout << "      Speech: " << speech_node_id << std::endl;
    std::cout << "      Words: " << words.size() << std::endl;
    
    return speech_node_id;
}

uint64_t SpeechIntent::process_with_cause(const std::string& text, uint64_t cause_node) {
    uint64_t speech_id = process_output(text, 0);
    
    // Create explicit causal link
    graph_.add_edge(cause_node, speech_id, Relation::TEMPORAL_NEXT, 1.5f);
    
    std::cout << "   🔗 Linked to cause node " << cause_node << std::endl;
    
    return speech_id;
}

void SpeechIntent::mark_spoken(uint64_t speech_id, const std::string& audio_file) {
    // Mark as actually vocalized
    uint64_t status_node = graph_.get_or_create_node("vocalized", 0);
    graph_.add_edge(speech_id, status_node, Relation::OBSERVED_AS, 1.0f);
    
    if (!audio_file.empty()) {
        uint64_t audio_node = graph_.get_or_create_node("audio_file:" + audio_file, 2);
        graph_.add_edge(speech_id, audio_node, SPOKEN_AS, 1.0f);
    }
}

// ============================================================================
// SELF-RECOGNITION
// ============================================================================

bool SpeechIntent::is_self_speech(const std::string& audio_label, float timestamp) {
    // Check recent speech for matches
    for (const auto& record : recent_speech_) {
        // Check time proximity (within 5 seconds)
        if (std::abs(timestamp - record.timestamp) < 5.0f) {
            // Check text similarity (simple substring match)
            std::string lower_audio = audio_label;
            std::string lower_record = record.text;
            
            std::transform(lower_audio.begin(), lower_audio.end(), lower_audio.begin(), ::tolower);
            std::transform(lower_record.begin(), lower_record.end(), lower_record.begin(), ::tolower);
            
            if (lower_record.find(lower_audio) != std::string::npos ||
                lower_audio.find(lower_record) != std::string::npos) {
                
                std::cout << "   🔍 Self-recognition: \"" << audio_label 
                          << "\" matches recent speech" << std::endl;
                return true;
            }
        }
    }
    
    return false;
}

void SpeechIntent::link_self_recognition(uint64_t speech_id, uint64_t audio_id) {
    // Create bidirectional link between output and input
    graph_.add_edge(speech_id, audio_id, HEARD_AS, 1.0f);
    graph_.add_edge(audio_id, speech_id, DERIVES_FROM, 1.0f);
    
    // Mark both as self-produced
    graph_.add_edge(audio_id, melvin_agent_id_, SELF_PRODUCED, 1.0f);
    
    self_recognition_count_++;
    
    std::cout << "   🔁 Self-recognition link created: speech(" << speech_id 
              << ") ↔ audio(" << audio_id << ")" << std::endl;
}

// ============================================================================
// REFLECTION & MEMORY
// ============================================================================

std::vector<uint64_t> SpeechIntent::get_recent_speech(float time_window) {
    std::vector<uint64_t> result;
    float current_time = get_current_timestamp();
    
    for (const auto& record : recent_speech_) {
        if (current_time - record.timestamp <= time_window) {
            result.push_back(record.speech_id);
        }
    }
    
    return result;
}

std::vector<uint64_t> SpeechIntent::find_speech_about(const std::string& concept_label) {
    std::vector<uint64_t> result;
    
    // Find concept node
    auto concept_nodes = graph_.find_nodes(concept_label);
    
    for (auto concept_id : concept_nodes) {
        // Find all speech nodes that derive from this concept
        for (const auto& record : recent_speech_) {
            // Check if this speech is linked to the concept
            if (graph_.get_edge_weight(record.speech_id, concept_id, DERIVES_FROM) > 0) {
                result.push_back(record.speech_id);
            }
        }
    }
    
    return result;
}

// ============================================================================
// STATISTICS
// ============================================================================

void SpeechIntent::print_stats() const {
    std::cout << "\n📊 SpeechIntent Statistics:" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "   Total utterances: " << speech_count_ << std::endl;
    std::cout << "   Total words spoken: " << word_count_ << std::endl;
    std::cout << "   Self-recognitions: " << self_recognition_count_ << std::endl;
    std::cout << "   Recent speech records: " << recent_speech_.size() << std::endl;
    std::cout << "   Last speech ID: " << last_speech_id_ << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
}

// ============================================================================
// HELPER METHODS
// ============================================================================

std::vector<std::string> SpeechIntent::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string word;
    
    while (iss >> word) {
        // Remove punctuation
        word.erase(std::remove_if(word.begin(), word.end(), 
                   [](char c) { return !std::isalnum(c) && c != '\'' && c != '-'; }), 
                   word.end());
        
        if (!word.empty()) {
            // Convert to lowercase
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            tokens.push_back(word);
        }
    }
    
    return tokens;
}

std::string SpeechIntent::get_concept(const std::string& word) {
    // Simple concept extraction (can be enhanced with WordNet, etc.)
    // For now, just use the word itself as the concept
    return "concept:" + word;
}

std::string SpeechIntent::generate_speech_id() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    ).count();
    
    std::ostringstream oss;
    oss << "speech_" << timestamp;
    return oss.str();
}

float SpeechIntent::get_current_timestamp() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    );
    return duration.count() / 1000000.0f;
}

void SpeechIntent::cleanup_old_records(float current_time) {
    // Remove records older than 30 seconds
    const float max_age = 30.0f;
    
    recent_speech_.erase(
        std::remove_if(recent_speech_.begin(), recent_speech_.end(),
                      [current_time, max_age](const SpeechRecord& r) {
                          return current_time - r.timestamp > max_age;
                      }),
        recent_speech_.end()
    );
}

} // namespace io
} // namespace melvin

