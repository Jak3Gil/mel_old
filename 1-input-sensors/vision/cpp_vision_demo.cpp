/*
 * Melvin C++ Vision Demo
 * Simple OpenCV camera → attention → display
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

// Compute attention scores (S + G + C + D)
vector<float> compute_attention(const Mat& frame, const Mat& prev_frame) {
    int patch_size = 32;
    int grid_h = frame.rows / patch_size;
    int grid_w = frame.cols / patch_size;
    
    vector<float> scores;
    Mat gray, prev_gray, motion_map;
    
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    bool has_motion = false;
    
    if (!prev_frame.empty()) {
        cvtColor(prev_frame, prev_gray, COLOR_BGR2GRAY);
        absdiff(gray, prev_gray, motion_map);
        threshold(motion_map, motion_map, 25, 255, THRESH_BINARY);
        has_motion = true;
    }
    
    for (int py = 0; py < grid_h; ++py) {
        for (int px = 0; px < grid_w; ++px) {
            int x = px * patch_size;
            int y = py * patch_size;
            
            if (x + patch_size > frame.cols || y + patch_size > frame.rows) {
                scores.push_back(0.0f);
                continue;
            }
            
            Rect roi(x, y, patch_size, patch_size);
            Mat patch_gray = gray(roi);
            
            // SALIENCY: Contrast
            Scalar mean, stddev;
            meanStdDev(patch_gray, mean, stddev);
            float saliency = stddev[0] / 128.0f;
            
            // GOAL: Motion
            float goal = 0.0f;
            if (has_motion) {
                Mat motion_patch = motion_map(roi);
                int motion_pixels = countNonZero(motion_patch);
                goal = (float)motion_pixels / (patch_size * patch_size) * 2.0f;
            }
            
            // CURIOSITY: Edge density
            Mat edges;
            Canny(patch_gray, edges, 50, 150);
            int edge_pixels = countNonZero(edges);
            float curiosity = (float)edge_pixels / (patch_size * patch_size);
            
            // FOCUS: Total
            float focus = saliency + goal + curiosity;
            scores.push_back(focus);
        }
    }
    
    return scores;
}

int main() {
    cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║  🧠 MELVIN C++ VISION DEMO                                   ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // Open camera
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "❌ Cannot open camera\n";
        return 1;
    }
    
    // Set resolution
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    
    cout << "✅ Camera opened: " << cap.get(CAP_PROP_FRAME_WIDTH) 
         << "×" << cap.get(CAP_PROP_FRAME_HEIGHT) << "\n\n";
    
    cout << "Attention Formula: F = S + G + C + D\n";
    cout << "  S = Saliency (contrast)\n";
    cout << "  G = Goal (motion)\n";
    cout << "  C = Curiosity (edges)\n";
    cout << "  D = Diversity (novelty)\n\n";
    
    cout << "Controls:\n";
    cout << "  'q' - Quit\n";
    cout << "  SPACE - Pause/Resume\n\n";
    
    Mat frame, prev_frame, display;
    int patch_size = 32;
    bool paused = false;
    int frame_count = 0;
    
    while (true) {
        if (!paused) {
            cap >> frame;
            if (frame.empty()) break;
            
            frame_count++;
            
            // Compute attention
            vector<float> attention = compute_attention(frame, prev_frame);
            
            // Find max attention
            float max_attention = *max_element(attention.begin(), attention.end());
            
            // Draw visualization
            display = frame.clone();
            int grid_h = frame.rows / patch_size;
            int grid_w = frame.cols / patch_size;
            
            int idx = 0;
            int max_idx = 0;
            float max_val = 0;
            
            for (int py = 0; py < grid_h; ++py) {
                for (int px = 0; px < grid_w; ++px) {
                    if (idx >= attention.size()) break;
                    
                    float score = attention[idx];
                    if (score > max_val) {
                        max_val = score;
                        max_idx = idx;
                    }
                    
                    // Normalize and colorize
                    float normalized = (max_attention > 0) ? (score / max_attention) : 0;
                    
                    if (normalized > 0.5f) {
                        int x = px * patch_size;
                        int y = py * patch_size;
                        
                        // Color based on attention
                        Scalar color;
                        if (normalized > 0.8f) {
                            color = Scalar(0, 0, 255);  // Red (high)
                        } else if (normalized > 0.6f) {
                            color = Scalar(0, 165, 255);  // Orange
                        } else {
                            color = Scalar(0, 255, 255);  // Yellow
                        }
                        
                        rectangle(display, Point(x, y), Point(x + patch_size, y + patch_size),
                                color, 2);
                    }
                    
                    idx++;
                }
            }
            
            // Draw crosshair on max attention
            int max_py = max_idx / grid_w;
            int max_px = max_idx % grid_w;
            int cross_x = max_px * patch_size + patch_size / 2;
            int cross_y = max_py * patch_size + patch_size / 2;
            
            drawMarker(display, Point(cross_x, cross_y), Scalar(0, 255, 0),
                      MARKER_CROSS, 30, 3);
            
            // Draw stats
            string stats = format("Frame: %d | Max Attention: %.2f", frame_count, max_val);
            putText(display, stats, Point(10, 30),
                   FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
            
            putText(display, "F = S + G + C + D", Point(10, display.rows - 10),
                   FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
            
            prev_frame = frame.clone();
        }
        
        // Show window
        imshow("🧠 Melvin C++ Vision", display);
        
        // Handle keyboard
        int key = waitKey(1);
        if (key == 'q' || key == 27) break;
        if (key == ' ') paused = !paused;
    }
    
    cout << "\n✅ Processed " << frame_count << " frames\n";
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║  ✅ C++ VISION DEMO COMPLETE                                ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

