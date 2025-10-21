/*
 * Melvin C++ Vision Learning System
 * Full port of vision_learning.py
 * 
 * Features:
 * - Grid overlay (10×8 patches)
 * - Color-coded attention heatmap
 * - Cluster detection for node creation
 * - Adaptive EXACT window (5-50 frames)
 * - Graph building (nodes + edges)
 * - Real-time statistics
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace cv;
using namespace std;

// ============================================================================
// CONFIGURATION
// ============================================================================

const int PATCH_SIZE = 64;
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 8;
const float HIGHLIGHT_THRESHOLD = 0.3f;  // Lower threshold = more highlights
const int CLUSTER_MIN_SIZE = 3;
const int CLUSTER_RADIUS = 2;

// Adaptive window parameters
const int N_MIN = 5;
const int N_MAX = 50;
const float NOVELTY_DECAY = 0.1f;
const float TEMPORAL_DECAY_LAMBDA = 0.1f;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

struct PatchScore {
    int x, y;
    float saliency, goal, curiosity, diversity;
    float focus;
    string color;
    string shape;
};

struct VisualNode {
    string label;
    int activation_count = 0;
    float avg_edge_weight = 0.0f;
    int edge_count = 0;
};

struct Edge {
    string from, to;
    string type;  // "EXACT" or "LEAP"
    float weight;
};

// ============================================================================
// GRAPH STATE
// ============================================================================

class VisionGraph {
public:
    unordered_map<string, VisualNode> nodes;
    vector<Edge> edges;
    deque<vector<string>> temporal_buffer;  // Last N frames' node IDs
    unordered_map<string, int> color_history;
    
    int frame_count = 0;
    int nodes_created = 0;
    int exact_edges = 0;
    int leap_edges = 0;
    
    void add_node(const string& label) {
        if (nodes.find(label) == nodes.end()) {
            nodes[label] = VisualNode{label, 0, 0.0f, 0};
            nodes_created++;
        }
        nodes[label].activation_count++;
    }
    
    void add_edge(const string& from, const string& to, const string& type, float weight) {
        edges.push_back({from, to, type, weight});
        
        // Update node edge stats
        if (nodes.find(from) != nodes.end()) {
            auto& node = nodes[from];
            node.avg_edge_weight = (node.avg_edge_weight * node.edge_count + weight) / (node.edge_count + 1);
            node.edge_count++;
        }
        
        if (type == "EXACT") exact_edges++;
        else if (type == "LEAP") leap_edges++;
    }
    
    float calculate_novelty(const string& label) {
        int activations = nodes[label].activation_count;
        return 1.0f / (1.0f + activations);
    }
    
    float calculate_strength(const string& label) {
        if (nodes[label].edge_count == 0) return 0.0f;
        return nodes[label].avg_edge_weight;
    }
    
    int calculate_window_size(const string& label) {
        float novelty = calculate_novelty(label);
        float strength = calculate_strength(label);
        
        float N_exact = N_MIN + (N_MAX - N_MIN) * novelty * (1.0f - strength);
        return (int)clamp(N_exact, (float)N_MIN, (float)N_MAX);
    }
    
    float calculate_temporal_weight(int frame_distance) {
        return exp(-TEMPORAL_DECAY_LAMBDA * frame_distance);
    }
    
    void save(const string& node_file, const string& edge_file) {
        // Save nodes
        ofstream nf(node_file);
        for (const auto& [label, node] : nodes) {
            nf << label << "," << node.activation_count << "," 
               << node.avg_edge_weight << "," << node.edge_count << "\n";
        }
        nf.close();
        
        // Save edges
        ofstream ef(edge_file);
        for (const auto& edge : edges) {
            ef << edge.from << "," << edge.to << "," 
               << edge.type << "," << edge.weight << "\n";
        }
        ef.close();
    }
};

// ============================================================================
// VISION PROCESSING
// ============================================================================

string classify_color(const Mat& patch) {
    Scalar mean_color = mean(patch);
    float b = mean_color[0], g = mean_color[1], r = mean_color[2];
    
    if (r > 150 && r > g + 30 && r > b + 30) return "red";
    if (b > 150 && b > r + 30 && b > g + 30) return "blue";
    if (g > 150 && g > r + 30 && g > b + 30) return "green";
    if (r > 200 && g > 200 && b > 200) return "bright";
    if (r < 80 && g < 80 && b < 80) return "dark";
    return "neutral";
}

string classify_shape(const Mat& patch) {
    Mat gray, edges;
    cvtColor(patch, gray, COLOR_BGR2GRAY);
    Canny(gray, edges, 50, 150);
    
    int edge_pixels = countNonZero(edges);
    float edge_ratio = (float)edge_pixels / (patch.rows * patch.cols);
    
    if (edge_ratio > 0.3f) return "edgy";
    return "smooth";
}

vector<PatchScore> compute_patch_scores(const Mat& frame, const Mat& prev_frame,
                                         unordered_map<string, int>& color_history) {
    vector<PatchScore> scores;
    Mat gray, prev_gray, motion_map;
    
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    bool has_motion = false;
    
    if (!prev_frame.empty()) {
        cvtColor(prev_frame, prev_gray, COLOR_BGR2GRAY);
        absdiff(gray, prev_gray, motion_map);
        threshold(motion_map, motion_map, 25, 255, THRESH_BINARY);
        has_motion = true;
    }
    
    for (int py = 0; py < GRID_HEIGHT; ++py) {
        for (int px = 0; px < GRID_WIDTH; ++px) {
            int x = px * PATCH_SIZE;
            int y = py * PATCH_SIZE;
            
            if (x + PATCH_SIZE > frame.cols || y + PATCH_SIZE > frame.rows) {
                continue;
            }
            
            Rect roi(x, y, PATCH_SIZE, PATCH_SIZE);
            Mat patch = frame(roi);
            Mat patch_gray = gray(roi);
            
            PatchScore ps;
            ps.x = px;
            ps.y = py;
            
            // SALIENCY: Contrast (normalized better)
            Scalar mean_val, stddev;
            meanStdDev(patch_gray, mean_val, stddev);
            float saliency_raw = stddev[0] / 64.0f;
            ps.saliency = (saliency_raw > 1.0f) ? 1.0f : saliency_raw;
            
            // GOAL: Motion (boost motion heavily)
            ps.goal = 0.0f;
            if (has_motion) {
                Mat motion_patch = motion_map(roi);
                int motion_pixels = countNonZero(motion_patch);
                float goal_raw = (float)motion_pixels / (PATCH_SIZE * PATCH_SIZE) * 5.0f;
                ps.goal = (goal_raw > 2.0f) ? 2.0f : goal_raw;
            }
            
            // CURIOSITY: Edge density (boost edges)
            Mat edges;
            Canny(patch_gray, edges, 50, 150);
            int edge_pixels = countNonZero(edges);
            float curiosity_raw = (float)edge_pixels / (PATCH_SIZE * PATCH_SIZE) * 3.0f;
            ps.curiosity = (curiosity_raw > 1.5f) ? 1.5f : curiosity_raw;
            
            // DIVERSITY: Color novelty
            ps.color = classify_color(patch);
            ps.shape = classify_shape(patch);
            
            color_history[ps.color]++;
            
            int total_recent = 0;
            for (const auto& [c, count] : color_history) {
                total_recent += count;
            }
            
            float color_ratio = (total_recent > 0) ? 
                (float)color_history[ps.color] / total_recent : 0.0f;
            
            if (color_ratio > 0.6f) ps.diversity = -0.25f;
            else if (color_ratio < 0.2f) ps.diversity = 0.25f;
            else ps.diversity = 0.0f;
            
            // TOTAL FOCUS
            ps.focus = ps.saliency + ps.goal + ps.curiosity + ps.diversity;
            
            scores.push_back(ps);
        }
    }
    
    return scores;
}

// Find best patch with cluster check
PatchScore* find_clustered_best(vector<PatchScore>& scores) {
    if (scores.empty()) return nullptr;
    
    // Find highest focus patch
    PatchScore* best = &scores[0];
    for (auto& s : scores) {
        if (s.focus > best->focus) {
            best = &s;
        }
    }
    
    // Count cluster size around best
    int cluster_size = 0;
    for (const auto& s : scores) {
        int dx = abs(s.x - best->x);
        int dy = abs(s.y - best->y);
        
        if (dx <= CLUSTER_RADIUS && dy <= CLUSTER_RADIUS && s.focus > HIGHLIGHT_THRESHOLD) {
            cluster_size++;
        }
    }
    
    // Only return if part of a cluster
    if (cluster_size >= CLUSTER_MIN_SIZE) {
        return best;
    }
    
    return nullptr;
}

// ============================================================================
// VISUALIZATION
// ============================================================================

void draw_vision_overlay(Mat& display, const vector<PatchScore>& scores, 
                        PatchScore* new_node_patch, int frame_count,
                        const VisionGraph& graph) {
    // Draw grid
    for (int x = 0; x <= GRID_WIDTH * PATCH_SIZE; x += PATCH_SIZE) {
        line(display, Point(x, 0), Point(x, display.rows), Scalar(50, 50, 50), 1);
    }
    for (int y = 0; y <= GRID_HEIGHT * PATCH_SIZE; y += PATCH_SIZE) {
        line(display, Point(0, y), Point(display.cols, y), Scalar(50, 50, 50), 1);
    }
    
    // Draw attention heatmap
    float max_focus = 0.0f;
    for (const auto& s : scores) {
        if (s.focus > max_focus) max_focus = s.focus;
    }
    
    PatchScore* best = nullptr;
    float best_focus = 0.0f;
    
    for (const auto& s : scores) {
        int x = s.x * PATCH_SIZE;
        int y = s.y * PATCH_SIZE;
        
        if (s.focus > best_focus) {
            best_focus = s.focus;
            best = const_cast<PatchScore*>(&s);
        }
        
        // Color overlay (always show relative attention)
        if (max_focus > 0) {
            float normalized = s.focus / max_focus;
            
            if (normalized > 0.5f) {  // Top 50% get colored
                Scalar color;
                
                if (normalized > 0.8f) {
                    color = Scalar(0, 0, 255);  // Red (top 20%)
                } else if (normalized > 0.65f) {
                    color = Scalar(0, 165, 255);  // Orange
                } else {
                    color = Scalar(0, 255, 255);  // Yellow
                }
                
                Mat overlay = display(Rect(x, y, PATCH_SIZE, PATCH_SIZE)).clone();
                addWeighted(overlay, 0.7, Mat(PATCH_SIZE, PATCH_SIZE, CV_8UC3, color), 0.3, 0, overlay);
                overlay.copyTo(display(Rect(x, y, PATCH_SIZE, PATCH_SIZE)));
            }
        }
    }
    
    // Draw crosshair on best patch
    if (best) {
        int cross_x = best->x * PATCH_SIZE + PATCH_SIZE / 2;
        int cross_y = best->y * PATCH_SIZE + PATCH_SIZE / 2;
        drawMarker(display, Point(cross_x, cross_y), Scalar(0, 255, 255),
                  MARKER_CROSS, 40, 3);
    }
    
    // Highlight new node creation
    if (new_node_patch) {
        int x = new_node_patch->x * PATCH_SIZE;
        int y = new_node_patch->y * PATCH_SIZE;
        rectangle(display, Point(x, y), Point(x + PATCH_SIZE, y + PATCH_SIZE),
                 Scalar(0, 255, 0), 4);
        putText(display, "NODE!", Point(x + 5, y + 20),
               FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
    }
    
    // Draw stats (large, visible)
    string stats1 = format("Frame: %d", frame_count);
    string stats2 = format("Nodes: %d | Edges: %d", 
                          (int)graph.nodes.size(), (int)graph.edges.size());
    string stats3 = format("EXACT: %d | LEAP: %d", graph.exact_edges, graph.leap_edges);
    
    putText(display, stats1, Point(10, 30),
           FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 3);
    putText(display, stats2, Point(10, 60),
           FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);
    putText(display, stats3, Point(10, 85),
           FONT_HERSHEY_SIMPLEX, 0.6, Scalar(200, 200, 200), 2);
    
    // Formula
    putText(display, "F = S + G + C + D", Point(10, display.rows - 10),
           FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║  🧠 MELVIN VISION LEARNING - C++ Version                 ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    // Open camera
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "❌ Cannot open camera\n";
        return 1;
    }
    
    cap.set(CAP_PROP_FRAME_WIDTH, GRID_WIDTH * PATCH_SIZE);
    cap.set(CAP_PROP_FRAME_HEIGHT, GRID_HEIGHT * PATCH_SIZE);
    
    cout << "✅ Camera opened: " << cap.get(CAP_PROP_FRAME_WIDTH) 
         << "×" << cap.get(CAP_PROP_FRAME_HEIGHT) << "\n\n";
    
    cout << "Pipeline: Camera → Attention → Clusters → Nodes → Graph\n";
    cout << "Adaptive EXACT Window: " << N_MIN << "-" << N_MAX << " frames\n\n";
    
    cout << "Controls:\n";
    cout << "  'q' - Quit and save\n";
    cout << "  's' - Save graph now\n";
    cout << "  'p' - Print stats\n";
    cout << "  SPACE - Pause\n\n";
    
    VisionGraph graph;
    Mat frame, prev_frame, display;
    bool paused = false;
    
    while (true) {
        if (!paused) {
            cap >> frame;
            if (frame.empty()) break;
            
            graph.frame_count++;
            
            // Compute attention
            vector<PatchScore> scores = compute_patch_scores(frame, prev_frame, graph.color_history);
            
            // Find clustered best patch
            PatchScore* best_patch = find_clustered_best(scores);
            
            // Create node if cluster detected
            PatchScore* new_node_patch = nullptr;
            vector<string> current_node_ids;
            
            if (best_patch) {
                int region_x = best_patch->x / 5;
                int region_y = best_patch->y / 3;
                
                stringstream ss;
                ss << "visual:" << best_patch->color << ":" 
                   << best_patch->shape << ":r" << region_x << region_y;
                string node_label = ss.str();
                
                bool is_new = (graph.nodes.find(node_label) == graph.nodes.end());
                
                graph.add_node(node_label);
                current_node_ids.push_back(node_label);
                
                if (is_new) {
                    new_node_patch = best_patch;
                }
                
                // Create EXACT edges with adaptive window
                int N_exact = graph.calculate_window_size(node_label);
                int frames_to_connect = min(N_exact, (int)graph.temporal_buffer.size());
                
                for (int i = 0; i < frames_to_connect; ++i) {
                    int frame_distance = i + 1;
                    float weight = graph.calculate_temporal_weight(frame_distance);
                    
                    if (weight > 0 && i < graph.temporal_buffer.size()) {
                        const auto& prev_frame_nodes = graph.temporal_buffer[graph.temporal_buffer.size() - 1 - i];
                        for (const auto& prev_node : prev_frame_nodes) {
                            if (prev_node != node_label) {
                                graph.add_edge(prev_node, node_label, "EXACT", weight);
                            }
                        }
                    }
                }
                
                // Create LEAP edges for distant connections
                if (graph.temporal_buffer.size() > N_MAX) {
                    for (size_t i = N_MAX; i < graph.temporal_buffer.size(); i += 10) {
                        const auto& distant_nodes = graph.temporal_buffer[graph.temporal_buffer.size() - 1 - i];
                        for (const auto& distant_node : distant_nodes) {
                            if (distant_node != node_label) {
                                graph.add_edge(distant_node, node_label, "LEAP", 0.3f);
                            }
                        }
                    }
                }
            }
            
            // Update temporal buffer
            if (!current_node_ids.empty()) {
                graph.temporal_buffer.push_back(current_node_ids);
                if (graph.temporal_buffer.size() > N_MAX) {
                    graph.temporal_buffer.pop_front();
                }
            }
            
            // Visualization
            display = frame.clone();
            draw_vision_overlay(display, scores, new_node_patch, graph.frame_count, graph);
            
            prev_frame = frame.clone();
            
            // Auto-save every 60 frames
            if (graph.frame_count % 60 == 0) {
                graph.save("data/vision_nodes_cpp.dat", "data/vision_edges_cpp.dat");
                cout << "💾 Auto-saved at frame " << graph.frame_count << "\n";
            }
        }
        
        imshow("🧠 Melvin Vision Learning (C++)", display);
        
        int key = waitKey(1);
        if (key == 'q' || key == 27) break;
        if (key == ' ') paused = !paused;
        if (key == 's') {
            graph.save("data/vision_nodes_cpp.dat", "data/vision_edges_cpp.dat");
            cout << "💾 Manual save complete\n";
        }
        if (key == 'p') {
            cout << "\n📊 Stats: Frames=" << graph.frame_count 
                 << " | Nodes=" << graph.nodes.size() 
                 << " | Edges=" << graph.edges.size() << "\n";
        }
    }
    
    // Final save
    cout << "\n💾 Saving final graph...\n";
    graph.save("data/vision_nodes_cpp.dat", "data/vision_edges_cpp.dat");
    
    cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║  📊 FINAL STATISTICS                                      ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    cout << "  Frames processed: " << graph.frame_count << "\n";
    cout << "  Visual nodes: " << graph.nodes.size() << "\n";
    cout << "  Total edges: " << graph.edges.size() << "\n";
    cout << "    EXACT: " << graph.exact_edges << "\n";
    cout << "    LEAP: " << graph.leap_edges << "\n\n";
    
    cout << "  Graph saved to: data/\n\n";
    
    cout << "╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║  ✅ VISION LEARNING COMPLETE                             ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

