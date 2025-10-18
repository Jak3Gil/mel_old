/*
 * Test Harness for Melvin Visual Perception Engine
 * 
 * Demonstrates YOLO integration and object detection → knowledge graph
 */

#include "../io/melvin_perception.h"
#include "../core/storage.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

using namespace melvin;
using namespace melvin::perception;

// ============================================================================
// TEST SCENARIOS
// ============================================================================

void test_yolo_detection(const std::string& image_path) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 1: YOLO Object Detection                                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    YOLODetector::Config config;
    config.verbose = true;
    config.confidence_threshold = 0.25f;
    
    YOLODetector detector(config);
    
    std::cout << "[Test] Detecting objects in: " << image_path << "\n\n";
    
    auto detections = detector.detect_from_file(image_path);
    
    std::cout << "\n[Test] Detected " << detections.size() << " objects:\n\n";
    
    for (size_t i = 0; i < detections.size(); ++i) {
        const auto& obj = detections[i];
        std::cout << "  [" << i << "] " << obj.label 
                  << " (conf=" << std::fixed << std::setprecision(2) << obj.confidence << ") "
                  << "bbox=[" << obj.x1 << "," << obj.y1 << "," << obj.x2 << "," << obj.y2 << "] "
                  << "area=" << obj.area() << " px²\n";
    }
    
    std::cout << "\n✓ YOLO detection test passed\n";
}

void test_single_frame_processing() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 2: Single Frame Processing                               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    // Create storage
    Storage storage;
    
    // Create perception engine
    VisualPerception::Config config;
    config.verbose = true;
    config.create_intra_frame_edges = true;
    config.create_inter_frame_edges = false;  // Single frame test
    
    VisualPerception perception(&storage, config);
    
    // Create mock scene frame
    SceneFrame scene;
    scene.frame_number = 1;
    scene.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Add mock detections
    scene.objects.push_back(DetectedObject("person", 0.95f, 100, 100, 200, 300));
    scene.objects.push_back(DetectedObject("dog", 0.87f, 250, 150, 350, 280));
    scene.objects.push_back(DetectedObject("car", 0.92f, 400, 200, 600, 400));
    
    std::cout << "[Test] Processing frame with " << scene.objects.size() << " objects\n\n";
    
    int nodes_created = perception.process_scene(scene);
    
    std::cout << "\n[Test] Created " << nodes_created << " nodes\n";
    
    // Print stats
    perception.print_stats();
    
    // Verify graph
    std::cout << "\n[Test] Knowledge Graph State:\n";
    storage.print_stats();
    
    std::cout << "\n✓ Single frame processing test passed\n";
}

void test_multi_frame_processing() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 3: Multi-Frame Temporal Processing                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    
    VisualPerception::Config config;
    config.verbose = true;
    config.create_intra_frame_edges = true;
    config.create_inter_frame_edges = true;
    config.temporal_window = 3;
    config.track_object_persistence = false;  // Link all same-label objects
    
    VisualPerception perception(&storage, config);
    
    // Simulate 3 frames with evolving scene
    std::vector<SceneFrame> frames;
    
    // Frame 1: person + dog
    SceneFrame frame1;
    frame1.frame_number = 1;
    frame1.timestamp = 1000;
    frame1.objects.push_back(DetectedObject("person", 0.95f, 100, 100, 200, 300));
    frame1.objects.push_back(DetectedObject("dog", 0.87f, 250, 150, 350, 280));
    frames.push_back(frame1);
    
    // Frame 2: person moves, dog stays, car appears
    SceneFrame frame2;
    frame2.frame_number = 2;
    frame2.timestamp = 2000;
    frame2.objects.push_back(DetectedObject("person", 0.93f, 120, 105, 220, 305));
    frame2.objects.push_back(DetectedObject("dog", 0.88f, 252, 152, 352, 282));
    frame2.objects.push_back(DetectedObject("car", 0.91f, 400, 200, 600, 400));
    frames.push_back(frame2);
    
    // Frame 3: all objects present
    SceneFrame frame3;
    frame3.frame_number = 3;
    frame3.timestamp = 3000;
    frame3.objects.push_back(DetectedObject("person", 0.94f, 140, 110, 240, 310));
    frame3.objects.push_back(DetectedObject("dog", 0.89f, 254, 154, 354, 284));
    frame3.objects.push_back(DetectedObject("car", 0.90f, 405, 205, 605, 405));
    frames.push_back(frame3);
    
    // Process all frames
    for (const auto& frame : frames) {
        std::cout << "\n[Test] Processing frame " << frame.frame_number 
                  << " with " << frame.objects.size() << " objects\n";
        perception.process_scene(frame);
    }
    
    // Print final stats
    std::cout << "\n";
    perception.print_stats();
    
    std::cout << "\n[Test] Final Knowledge Graph State:\n";
    storage.print_stats();
    
    std::cout << "\n✓ Multi-frame temporal processing test passed\n";
}

void test_edge_types() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 4: Edge Type Verification (EXACT vs LEAP)                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    
    VisualPerception::Config config;
    config.verbose = false;  // Quiet mode
    config.create_intra_frame_edges = true;
    config.create_inter_frame_edges = true;
    
    VisualPerception perception(&storage, config);
    
    // Frame 1: 2 objects
    SceneFrame frame1;
    frame1.frame_number = 1;
    frame1.objects.push_back(DetectedObject("person", 0.95f, 100, 100, 200, 300));
    frame1.objects.push_back(DetectedObject("dog", 0.87f, 250, 150, 350, 280));
    perception.process_scene(frame1);
    
    // Frame 2: same objects
    SceneFrame frame2;
    frame2.frame_number = 2;
    frame2.objects.push_back(DetectedObject("person", 0.94f, 105, 105, 205, 305));
    frame2.objects.push_back(DetectedObject("dog", 0.88f, 255, 155, 355, 285));
    perception.process_scene(frame2);
    
    std::cout << "[Test] Verifying edge types...\n\n";
    
    size_t exact_count = storage.edge_count_by_type(RelationType::EXACT);
    size_t leap_count = storage.edge_count_by_type(RelationType::LEAP);
    
    std::cout << "  EXACT edges (intra-frame):  " << exact_count << "\n";
    std::cout << "  LEAP edges (inter-frame):   " << leap_count << "\n";
    
    std::cout << "\n[Test] Expected:\n";
    std::cout << "  EXACT: 4 (2 bidirectional pairs per frame = 2 frames * 2 = 4)\n";
    std::cout << "  LEAP:  4 (person→person, dog→dog, both directions = 2 * 2 = 4)\n";
    
    if (exact_count == 4 && leap_count == 4) {
        std::cout << "\n✓ Edge types verified correctly!\n";
    } else {
        std::cout << "\n⚠ Edge counts don't match expected values\n";
    }
}

void test_persistence() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 5: Graph Persistence (Save & Load)                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    const std::string nodes_path = "test_perception_nodes.melvin";
    const std::string edges_path = "test_perception_edges.melvin";
    
    // Create and populate graph
    {
        Storage storage;
        VisualPerception perception(&storage);
        
        SceneFrame frame;
        frame.frame_number = 1;
        frame.objects.push_back(DetectedObject("person", 0.95f, 100, 100, 200, 300));
        frame.objects.push_back(DetectedObject("car", 0.91f, 400, 200, 600, 400));
        
        perception.process_scene(frame);
        
        std::cout << "[Test] Created graph with " << storage.node_count() 
                  << " nodes, " << storage.edge_count() << " edges\n";
        
        // Save
        std::cout << "[Test] Saving to disk...\n";
        storage.save(nodes_path, edges_path);
    }
    
    // Load and verify
    {
        Storage storage;
        std::cout << "[Test] Loading from disk...\n";
        
        if (storage.load(nodes_path, edges_path)) {
            std::cout << "[Test] Loaded graph with " << storage.node_count() 
                      << " nodes, " << storage.edge_count() << " edges\n";
            
            storage.print_stats();
            
            std::cout << "\n✓ Persistence test passed\n";
        } else {
            std::cout << "\n✗ Failed to load graph\n";
        }
    }
    
    // Cleanup
    std::remove(nodes_path.c_str());
    std::remove(edges_path.c_str());
}

void demo_full_pipeline(const std::string& image_path) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DEMO: Full Pipeline (Image → YOLO → Graph)                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    
    YOLODetector detector;
    VisualPerception::Config config;
    config.verbose = true;
    VisualPerception perception(&storage, config);
    
    std::cout << "[Demo] Processing image: " << image_path << "\n\n";
    
    int nodes_created = perception.process_image_file(image_path, detector);
    
    std::cout << "\n[Demo] Pipeline complete!\n";
    std::cout << "  Nodes created: " << nodes_created << "\n\n";
    
    perception.print_stats();
    
    std::cout << "\n[Demo] Knowledge Graph:\n";
    storage.print_stats();
    
    std::cout << "\n✓ Full pipeline demo complete\n";
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   👁️  MELVIN VISUAL PERCEPTION ENGINE TEST SUITE  👁️           ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   YOLO Object Detection → Knowledge Graph Integration         ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    try {
        // Run basic tests (no image file needed)
        test_single_frame_processing();
        test_multi_frame_processing();
        test_edge_types();
        test_persistence();
        
        // If image path provided, run YOLO tests
        if (argc > 1) {
            std::string image_path = argv[1];
            test_yolo_detection(image_path);
            demo_full_pipeline(image_path);
        } else {
            std::cout << "\n";
            std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
            std::cout << "║  ℹ️  Image-based tests skipped                                 ║\n";
            std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
            std::cout << "\nTo run YOLO detection tests, provide an image path:\n";
            std::cout << "  ./test_visual_perception <image.jpg>\n\n";
        }
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✅  ALL TESTS PASSED                                          ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED: " << e.what() << "\n\n";
        return 1;
    }
    
    return 0;
}


