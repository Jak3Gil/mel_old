#pragma once

#include <string>
#include <vector>
#include <random>
#include <unordered_set>
#include <unordered_map>

namespace melvin {
namespace scale_demo {

// Template-based text corpus generator
class TextCorpusGenerator {
public:
    struct Config {
        uint32_t num_sentences = 8000;
        float leap_probability = 0.15f;
        uint64_t seed = 12345;
    };
    
    struct GeneratedSentence {
        std::string text;
        std::vector<std::string> tokens;
        std::vector<std::pair<int, int>> temporal_edges;  // (token_i, token_i+1)
        std::vector<std::pair<int, int>> leap_edges;      // non-adjacent connections
    };
    
    TextCorpusGenerator(const Config& config);
    
    std::vector<GeneratedSentence> generate();
    
private:
    Config config_;
    std::mt19937 rng_;
    
    // Templates
    std::vector<std::string> animals_ = {
        "cat", "dog", "sparrow", "eagle", "salmon", "shark", "lion", "elephant",
        "bear", "wolf", "rabbit", "deer", "fox", "owl", "hawk", "crow",
        "dolphin", "whale", "octopus", "snake", "lizard", "frog", "toad"
    };
    
    std::vector<std::string> classes_ = {
        "mammal", "bird", "fish", "reptile", "amphibian"
    };
    
    std::vector<std::string> places_ = {
        "forest", "ocean", "river", "desert", "mountain", "grassland", "swamp",
        "cave", "tree", "burrow", "nest", "den"
    };
    
    std::vector<std::string> verbs_ = {
        "fly", "swim", "run", "hunt", "climb", "dig", "jump", "dive", "crawl"
    };
    
    std::vector<std::string> tools_ = {
        "hammer", "saw", "drill", "screwdriver", "wrench", "pliers", "knife"
    };
    
    std::vector<std::string> tasks_ = {
        "building", "cutting", "drilling", "fastening", "gripping", "slicing"
    };
    
    std::vector<std::string> parts_ = {
        "wheel", "engine", "door", "window", "roof", "wing", "tail", "leg"
    };
    
    std::vector<std::string> objects_ = {
        "car", "plane", "boat", "bicycle", "train", "helicopter"
    };
    
    GeneratedSentence generate_taxonomy();
    GeneratedSentence generate_habitat();
    GeneratedSentence generate_capability();
    GeneratedSentence generate_tool_use();
    GeneratedSentence generate_part_of();
    GeneratedSentence generate_cause_effect();
    
    void add_leap_edges(GeneratedSentence& sent);
};

// Audio-code stream generator (speech-like sequences)
class AudioCodeGenerator {
public:
    struct Config {
        uint32_t num_utterances = 400;
        uint32_t frames_per_utterance = 100;
        uint32_t num_phonemes = 12;
        uint64_t seed = 54321;
    };
    
    struct AudioFrame {
        uint8_t phoneme_code;
        uint64_t timestamp_ms;
        float amplitude;
    };
    
    struct GeneratedUtterance {
        std::vector<AudioFrame> frames;
        std::vector<std::string> motif_labels;  // Recognized word-like patterns
    };
    
    AudioCodeGenerator(const Config& config);
    
    std::vector<GeneratedUtterance> generate();
    
private:
    Config config_;
    std::mt19937 rng_;
    
    // Phoneme motifs that combine into "pseudo-words"
    std::vector<std::vector<uint8_t>> motifs_;
    std::vector<std::string> motif_names_;
    
    void initialize_motifs();
    std::vector<uint8_t> generate_motif_sequence();
    bool matches_motif(const std::vector<uint8_t>& sequence, size_t pos, size_t motif_idx);
};

// Image embedding stub generator (semantic anchors)
class ImageEmbeddingGenerator {
public:
    struct Config {
        uint32_t num_labels = 30;
        uint32_t samples_per_label = 200;
        uint32_t embedding_dim = 256;
        float cluster_stddev = 0.1f;
        uint64_t seed = 98765;
    };
    
    struct ImageEmbedding {
        std::vector<int8_t> embedding;  // Quantized int8 embedding
        std::string label;
        uint32_t label_id;
    };
    
    ImageEmbeddingGenerator(const Config& config);
    
    std::vector<ImageEmbedding> generate();
    
    // Compute similarity between embeddings
    static float cosine_similarity(const std::vector<int8_t>& a, const std::vector<int8_t>& b);
    
private:
    Config config_;
    std::mt19937 rng_;
    
    std::vector<std::string> labels_ = {
        "cat", "dog", "car", "tree", "house", "person", "flower", "bird",
        "chair", "table", "computer", "phone", "book", "cup", "ball", "shoe",
        "mountain", "ocean", "sky", "grass", "road", "building", "bridge", "river",
        "food", "fruit", "vegetable", "tool", "machine", "animal"
    };
    
    // Generate cluster center for a label
    std::vector<float> generate_cluster_center();
    
    // Sample from cluster
    std::vector<int8_t> sample_from_cluster(const std::vector<float>& center);
};

// Query generator for forming thought nodes
class QueryGenerator {
public:
    QueryGenerator(uint64_t seed = 11111);
    
    std::vector<std::string> generate_queries(const std::vector<std::string>& vocabulary, 
                                               uint32_t num_queries = 500);
    
private:
    std::mt19937 rng_;
    
    std::vector<std::string> query_templates_ = {
        "what are {}",
        "what is {}",
        "where do {} live",
        "what can {} do",
        "what has {}",
        "what is {} used for"
    };
};

// Ground-truth probe set for validation
class ProbeSetGenerator {
public:
    struct ProbeQuestion {
        std::string query;
        std::vector<std::string> expected_path;  // Expected token sequence
        std::string category;  // taxonomy/habitat/capability/etc.
    };
    
    ProbeSetGenerator(uint64_t seed = 22222);
    
    std::vector<ProbeQuestion> generate_probes(uint32_t num_probes = 500);
    
private:
    std::mt19937 rng_;
};

} // namespace scale_demo
} // namespace melvin

