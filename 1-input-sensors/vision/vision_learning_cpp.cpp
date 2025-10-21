/*
 * Melvin Vision Learning - Camera to Brain Graph
 * Complete C++ port of vision_learning.py
 * 
 * Converts visual attention into nodes and EXACT/LEAP edges
 * 
 * Uses Adaptive EXACT Window:
 * - New visual concepts → large temporal window (up to 50 frames)
 * - Familiar visual concepts → small temporal window (5 frames)
 * - EXACT edges decay exponentially with frame distance
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>

using namespace cv;
using namespace std;

// ============================================================================
// STRUCTURES
// ============================================================================

struct Edge {
    int from_id;
    int to_id;
    int edge_type;
    float weight;
};

struct PatchScore {
    int x, y;
    int cx, cy;
    float saliency;
    float goal;
    float curiosity;
    float diversity;
    float focus;
    string color;
    string shape;
};

struct Concept {
    string label;
    float focus;
    string color;
    string shape;
    int x, y;
    int cluster_size;
};

struct NewNodeInfo {
    string label;
    int patch_x, patch_y;
    int cluster_size;
    float novelty;
    int window;
};

// ============================================================================
// VISION LEARNING CLASS
// ============================================================================

class VisionLearning {
public:
    // Camera and frame info
    int camera_id;
    VideoCapture cap;
    int width, height;
    int patch_size;
    
    // Graph data structures
    unordered_map<string, int> nodes;  // label -> node_id
    vector<Edge> edges;
    int next_node_id;
    
    // Temporal tracking with ADAPTIVE WINDOW
    deque<vector<int>> temporal_buffer;  // Keep last 100 frames
    unordered_map<int, int> node_activation_counts;
    int frame_count;
    
    // Edge types
    static const int EXACT = 0;  // Direct observation
    static const int LEAP = 1;   // Inferred temporal
    
    // Adaptive window config
    int N_min;
    int N_max;
    float novelty_decay;
    float novelty_boost;
    float temporal_decay_lambda;
    float min_edge_weight;
    
    // Motion detection
    Mat prev_frame_gray;
    
    // Visualization
    vector<Point> focus_history;
    vector<string> color_history;
    vector<string> shape_history;
    
    // Graph path
    string graph_path;
    
    VisionLearning(int cam_id = 0, const string& graph_dir = "data")
        : camera_id(cam_id), graph_path(graph_dir), patch_size(30),
          next_node_id(1), frame_count(0),
          N_min(5), N_max(50), novelty_decay(0.1f), novelty_boost(0.5f),
          temporal_decay_lambda(0.1f), min_edge_weight(0.01f)
    {
        temporal_buffer = deque<vector<int>>(100);
        
        // Load existing graph if it exists
        load_graph();
        
        // Camera setup
        cout << "🔍 Opening camera " << camera_id << "...\n";
        cap.open(camera_id);
        
        if (!cap.isOpened()) {
            throw runtime_error("Cannot open camera " + to_string(camera_id));
        }
        
        Mat test_frame;
        cap >> test_frame;
        
        if (test_frame.empty()) {
            throw runtime_error("Cannot read from camera " + to_string(camera_id));
        }
        
        height = test_frame.rows;
        width = test_frame.cols;
        
        cout << "✅ Camera opened: " << width << "x" << height << "\n";
        cout << "   Graph: " << nodes.size() << " nodes, " << edges.size() << " edges\n";
    }
    
    ~VisionLearning() {
        cap.release();
        destroyAllWindows();
    }
    
    void load_graph() {
        string nodes_file = graph_path + "/vision_nodes.dat";
        string edges_file = graph_path + "/vision_edges.dat";
        
        // Load nodes
        ifstream nf(nodes_file);
        if (nf.is_open()) {
            string line;
            while (getline(nf, line)) {
                istringstream iss(line);
                int node_id;
                string label;
                if (iss >> node_id) {
                    getline(iss, label);
                    if (!label.empty() && label[0] == '\t') {
                        label = label.substr(1);
                    }
                    nodes[label] = node_id;
                    next_node_id = max(next_node_id, node_id + 1);
                }
            }
            nf.close();
        }
        
        // Load edges
        ifstream ef(edges_file);
        if (ef.is_open()) {
            string line;
            while (getline(ef, line)) {
                istringstream iss(line);
                Edge e;
                if (iss >> e.from_id >> e.to_id >> e.edge_type >> e.weight) {
                    edges.push_back(e);
                }
            }
            ef.close();
        }
    }
    
    void save_graph() {
        // Create directory if needed
        system(("mkdir -p " + graph_path).c_str());
        
        // Save nodes
        ofstream nf(graph_path + "/vision_nodes.dat");
        for (const auto& [label, node_id] : nodes) {
            nf << node_id << "\t" << label << "\n";
        }
        nf.close();
        
        // Save edges
        ofstream ef(graph_path + "/vision_edges.dat");
        for (const auto& e : edges) {
            ef << e.from_id << "\t" << e.to_id << "\t" 
               << e.edge_type << "\t" << e.weight << "\n";
        }
        ef.close();
    }
    
    int get_or_create_node(const string& label) {
        if (nodes.find(label) != nodes.end()) {
            return nodes[label];
        }
        
        int node_id = next_node_id++;
        nodes[label] = node_id;
        return node_id;
    }
    
    void add_edge(int from_id, int to_id, int edge_type, float weight = 1.0f) {
        // Check if edge exists
        for (auto& e : edges) {
            if (e.from_id == from_id && e.to_id == to_id && e.edge_type == edge_type) {
                e.weight += weight;
                return;
            }
        }
        
        // New edge
        edges.push_back({from_id, to_id, edge_type, weight});
    }
    
    string classify_color(const Scalar& avg_bgr) {
        float b = avg_bgr[0], g = avg_bgr[1], r = avg_bgr[2];
        
        if (r > 150 && r > g + 30 && r > b + 30) return "red";
        if (b > 150 && b > r + 30 && b > g + 30) return "blue";
        if (g > 150 && g > r + 30 && g > b + 30) return "green";
        if (r > 200 && g > 200 && b > 200) return "bright";
        if (r < 80 && g < 80 && b < 80) return "dark";
        return "neutral";
    }
    
    float compute_diversity(const string& color, const string& shape) {
        float diversity = 0.0f;
        
        // Check recent colors
        if (color_history.size() > 10) {
            int recent_size = min(20, (int)color_history.size());
            vector<string> recent(color_history.end() - recent_size, color_history.end());
            
            int color_count = count(recent.begin(), recent.end(), color);
            float color_ratio = (float)color_count / recent.size();
            
            if (color_ratio > 0.6f) diversity -= 0.25f;
            else if (color_ratio < 0.2f) diversity += 0.25f;
        }
        
        // Check recent shapes
        if (shape_history.size() > 10) {
            int recent_size = min(20, (int)shape_history.size());
            vector<string> recent(shape_history.end() - recent_size, shape_history.end());
            
            int shape_count = count(recent.begin(), recent.end(), shape);
            float shape_ratio = (float)shape_count / recent.size();
            
            if (shape_ratio > 0.6f) diversity -= 0.15f;
            else if (shape_ratio < 0.2f) diversity += 0.15f;
        }
        
        return max(-0.4f, min(0.4f, diversity));
    }
    
    pair<vector<PatchScore>, vector<Concept>> compute_attention(const Mat& img) {
        vector<PatchScore> scores;
        
        Mat gray_frame;
        cvtColor(img, gray_frame, COLOR_BGR2GRAY);
        
        // Motion map
        Mat motion_map;
        bool has_motion = false;
        
        if (!prev_frame_gray.empty()) {
            Mat frame_diff;
            absdiff(gray_frame, prev_frame_gray, frame_diff);
            threshold(frame_diff, motion_map, 25, 255, THRESH_BINARY);
            has_motion = true;
        }
        prev_frame_gray = gray_frame.clone();
        
        int grid_h = height / patch_size;
        int grid_w = width / patch_size;
        
        for (int py = 0; py < grid_h; ++py) {
            for (int px = 0; px < grid_w; ++px) {
                int y = py * patch_size;
                int x = px * patch_size;
                
                if (y + patch_size > height || x + patch_size > width) continue;
                
                Rect roi(x, y, patch_size, patch_size);
                Mat patch = img(roi);
                Mat gray = gray_frame(roi);
                
                PatchScore ps;
                ps.x = px;
                ps.y = py;
                ps.cx = x + patch_size / 2;
                ps.cy = y + patch_size / 2;
                
                // Saliency
                Scalar mean_val, stddev;
                meanStdDev(gray, mean_val, stddev);
                ps.saliency = stddev[0] / 128.0f;
                
                // Goal (Motion)
                ps.goal = 0.0f;
                if (has_motion) {
                    Mat motion_patch = motion_map(roi);
                    int motion_pixels = countNonZero(motion_patch);
                    ps.goal = (float)motion_pixels / (patch_size * patch_size) * 2.0f;
                }
                
                // Curiosity (edges)
                Mat edges;
                Canny(gray, edges, 50, 150);
                int edge_pixels = countNonZero(edges);
                ps.curiosity = (float)edge_pixels / (patch_size * patch_size);
                ps.shape = (ps.curiosity > 0.3f) ? "edgy" : "smooth";
                
                // Color
                Scalar avg_color = mean(patch);
                ps.color = classify_color(avg_color);
                
                // Diversity
                ps.diversity = compute_diversity(ps.color, ps.shape);
                
                // Total focus: F = S + G + C + D
                ps.focus = ps.saliency + ps.goal + ps.curiosity + ps.diversity;
                
                scores.push_back(ps);
            }
        }
        
        // Find clustered concepts
        vector<Concept> concepts;
        
        if (!scores.empty()) {
            // Find best
            auto best_it = max_element(scores.begin(), scores.end(),
                [](const PatchScore& a, const PatchScore& b) {
                    return a.focus < b.focus;
                });
            
            PatchScore best = *best_it;
            
            // Count cluster
            float cluster_threshold = 0.6f;
            int cluster_size = 0;
            
            for (const auto& s : scores) {
                int dx = abs(s.x - best.x);
                int dy = abs(s.y - best.y);
                
                if (dx <= 2 && dy <= 2 && s.focus > cluster_threshold) {
                    cluster_size++;
                }
            }
            
            // Only create node if cluster
            if (cluster_size >= 3) {
                int region_x = best.x / 5;
                int region_y = best.y / 3;
                
                stringstream ss;
                ss << "visual:" << best.color << ":" << best.shape << ":r" << region_x << region_y;
                
                Concept c;
                c.label = ss.str();
                c.focus = best.focus;
                c.color = best.color;
                c.shape = best.shape;
                c.x = best.x;
                c.y = best.y;
                c.cluster_size = cluster_size;
                
                concepts.push_back(c);
            }
        }
        
        return {scores, concepts};
    }
    
    float calculate_novelty(int node_id) {
        int activations = node_activation_counts[node_id];
        return 1.0f / (1.0f + activations * novelty_decay);
    }
    
    float calculate_strength(int node_id) {
        int activations = node_activation_counts[node_id];
        float max_activations = 100.0f;
        return min(1.0f, activations / max_activations);
    }
    
    int calculate_window_size(float novelty, float strength) {
        float base_factor = 1.0f - strength;
        float novelty_factor = 1.0f + (novelty * novelty_boost);
        
        float window_range = N_max - N_min;
        float window_size = N_min + (window_range * base_factor * novelty_factor);
        
        return (int)min((float)N_max, max((float)N_min, window_size));
    }
    
    float calculate_temporal_weight(int distance) {
        float weight = exp(-temporal_decay_lambda * distance);
        return (weight >= min_edge_weight) ? weight : 0.0f;
    }
    
    vector<NewNodeInfo> process_frame(const Mat& img) {
        auto [scores, concepts] = compute_attention(img);
        
        vector<NewNodeInfo> new_nodes_created;
        vector<int> current_node_ids;
        
        for (const auto& concept : concepts) {
            bool is_new = (nodes.find(concept.label) == nodes.end());
            
            int node_id = get_or_create_node(concept.label);
            current_node_ids.push_back(node_id);
            
            // Update activation count
            node_activation_counts[node_id]++;
            
            // Calculate adaptive window
            float novelty = calculate_novelty(node_id);
            float strength = calculate_strength(node_id);
            int N_exact = calculate_window_size(novelty, strength);
            
            // Track new node
            if (is_new) {
                new_nodes_created.push_back({
                    concept.label, concept.x, concept.y,
                    concept.cluster_size, novelty, N_exact
                });
            }
            
            // Create EXACT edges to previous frames
            int frames_to_connect = min(N_exact, (int)temporal_buffer.size());
            
            for (int i = 0; i < frames_to_connect; ++i) {
                int frame_distance = i + 1;
                if (temporal_buffer.size() <= i) break;
                
                const auto& prev_frame_nodes = temporal_buffer[temporal_buffer.size() - 1 - i];
                float weight = calculate_temporal_weight(frame_distance);
                
                if (weight > 0) {
                    for (int prev_node_id : prev_frame_nodes) {
                        if (prev_node_id != node_id) {
                            add_edge(prev_node_id, node_id, EXACT, weight);
                        }
                    }
                }
            }
            
            // Track for diversity
            color_history.push_back(concept.color);
            shape_history.push_back(concept.shape);
            if (color_history.size() > 50) color_history.erase(color_history.begin());
            if (shape_history.size() > 50) shape_history.erase(shape_history.begin());
        }
        
        // Add to temporal buffer
        temporal_buffer.push_back(current_node_ids);
        if (temporal_buffer.size() > 100) {
            temporal_buffer.pop_front();
        }
        
        // LEAP edges for distant connections
        if (temporal_buffer.size() > N_max) {
            int leap_start = temporal_buffer.size() - N_max;
            for (int i = 0; i < leap_start; ++i) {
                for (int old_node_id : temporal_buffer[i]) {
                    for (int curr_node_id : current_node_ids) {
                        if (old_node_id != curr_node_id) {
                            add_edge(old_node_id, curr_node_id, LEAP, 0.3f);
                        }
                    }
                }
            }
        }
        
        frame_count++;
        
        return new_nodes_created;
    }
    
    Mat draw_visualization(const Mat& img, const vector<PatchScore>& scores, 
                          const vector<NewNodeInfo>& new_nodes) {
        Mat overlay = img.clone();
        
        // Draw attention heatmap
        Mat heatmap = Mat::zeros(img.size(), img.type());
        for (const auto& s : scores) {
            int intensity = (int)(s.focus * 255);
            Scalar color(intensity / 2, intensity, intensity);
            
            int x = s.x * patch_size;
            int y = s.y * patch_size;
            rectangle(heatmap, Point(x, y), 
                     Point(x + patch_size, y + patch_size),
                     color, -1);
        }
        
        // Blend heatmap
        addWeighted(img, 0.6, heatmap, 0.4, 0, overlay);
        
        // Highlight new nodes
        for (const auto& node_info : new_nodes) {
            int x = node_info.patch_x * patch_size;
            int y = node_info.patch_y * patch_size;
            
            rectangle(overlay, Point(x, y), 
                     Point(x + patch_size, y + patch_size),
                     Scalar(0, 255, 0), 4);
            putText(overlay, "NODE!", Point(x + 5, y + 20),
                   FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
        }
        
        // Find and draw focus
        if (!scores.empty()) {
            auto best_it = max_element(scores.begin(), scores.end(),
                [](const PatchScore& a, const PatchScore& b) {
                    return a.focus < b.focus;
                });
            
            PatchScore best = *best_it;
            Point center(best.cx, best.cy);
            
            // Crosshair
            drawMarker(overlay, center, Scalar(0, 255, 255),
                      MARKER_CROSS, 60, 3);
            circle(overlay, center, 50, Scalar(0, 255, 255), 3);
            
            // Label
            string label = "FOCUS F=" + to_string(best.focus).substr(0, 4);
            putText(overlay, label, Point(best.cx - 80, best.cy - 60),
                   FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 255), 2);
            
            // Score breakdown
            int y_offset = best.cy + 70;
            putText(overlay, "S:" + to_string(best.saliency).substr(0, 4), 
                   Point(best.cx - 80, y_offset),
                   FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 2);
            putText(overlay, "G:" + to_string(best.goal).substr(0, 4),
                   Point(best.cx - 20, y_offset),
                   FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 0), 2);
            putText(overlay, "C:" + to_string(best.curiosity).substr(0, 4),
                   Point(best.cx + 40, y_offset),
                   FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 0, 255), 2);
            
            // Track focus
            focus_history.push_back(center);
            if (focus_history.size() > 30) {
                focus_history.erase(focus_history.begin());
            }
            
            // Draw focus trail
            for (size_t i = 1; i < focus_history.size(); ++i) {
                float alpha = (float)i / focus_history.size();
                int thickness = max(1, (int)(3 * alpha));
                line(overlay, focus_history[i-1], focus_history[i],
                    Scalar(0, (int)(200*alpha), (int)(200*alpha)), thickness);
            }
        }
        
        // Draw grid
        for (int y = 0; y < height; y += patch_size) {
            line(overlay, Point(0, y), Point(width, y), Scalar(80, 80, 80), 1);
        }
        for (int x = 0; x < width; x += patch_size) {
            line(overlay, Point(x, 0), Point(x, height), Scalar(80, 80, 80), 1);
        }
        
        // Title & Info
        putText(overlay, "Melvin Vision Learning - Building Graph", Point(10, 30),
               FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 255), 2);
        putText(overlay, "F = S + G + C + D", Point(10, 60),
               FONT_HERSHEY_SIMPLEX, 0.7, Scalar(200, 200, 200), 2);
        
        // Stats overlay
        putText(overlay, "Frame: " + to_string(frame_count), Point(10, height - 120),
               FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
        putText(overlay, "Nodes: " + to_string(nodes.size()), Point(10, height - 80),
               FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
        putText(overlay, "Edges: " + to_string(edges.size()), Point(10, height - 40),
               FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
        
        // New node alerts
        if (!new_nodes.empty()) {
            int alert_y = 120;
            for (const auto& node_info : new_nodes) {
                string cluster_info = " [cluster:" + to_string(node_info.cluster_size) + "]";
                string window_info = " [window:" + to_string(node_info.window) + "]";
                string novelty_info = " novelty:" + to_string(node_info.novelty).substr(0, 4);
                
                string short_label = node_info.label.substr(0, 30);
                putText(overlay, "NEW: " + short_label, Point(10, alert_y),
                       FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 2);
                putText(overlay, cluster_info + window_info + novelty_info, Point(10, alert_y + 25),
                       FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
                alert_y += 55;
            }
        }
        
        return overlay;
    }
    
    void run() {
        cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
        cout << "║  🧠 MELVIN VISION LEARNING - Saving to Brain             ║\n";
        cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
        cout << "Camera → Attention → Graph Nodes & Edges\n\n";
        cout << "Controls:\n";
        cout << "  'q' - Quit and save\n";
        cout << "  's' - Save graph now\n";
        cout << "  'p' - Print stats\n\n";
        
        int save_interval = 60;
        
        try {
            while (true) {
                Mat frame;
                cap >> frame;
                
                if (frame.empty()) {
                    cerr << "❌ Failed to grab frame\n";
                    break;
                }
                
                // Get scores first for visualization
                auto [scores, concepts] = compute_attention(frame);
                
                // Process frame
                vector<NewNodeInfo> new_nodes = process_frame(frame);
                
                // Draw visualization
                Mat display = draw_visualization(frame, scores, new_nodes);
                
                imshow("Melvin Vision Learning", display);
                
                // Auto-save
                if (frame_count % save_interval == 0) {
                    save_graph();
                    cout << "💾 Auto-saved at frame " << frame_count << "\n";
                }
                
                // Keys
                int key = waitKey(1);
                if (key == 'q' || key == 27) break;
                else if (key == 's') {
                    save_graph();
                    cout << "💾 Saved: " << nodes.size() << " nodes, " << edges.size() << " edges\n";
                }
                else if (key == 'p') {
                    print_stats();
                }
            }
        }
        catch (const exception& e) {
            cerr << "\n❌ Error: " << e.what() << "\n";
        }
        
        cout << "\n💾 Saving final graph...\n";
        save_graph();
        print_stats();
    }
    
    void print_stats() {
        cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
        cout << "║  📊 BRAIN GRAPH STATISTICS                               ║\n";
        cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
        cout << "  Frames processed:  " << frame_count << "\n";
        cout << "  Visual nodes:      " << nodes.size() << "\n";
        cout << "  Total edges:       " << edges.size() << "\n";
        
        // Count edge types
        int exact_count = 0, leap_count = 0;
        for (const auto& e : edges) {
            if (e.edge_type == EXACT) exact_count++;
            else if (e.edge_type == LEAP) leap_count++;
        }
        cout << "    EXACT (spatial): " << exact_count << "\n";
        cout << "    LEAP (temporal): " << leap_count << "\n";
        
        // Top concepts
        cout << "\n  Top visual concepts:\n";
        int count = 0;
        for (const auto& [label, _] : nodes) {
            cout << "    - " << label << "\n";
            if (++count >= 10) break;
        }
        
        cout << "\n  Graph saved to: " << graph_path << "/\n\n";
    }
};

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    try {
        int camera_id = (argc > 1) ? atoi(argv[1]) : 0;
        
        VisionLearning learner(camera_id);
        learner.run();
        
        return 0;
    }
    catch (const exception& e) {
        cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}

