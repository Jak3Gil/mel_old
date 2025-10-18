#include "audio_bridge.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace melvin {
namespace audio {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AudioBridge::AudioBridge()
    : AudioBridge(Config()) {
}

AudioBridge::AudioBridge(const Config& config)
    : config_(config)
    , events_processed_(0)
    , nodes_created_(0)
    , edges_created_(0)
    , cross_modal_links_(0) {
    
    std::cout << "🌉 AudioBridge initialized" << std::endl;
    std::cout << "   Temporal window: " << config_.temporal_window << " seconds" << std::endl;
    std::cout << "   Cross-modal linking: " << (config_.enable_cross_modal ? "enabled" : "disabled") << std::endl;
}

AudioBridge::~AudioBridge() {
}

// ============================================================================
// EVENT PROCESSING
// ============================================================================

std::vector<uint64_t> AudioBridge::process(const AudioEvent& event, AtomicGraph& graph) {
    std::vector<uint64_t> created_nodes;
    
    // Check confidence threshold
    if (event.confidence < config_.min_confidence) {
        return created_nodes;
    }
    
    // Route based on event type
    if (event.type == "speech") {
        process_speech_event(event, graph);
    } else if (event.type == "ambient") {
        process_ambient_event(event, graph);
    }
    
    events_processed_++;
    
    return created_nodes;
}

void AudioBridge::process_batch(const std::vector<AudioEvent>& events, AtomicGraph& graph) {
    for (const auto& event : events) {
        process(event, graph);
    }
}

// ============================================================================
// SPEECH EVENT PROCESSING
// ============================================================================

void AudioBridge::process_speech_event(const AudioEvent& event, AtomicGraph& graph) {
    // Create phrase node
    uint64_t phrase_node = 0;
    
    if (config_.create_phrase_nodes) {
        phrase_node = get_or_create_audio_node(event.label, AUDIO_PHRASE, graph);
        nodes_created_++;
        
        // Add metadata: mark as speech type
        uint64_t speech_category = get_or_create_audio_node("speech", AUDIO_CATEGORY, graph);
        graph.add_edge(phrase_node, speech_category, Relation::INSTANCE_OF, 1.0f);
        edges_created_++;
        
        // Track activation time
        node_last_activation_[phrase_node] = event.timestamp;
    }
    
    // Create individual word nodes
    if (config_.create_word_nodes && phrase_node > 0) {
        create_word_nodes(event.label, phrase_node, event.timestamp, graph);
    }
    
    std::cout << "🎤 Speech processed: \"" << event.label << "\" (node " << phrase_node << ")" << std::endl;
}

void AudioBridge::create_word_nodes(const std::string& phrase, uint64_t phrase_node, 
                                    float timestamp, AtomicGraph& graph) {
    // Simple word tokenization (split on spaces)
    std::istringstream iss(phrase);
    std::string word;
    uint64_t prev_word_node = 0;
    
    while (iss >> word) {
        // Convert to lowercase and remove punctuation
        std::string clean_word;
        for (char c : word) {
            if (std::isalnum(c)) {
                clean_word += std::tolower(c);
            }
        }
        
        if (clean_word.empty()) {
            continue;
        }
        
        // Create word node
        uint64_t word_node = get_or_create_audio_node(clean_word, AUDIO_WORD, graph);
        
        // Link word to phrase
        graph.add_edge(word_node, phrase_node, Relation::INSTANCE_OF, 1.0f);
        edges_created_++;
        
        // Link to previous word (sequence)
        if (prev_word_node > 0) {
            graph.add_edge(prev_word_node, word_node, Relation::TEMPORAL_NEXT, 1.0f);
            edges_created_++;
        }
        
        prev_word_node = word_node;
        node_last_activation_[word_node] = timestamp;
    }
}

// ============================================================================
// AMBIENT SOUND PROCESSING
// ============================================================================

void AudioBridge::process_ambient_event(const AudioEvent& event, AtomicGraph& graph) {
    // Create sound node
    uint64_t sound_node = get_or_create_audio_node(event.label, AUDIO_SOUND, graph);
    nodes_created_++;
    
    // Add metadata: mark as ambient sound
    uint64_t ambient_category = get_or_create_audio_node("ambient", AUDIO_CATEGORY, graph);
    graph.add_edge(sound_node, ambient_category, Relation::INSTANCE_OF, 1.0f);
    edges_created_++;
    
    // Track activation time
    node_last_activation_[sound_node] = event.timestamp;
    
    std::cout << "🔊 Ambient sound processed: \"" << event.label << "\" (node " << sound_node << ")" << std::endl;
}

// ============================================================================
// CROSS-MODAL SYNCHRONIZATION
// ============================================================================

void AudioBridge::sync_with(
    const std::vector<AudioEvent>& audio_events,
    const std::vector<VisualEvent>& visual_events,
    const std::vector<TextEvent>& text_events,
    const std::vector<ActionEvent>& action_events,
    AtomicGraph& graph) {
    
    if (!config_.enable_cross_modal) {
        return;
    }
    
    // Link audio with vision
    for (const auto& audio_ev : audio_events) {
        uint64_t audio_node = get_or_create_audio_node(audio_ev.label, AUDIO_PHRASE, graph);
        
        // Check vision events
        for (const auto& visual_ev : visual_events) {
            if (is_temporally_close(audio_ev.timestamp, visual_ev.timestamp)) {
                uint64_t visual_node = graph.get_or_create_node(visual_ev.label, 1);  // Instance type
                
                // Create co-occurrence edge
                graph.add_edge(audio_node, visual_node, Relation::CO_OCCURS_WITH, 
                              audio_ev.confidence * visual_ev.confidence);
                cross_modal_links_++;
                
                std::cout << "🔗 Cross-modal link: \"" << audio_ev.label 
                         << "\" ↔ \"" << visual_ev.label << "\"" << std::endl;
            }
        }
        
        // Check text events
        for (const auto& text_ev : text_events) {
            if (is_temporally_close(audio_ev.timestamp, text_ev.timestamp)) {
                uint64_t text_node = graph.get_or_create_node(text_ev.text, 0);  // Concept type
                graph.add_edge(audio_node, text_node, Relation::CO_OCCURS_WITH, 1.0f);
                cross_modal_links_++;
            }
        }
        
        // Check action events (causal inference)
        if (config_.enable_causal_inference) {
            for (const auto& action_ev : action_events) {
                if (action_ev.timestamp > audio_ev.timestamp && 
                    is_temporally_close(audio_ev.timestamp, action_ev.timestamp)) {
                    
                    uint64_t action_node = graph.get_or_create_node(action_ev.action, 1);
                    
                    // Infer causality: speech → action
                    if (action_ev.result == "success") {
                        // Add custom edge type (extend Relation enum if needed)
                        graph.add_edge(audio_node, action_node, Relation::TEMPORAL_NEXT, 1.5f);
                        
                        std::cout << "⚡ Causal link: \"" << audio_ev.label 
                                 << "\" → \"" << action_ev.action << "\"" << std::endl;
                    }
                }
            }
        }
    }
}

void AudioBridge::sync_with_vision(
    const std::vector<AudioEvent>& audio_events,
    const std::vector<VisualEvent>& visual_events,
    AtomicGraph& graph) {
    
    sync_with(audio_events, visual_events, {}, {}, graph);
}

// ============================================================================
// CAUSAL INFERENCE
// ============================================================================

void AudioBridge::infer_causality(AtomicGraph& graph) {
    // Analyze temporal patterns in the graph
    // Look for A → B sequences that repeat
    // Strengthen causal edges
    
    // NOTE: This is a placeholder for more sophisticated causal inference
    // In production, this would:
    // 1. Find temporal patterns
    // 2. Compute correlation scores
    // 3. Create/strengthen causal edges
    // 4. Remove spurious correlations
}

void AudioBridge::reinforce_patterns(AtomicGraph& graph, float decay_factor) {
    // Apply decay to all edges
    graph.decay_edges(decay_factor);
    
    // Reinforce recently co-activated nodes
    // (Nodes activated within temporal window get stronger connections)
    
    for (const auto& [node_a, time_a] : node_last_activation_) {
        for (const auto& [node_b, time_b] : node_last_activation_) {
            if (node_a >= node_b) continue;  // Avoid duplicates
            
            if (is_temporally_close(time_a, time_b)) {
                // Get existing edge weight
                float current_weight = graph.get_edge_weight(node_a, node_b, Relation::CO_OCCURS_WITH);
                
                // Reinforce if exists, otherwise create weak link
                if (current_weight > 0.0f) {
                    graph.add_edge(node_a, node_b, Relation::CO_OCCURS_WITH, 0.1f);
                }
            }
        }
    }
}

// ============================================================================
// UTILITIES
// ============================================================================

uint64_t AudioBridge::get_or_create_audio_node(
    const std::string& label, 
    uint8_t type,
    AtomicGraph& graph) {
    
    // Use AtomicGraph's built-in get_or_create
    // Prefix audio nodes to avoid conflicts
    std::string audio_label = "audio:" + label;
    return graph.get_or_create_node(audio_label, type);
}

bool AudioBridge::is_temporally_close(float t1, float t2) const {
    return std::abs(t1 - t2) <= config_.temporal_window;
}

void AudioBridge::print_stats() const {
    std::cout << "\n📊 AudioBridge Statistics:" << std::endl;
    std::cout << "   Events processed: " << events_processed_ << std::endl;
    std::cout << "   Nodes created: " << nodes_created_ << std::endl;
    std::cout << "   Edges created: " << edges_created_ << std::endl;
    std::cout << "   Cross-modal links: " << cross_modal_links_ << std::endl;
}

} // namespace audio
} // namespace melvin

