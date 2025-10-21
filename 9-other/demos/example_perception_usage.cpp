/*
 * Example Usage Patterns for Melvin Visual Perception
 */

#include "../io/melvin_perception.h"
#include "../core/storage.h"
#include "../core/reasoning.h"
#include <iostream>

using namespace melvin;
using namespace melvin::perception;

// ============================================================================
// EXAMPLE 1: Basic Single Image Processing
// ============================================================================

void example_basic_usage() {
    std::cout << "\n=== Example 1: Basic Single Image Processing ===\n\n";
    
    Storage storage;
    VisualPerception perception(&storage);
    YOLODetector detector;
    
    // Process a single image
    perception.process_image_file("photo.jpg", detector);
    
    // Check results
    perception.print_stats();
    storage.print_stats();
    
    // Save to disk
    storage.save("perception_nodes.melvin", "perception_edges.melvin");
}

// ============================================================================
// EXAMPLE 2: Video Frame Sequence
// ============================================================================

void example_video_sequence() {
    std::cout << "\n=== Example 2: Video Frame Sequence ===\n\n";
    
    Storage storage;
    
    VisualPerception::Config config;
    config.create_inter_frame_edges = true;  // Enable temporal linking
    config.temporal_window = 10;             // Track last 10 frames
    config.verbose = true;
    
    VisualPerception perception(&storage, config);
    YOLODetector detector;
    
    // Process multiple frames
    for (int i = 1; i <= 100; i++) {
        std::string frame_path = "video_frame_" + std::to_string(i) + ".jpg";
        perception.process_image_file(frame_path, detector);
        
        if (i % 10 == 0) {
            std::cout << "[Progress] Processed " << i << " frames\n";
        }
    }
    
    std::cout << "\n[Final] Video sequence processed!\n";
    perception.print_stats();
}

// ============================================================================
// EXAMPLE 3: Manual Scene Construction (Testing)
// ============================================================================

void example_manual_scene() {
    std::cout << "\n=== Example 3: Manual Scene Construction ===\n\n";
    
    Storage storage;
    VisualPerception perception(&storage);
    
    // Create a scene manually (for testing without YOLO)
    SceneFrame scene;
    scene.frame_number = 1;
    scene.timestamp = 1000;
    
    // Add objects
    scene.objects.push_back(DetectedObject("person", 0.95f, 100, 100, 200, 300));
    scene.objects.push_back(DetectedObject("dog", 0.87f, 250, 150, 350, 280));
    scene.objects.push_back(DetectedObject("car", 0.92f, 400, 200, 600, 400));
    
    // Process
    int nodes_created = perception.process_scene(scene);
    std::cout << "[Result] Created " << nodes_created << " nodes\n";
    
    storage.print_stats();
}

// ============================================================================
// EXAMPLE 4: Advanced Configuration
// ============================================================================

void example_advanced_config() {
    std::cout << "\n=== Example 4: Advanced Configuration ===\n\n";
    
    Storage storage;
    
    // Custom YOLO detector settings
    YOLODetector::Config yolo_config;
    yolo_config.model_name = "yolov8m.pt";        // Use medium model
    yolo_config.confidence_threshold = 0.4f;      // Higher confidence
    yolo_config.verbose = true;
    
    YOLODetector detector(yolo_config);
    
    // Custom perception settings
    VisualPerception::Config perc_config;
    perc_config.create_intra_frame_edges = true;
    perc_config.create_inter_frame_edges = true;
    perc_config.intra_frame_weight = 1.0f;        // Strong same-frame links
    perc_config.inter_frame_weight = 0.3f;        // Weaker cross-frame links
    perc_config.temporal_window = 5;
    perc_config.track_object_persistence = true;  // Enable IoU tracking
    perc_config.iou_threshold = 0.5f;
    perc_config.min_confidence = 0.4f;            // Filter low-confidence
    perc_config.min_box_area = 200;               // Ignore tiny boxes
    perc_config.verbose = true;
    
    VisualPerception perception(&storage, perc_config);
    
    // Process images
    perception.process_image_file("scene1.jpg", detector);
    perception.process_image_file("scene2.jpg", detector);
    
    perception.print_stats();
}

// ============================================================================
// EXAMPLE 5: Integration with Reasoning Engine
// ============================================================================

void example_reasoning_integration() {
    std::cout << "\n=== Example 5: Integration with Reasoning ===\n\n";
    
    Storage storage;
    VisualPerception perception(&storage);
    YOLODetector detector;
    
    // Process some images
    perception.process_image_file("photo1.jpg", detector);
    perception.process_image_file("photo2.jpg", detector);
    
    std::cout << "\n[Reasoning] Graph contains:\n";
    storage.print_stats();
    
    // Now you can use Melvin's reasoning engine to query the graph
    // Example: Find objects that appear together (EXACT edges)
    // Example: Find objects that appear sequentially (LEAP edges)
    
    std::cout << "\n[Reasoning] You can now query this graph with Melvin's reasoning engine!\n";
}

// ============================================================================
// EXAMPLE 6: Batch Processing
// ============================================================================

void example_batch_processing() {
    std::cout << "\n=== Example 6: Batch Processing ===\n\n";
    
    Storage storage;
    VisualPerception::Config config;
    config.verbose = false;  // Quiet mode for batch
    
    VisualPerception perception(&storage, config);
    YOLODetector detector;
    
    std::vector<std::string> image_paths = {
        "dataset/img001.jpg",
        "dataset/img002.jpg",
        "dataset/img003.jpg",
        "dataset/img004.jpg",
        "dataset/img005.jpg"
    };
    
    int total_nodes = 0;
    
    for (const auto& path : image_paths) {
        int nodes = perception.process_image_file(path, detector);
        total_nodes += nodes;
        std::cout << "[Processed] " << path << " → " << nodes << " nodes\n";
    }
    
    std::cout << "\n[Summary] Total nodes created: " << total_nodes << "\n";
    perception.print_stats();
}

// ============================================================================
// EXAMPLE 7: Custom Python Script Path
// ============================================================================

void example_custom_python_path() {
    std::cout << "\n=== Example 7: Custom Python Script Path ===\n\n";
    
    Storage storage;
    
    // If your Python script is in a different location
    YOLODetector::Config config;
    config.python_script_path = "/path/to/custom/detect_objects.py";
    config.model_name = "yolov8n.pt";
    
    YOLODetector detector(config);
    VisualPerception perception(&storage);
    
    perception.process_image_file("test.jpg", detector);
}

// ============================================================================
// EXAMPLE 8: Statistics and Monitoring
// ============================================================================

void example_statistics() {
    std::cout << "\n=== Example 8: Statistics and Monitoring ===\n\n";
    
    Storage storage;
    VisualPerception perception(&storage);
    YOLODetector detector;
    
    // Process some frames
    for (int i = 1; i <= 5; i++) {
        std::string path = "frame_" + std::to_string(i) + ".jpg";
        perception.process_image_file(path, detector);
    }
    
    // Get stats programmatically
    auto stats = perception.get_stats();
    
    std::cout << "\n[Stats] Programmatic access:\n";
    std::cout << "  Frames: " << stats.frames_processed << "\n";
    std::cout << "  Objects: " << stats.objects_detected << "\n";
    std::cout << "  Nodes: " << stats.nodes_created << "\n";
    std::cout << "  EXACT edges: " << stats.exact_edges_created << "\n";
    std::cout << "  LEAP edges: " << stats.leap_edges_created << "\n";
    std::cout << "  Avg obj/frame: " << stats.avg_objects_per_frame << "\n";
    std::cout << "  Avg time: " << stats.avg_processing_time_ms << " ms\n";
    
    // Pretty print
    perception.print_stats();
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  👁️  Melvin Perception - Example Usage Patterns                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    if (argc > 1) {
        std::string example = argv[1];
        
        if (example == "basic") {
            example_basic_usage();
        } else if (example == "video") {
            example_video_sequence();
        } else if (example == "manual") {
            example_manual_scene();
        } else if (example == "advanced") {
            example_advanced_config();
        } else if (example == "reasoning") {
            example_reasoning_integration();
        } else if (example == "batch") {
            example_batch_processing();
        } else if (example == "stats") {
            example_statistics();
        } else {
            std::cout << "\nUsage: " << argv[0] << " [example_name]\n";
            std::cout << "\nExamples:\n";
            std::cout << "  basic     - Basic single image\n";
            std::cout << "  video     - Video frame sequence\n";
            std::cout << "  manual    - Manual scene construction\n";
            std::cout << "  advanced  - Advanced configuration\n";
            std::cout << "  reasoning - Reasoning integration\n";
            std::cout << "  batch     - Batch processing\n";
            std::cout << "  stats     - Statistics monitoring\n";
            return 1;
        }
    } else {
        // Run manual example (doesn't require files)
        example_manual_scene();
    }
    
    std::cout << "\n✅ Example complete!\n\n";
    return 0;
}


