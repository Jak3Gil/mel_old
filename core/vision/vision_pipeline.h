#ifndef VISION_PIPELINE_H
#define VISION_PIPELINE_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <unordered_map>
#include <deque>
#include <string>

namespace melvin {
namespace vision {

struct Patch {
    int x, y, width, height;
    cv::Scalar avg_color;
    float motion;
    float saliency;
    std::vector<float> embedding;
};

struct Token {
    int node_id;
    Patch patch;
    std::vector<float> embedding;
};

struct Object {
    int id;
    cv::Point2f center;
    std::string color;
    int frames_tracked;
    std::vector<float> embedding;
};

struct Concept {
    int id;
    std::string color;
    std::vector<float> prototype;
    std::vector<int> instances;
    int total_frames;
    bool is_leap;
    bool is_generative;
};

// Stage 1: Vision Input
class Stage1_VisionInput {
public:
    Stage1_VisionInput();
    
    struct Output {
        std::vector<Patch> patches;
        cv::Mat motion_map;
        cv::Point2f focus_point;
    };
    
    Output process(const cv::Mat& frame, const cv::Point2f& focus_point);
    
private:
    int patch_size_;
    int fine_patch_size_;
    int focus_radius_;
    cv::Mat prev_frame_;
};

// Stage 2: Tokenize
class Stage2_Tokenize {
public:
    Stage2_Tokenize();
    
    struct Output {
        std::vector<Token> tokens;
        cv::Point2f focus_point;
        int nodes_created;
        int nodes_reused;
    };
    
    Output process(const Stage1_VisionInput::Output& input);
    
private:
    std::unordered_map<std::string, int> patch_to_node_;
    std::deque<cv::Point2f> focus_history_;
    int next_node_id_;
    float alpha_;  // Temporal smoothing
    
    std::string hash_patch(const Patch& patch);
    std::vector<float> create_embedding(const Patch& patch);
    cv::Point2f compute_focus(const std::vector<Patch>& patches);
};

// Stage 3: Connect
class Stage3_Connect {
public:
    Stage3_Connect();
    
    struct Output {
        std::vector<std::pair<int, int>> edges;
        std::unordered_map<int, Object> objects;
    };
    
    Output process(const Stage2_Tokenize::Output& input);
    
private:
    std::unordered_map<int, Object> tracked_objects_;
    int next_object_id_;
    
    void update_object_tracking(const std::vector<Token>& tokens);
    std::string get_dominant_color(const cv::Scalar& color);
};

// Stage 5: Generalize (Concept Formation)
class Stage5_Generalize {
public:
    Stage5_Generalize();
    
    struct Output {
        std::vector<Concept> concepts;
        std::vector<std::string> dominant_concepts;
    };
    
    Output process(const Stage3_Connect::Output& input);
    
private:
    std::unordered_map<int, Concept> concepts_;
    std::unordered_map<int, std::vector<std::pair<int, float>>> object_graph_;
    int next_concept_id_;
    
    // Evolution parameters
    float match_threshold_;
    float cluster_distance_;
    float learning_rate_;
    float edge_confidence_threshold_;
    
    struct Cluster {
        std::vector<int> members;
        std::string color;
        cv::Point2f center;
        int total_frames;
        std::vector<std::vector<float>> embeddings;
    };
    
    std::vector<Cluster> cluster_objects(const std::unordered_map<int, Object>& objects);
    int match_or_create_concept(const Cluster& cluster);
    void evolve_parameters(bool success);
};

} // namespace vision
} // namespace melvin

#endif // VISION_PIPELINE_H
