#include "scale_demo_generators.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace melvin {
namespace scale_demo {

// ============================================================================
// TextCorpusGenerator
// ============================================================================

TextCorpusGenerator::TextCorpusGenerator(const Config& config) 
    : config_(config), rng_(config.seed) {}

std::vector<TextCorpusGenerator::GeneratedSentence> TextCorpusGenerator::generate() {
    std::vector<GeneratedSentence> sentences;
    sentences.reserve(config_.num_sentences);
    
    std::uniform_int_distribution<int> type_dist(0, 5);
    
    for (uint32_t i = 0; i < config_.num_sentences; ++i) {
        int type = type_dist(rng_);
        
        switch (type) {
            case 0: sentences.push_back(generate_taxonomy()); break;
            case 1: sentences.push_back(generate_habitat()); break;
            case 2: sentences.push_back(generate_capability()); break;
            case 3: sentences.push_back(generate_tool_use()); break;
            case 4: sentences.push_back(generate_part_of()); break;
            case 5: sentences.push_back(generate_cause_effect()); break;
        }
    }
    
    return sentences;
}

TextCorpusGenerator::GeneratedSentence TextCorpusGenerator::generate_taxonomy() {
    GeneratedSentence sent;
    
    std::uniform_int_distribution<size_t> animal_dist(0, animals_.size() - 1);
    std::uniform_int_distribution<size_t> class_dist(0, classes_.size() - 1);
    
    std::string animal = animals_[animal_dist(rng_)];
    std::string cls = classes_[class_dist(rng_)];
    
    sent.text = animal + " are " + cls;
    sent.tokens = {animal, "are", cls};
    
    // Temporal edges: animal -> are -> cls
    sent.temporal_edges.push_back({0, 1});
    sent.temporal_edges.push_back({1, 2});
    
    // Potential leap edge
    add_leap_edges(sent);
    
    return sent;
}

TextCorpusGenerator::GeneratedSentence TextCorpusGenerator::generate_habitat() {
    GeneratedSentence sent;
    
    std::uniform_int_distribution<size_t> animal_dist(0, animals_.size() - 1);
    std::uniform_int_distribution<size_t> place_dist(0, places_.size() - 1);
    
    std::string animal = animals_[animal_dist(rng_)];
    std::string place = places_[place_dist(rng_)];
    
    sent.text = animal + " live in " + place;
    sent.tokens = {animal, "live", "in", place};
    
    // Temporal edges
    for (size_t i = 0; i < sent.tokens.size() - 1; ++i) {
        sent.temporal_edges.push_back({static_cast<int>(i), static_cast<int>(i + 1)});
    }
    
    add_leap_edges(sent);
    
    return sent;
}

TextCorpusGenerator::GeneratedSentence TextCorpusGenerator::generate_capability() {
    GeneratedSentence sent;
    
    std::uniform_int_distribution<size_t> animal_dist(0, animals_.size() - 1);
    std::uniform_int_distribution<size_t> verb_dist(0, verbs_.size() - 1);
    
    std::string animal = animals_[animal_dist(rng_)];
    std::string verb = verbs_[verb_dist(rng_)];
    
    sent.text = animal + " can " + verb;
    sent.tokens = {animal, "can", verb};
    
    sent.temporal_edges.push_back({0, 1});
    sent.temporal_edges.push_back({1, 2});
    
    add_leap_edges(sent);
    
    return sent;
}

TextCorpusGenerator::GeneratedSentence TextCorpusGenerator::generate_tool_use() {
    GeneratedSentence sent;
    
    std::uniform_int_distribution<size_t> tool_dist(0, tools_.size() - 1);
    std::uniform_int_distribution<size_t> task_dist(0, tasks_.size() - 1);
    
    std::string tool = tools_[tool_dist(rng_)];
    std::string task = tasks_[task_dist(rng_)];
    
    sent.text = tool + " is used for " + task;
    sent.tokens = {tool, "is", "used", "for", task};
    
    for (size_t i = 0; i < sent.tokens.size() - 1; ++i) {
        sent.temporal_edges.push_back({static_cast<int>(i), static_cast<int>(i + 1)});
    }
    
    add_leap_edges(sent);
    
    return sent;
}

TextCorpusGenerator::GeneratedSentence TextCorpusGenerator::generate_part_of() {
    GeneratedSentence sent;
    
    std::uniform_int_distribution<size_t> obj_dist(0, objects_.size() - 1);
    std::uniform_int_distribution<size_t> part_dist(0, parts_.size() - 1);
    
    std::string obj = objects_[obj_dist(rng_)];
    std::string part = parts_[part_dist(rng_)];
    
    sent.text = obj + " has " + part;
    sent.tokens = {obj, "has", part};
    
    sent.temporal_edges.push_back({0, 1});
    sent.temporal_edges.push_back({1, 2});
    
    add_leap_edges(sent);
    
    return sent;
}

TextCorpusGenerator::GeneratedSentence TextCorpusGenerator::generate_cause_effect() {
    GeneratedSentence sent;
    
    // Simple cause-effect patterns
    std::vector<std::pair<std::string, std::string>> cause_effects = {
        {"rain", "flood"}, {"heat", "fire"}, {"cold", "ice"},
        {"wind", "erosion"}, {"pressure", "explosion"}
    };
    
    std::uniform_int_distribution<size_t> ce_dist(0, cause_effects.size() - 1);
    auto [cause, effect] = cause_effects[ce_dist(rng_)];
    
    sent.text = cause + " causes " + effect;
    sent.tokens = {cause, "causes", effect};
    
    sent.temporal_edges.push_back({0, 1});
    sent.temporal_edges.push_back({1, 2});
    
    add_leap_edges(sent);
    
    return sent;
}

void TextCorpusGenerator::add_leap_edges(GeneratedSentence& sent) {
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);
    
    // Try to add leap edges between non-adjacent tokens
    for (size_t i = 0; i < sent.tokens.size(); ++i) {
        for (size_t j = i + 2; j < sent.tokens.size(); ++j) {
            if (prob_dist(rng_) < config_.leap_probability) {
                sent.leap_edges.push_back({static_cast<int>(i), static_cast<int>(j)});
            }
        }
    }
}

// ============================================================================
// AudioCodeGenerator
// ============================================================================

AudioCodeGenerator::AudioCodeGenerator(const Config& config)
    : config_(config), rng_(config.seed) {
    initialize_motifs();
}

void AudioCodeGenerator::initialize_motifs() {
    // Create common phoneme patterns that represent "pseudo-words"
    motifs_ = {
        {0, 1, 2},       // "ba"
        {3, 4, 5},       // "da"
        {6, 7, 8},       // "ka"
        {1, 2, 3},       // "ta"
        {4, 5, 6},       // "ma"
        {7, 8, 9},       // "na"
        {9, 10, 11},     // "pa"
        {0, 5, 10}       // "la"
    };
    
    motif_names_ = {"ba", "da", "ka", "ta", "ma", "na", "pa", "la"};
}

std::vector<AudioCodeGenerator::GeneratedUtterance> AudioCodeGenerator::generate() {
    std::vector<GeneratedUtterance> utterances;
    utterances.reserve(config_.num_utterances);
    
    std::uniform_int_distribution<uint32_t> phoneme_dist(0, config_.num_phonemes - 1);
    std::uniform_real_distribution<float> amplitude_dist(0.3f, 1.0f);
    
    for (uint32_t utt = 0; utt < config_.num_utterances; ++utt) {
        GeneratedUtterance utterance;
        
        for (uint32_t frame = 0; frame < config_.frames_per_utterance; ++frame) {
            AudioFrame af;
            af.phoneme_code = static_cast<uint8_t>(phoneme_dist(rng_));
            af.timestamp_ms = utt * config_.frames_per_utterance * 10 + frame * 10;  // 10ms frames
            af.amplitude = amplitude_dist(rng_);
            
            utterance.frames.push_back(af);
        }
        
        // Identify motifs in the generated sequence
        std::vector<uint8_t> phoneme_seq;
        for (const auto& frame : utterance.frames) {
            phoneme_seq.push_back(frame.phoneme_code);
        }
        
        for (size_t i = 0; i < phoneme_seq.size() - 2; ++i) {
            for (size_t m = 0; m < motifs_.size(); ++m) {
                if (matches_motif(phoneme_seq, i, m)) {
                    utterance.motif_labels.push_back(motif_names_[m]);
                    break;
                }
            }
        }
        
        utterances.push_back(std::move(utterance));
    }
    
    return utterances;
}

bool AudioCodeGenerator::matches_motif(const std::vector<uint8_t>& sequence, 
                                       size_t pos, 
                                       size_t motif_idx) {
    const auto& motif = motifs_[motif_idx];
    
    if (pos + motif.size() > sequence.size()) return false;
    
    for (size_t i = 0; i < motif.size(); ++i) {
        if (sequence[pos + i] != motif[i]) return false;
    }
    
    return true;
}

// ============================================================================
// ImageEmbeddingGenerator
// ============================================================================

ImageEmbeddingGenerator::ImageEmbeddingGenerator(const Config& config)
    : config_(config), rng_(config.seed) {}

std::vector<ImageEmbeddingGenerator::ImageEmbedding> ImageEmbeddingGenerator::generate() {
    std::vector<ImageEmbedding> embeddings;
    embeddings.reserve(config_.num_labels * config_.samples_per_label);
    
    // Generate one cluster center per label
    std::vector<std::vector<float>> cluster_centers;
    for (uint32_t l = 0; l < config_.num_labels; ++l) {
        cluster_centers.push_back(generate_cluster_center());
    }
    
    // Sample from each cluster
    for (uint32_t l = 0; l < config_.num_labels; ++l) {
        std::string label = (l < labels_.size()) ? labels_[l] : "label_" + std::to_string(l);
        
        for (uint32_t s = 0; s < config_.samples_per_label; ++s) {
            ImageEmbedding emb;
            emb.embedding = sample_from_cluster(cluster_centers[l]);
            emb.label = label;
            emb.label_id = l;
            
            embeddings.push_back(std::move(emb));
        }
    }
    
    return embeddings;
}

std::vector<float> ImageEmbeddingGenerator::generate_cluster_center() {
    std::normal_distribution<float> center_dist(0.0f, 1.0f);
    
    std::vector<float> center(config_.embedding_dim);
    for (auto& val : center) {
        val = center_dist(rng_);
    }
    
    // Normalize to unit length
    float norm = 0.0f;
    for (float val : center) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    for (auto& val : center) {
        val /= norm;
    }
    
    return center;
}

std::vector<int8_t> ImageEmbeddingGenerator::sample_from_cluster(const std::vector<float>& center) {
    std::normal_distribution<float> noise_dist(0.0f, config_.cluster_stddev);
    
    std::vector<int8_t> embedding(config_.embedding_dim);
    
    for (size_t i = 0; i < config_.embedding_dim; ++i) {
        float val = center[i] + noise_dist(rng_);
        // Quantize to int8: [-1, 1] -> [-127, 127]
        int8_t quantized = static_cast<int8_t>(std::max(-127.0f, std::min(127.0f, val * 127.0f)));
        embedding[i] = quantized;
    }
    
    return embedding;
}

float ImageEmbeddingGenerator::cosine_similarity(const std::vector<int8_t>& a, 
                                                  const std::vector<int8_t>& b) {
    if (a.size() != b.size()) return 0.0f;
    
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<float>(a[i]) * static_cast<float>(b[i]);
        norm_a += static_cast<float>(a[i]) * static_cast<float>(a[i]);
        norm_b += static_cast<float>(b[i]) * static_cast<float>(b[i]);
    }
    
    if (norm_a == 0.0f || norm_b == 0.0f) return 0.0f;
    
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

// ============================================================================
// QueryGenerator
// ============================================================================

QueryGenerator::QueryGenerator(uint64_t seed) : rng_(seed) {}

std::vector<std::string> QueryGenerator::generate_queries(
    const std::vector<std::string>& vocabulary, 
    uint32_t num_queries) {
    
    std::vector<std::string> queries;
    queries.reserve(num_queries);
    
    std::uniform_int_distribution<size_t> template_dist(0, query_templates_.size() - 1);
    std::uniform_int_distribution<size_t> vocab_dist(0, vocabulary.size() - 1);
    
    for (uint32_t i = 0; i < num_queries; ++i) {
        std::string template_str = query_templates_[template_dist(rng_)];
        std::string word = vocabulary[vocab_dist(rng_)];
        
        // Replace {} with word
        size_t pos = template_str.find("{}");
        if (pos != std::string::npos) {
            template_str.replace(pos, 2, word);
        }
        
        queries.push_back(template_str);
    }
    
    return queries;
}

// ============================================================================
// ProbeSetGenerator
// ============================================================================

ProbeSetGenerator::ProbeSetGenerator(uint64_t seed) : rng_(seed) {}

std::vector<ProbeSetGenerator::ProbeQuestion> ProbeSetGenerator::generate_probes(uint32_t num_probes) {
    std::vector<ProbeQuestion> probes;
    probes.reserve(num_probes);
    
    // Define ground-truth facts
    std::vector<std::tuple<std::string, std::vector<std::string>, std::string>> facts = {
        // Taxonomy
        {"what are cats", {"cats", "are", "mammals"}, "taxonomy"},
        {"what are dogs", {"dogs", "are", "mammals"}, "taxonomy"},
        {"what are sparrows", {"sparrows", "are", "birds"}, "taxonomy"},
        {"what are eagles", {"eagles", "are", "birds"}, "taxonomy"},
        {"what are salmon", {"salmon", "are", "fish"}, "taxonomy"},
        
        // Habitat
        {"where do fish live", {"fish", "live", "in", "ocean"}, "habitat"},
        {"where do birds live", {"birds", "live", "in", "tree"}, "habitat"},
        
        // Capability
        {"what can birds do", {"birds", "can", "fly"}, "capability"},
        {"what can fish do", {"fish", "can", "swim"}, "capability"},
        {"what can lions do", {"lions", "can", "hunt"}, "capability"},
        
        // Tool use
        {"what is hammer used for", {"hammer", "is", "used", "for", "building"}, "tool_use"},
        {"what is saw used for", {"saw", "is", "used", "for", "cutting"}, "tool_use"},
        
        // Part-of
        {"what has car", {"car", "has", "wheel"}, "part_of"},
        {"what has plane", {"plane", "has", "wing"}, "part_of"}
    };
    
    // Sample probes
    std::uniform_int_distribution<size_t> fact_dist(0, facts.size() - 1);
    
    for (uint32_t i = 0; i < num_probes; ++i) {
        size_t idx = fact_dist(rng_);
        auto [query, path, category] = facts[idx];
        
        ProbeQuestion probe;
        probe.query = query;
        probe.expected_path = path;
        probe.category = category;
        
        probes.push_back(probe);
    }
    
    return probes;
}

} // namespace scale_demo
} // namespace melvin

