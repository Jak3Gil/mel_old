#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <json/json.h>

namespace Melvin::Evaluation {

// Forward declarations
struct PredictionResult;
struct GroundTruth;

/**
 * HuggingFace Dataset Integration
 * Downloads and processes datasets with proper test splits to prevent data leakage
 */
class HuggingFaceIntegration {
public:
    explicit HuggingFaceIntegration(const std::string& output_dir = "huggingface_data");
    ~HuggingFaceIntegration() = default;
    
    // Dataset processing
    struct DatasetConfig {
        std::string name;
        std::string split;
        int max_samples;
        bool official_split_only = true;
    };
    
    bool processDataset(const DatasetConfig& config);
    bool processMultipleDatasets(const std::vector<DatasetConfig>& configs);
    
    // Official test splits (prevents data leakage)
    static const std::map<std::string, std::vector<std::string>> OFFICIAL_SPLITS;
    
    // Dataset-specific processors
    std::vector<GroundTruth> processCommonsenseQA(const Json::Value& dataset);
    std::vector<GroundTruth> processPIQA(const Json::Value& dataset);
    std::vector<GroundTruth> processGSM8K(const Json::Value& dataset);
    std::vector<GroundTruth> processBoolQ(const Json::Value& dataset);
    std::vector<GroundTruth> processOpenBookQA(const Json::Value& dataset);
    
    // Run metadata (prevents data leakage)
    struct RunMeta {
        std::string experiment_id;
        std::string timestamp;
        std::string dataset_name;
        std::string split;
        int max_samples;
        std::string commit_hash;
        std::string commit_message;
        std::string profile;
        int seed;
        bool data_leakage_prevention = true;
        bool calibration_enabled = false;
        bool error_taxonomy_enabled = true;
    };
    
    RunMeta createRunMeta(const std::string& dataset_name, const std::string& split,
                         int max_samples, const std::string& profile = "", int seed = 42);
    bool saveRunMeta(const RunMeta& meta);
    
    // File I/O
    bool saveProcessedData(const std::string& dataset_name, 
                          const std::vector<GroundTruth>& data);
    bool loadProcessedData(const std::string& dataset_name, 
                          std::vector<GroundTruth>& data);
    
    // Test script generation
    std::string generateTestScript(const std::string& dataset_name, 
                                 const std::vector<GroundTruth>& data);
    bool saveTestScript(const std::string& script_content, 
                       const std::string& filename);
    
private:
    std::string output_dir_;
    RunMeta current_run_meta_;
    
    // Helper functions
    bool downloadDataset(const std::string& dataset_name, const std::string& split);
    bool validateSplit(const std::string& dataset_name, const std::string& split);
    std::string getGitCommitHash();
    std::string getCurrentTimestamp();
    
    // Data conversion
    GroundTruth convertCommonsenseQAItem(const Json::Value& item, int index);
    GroundTruth convertPIQAItem(const Json::Value& item, int index);
    GroundTruth convertGSM8KItem(const Json::Value& item, int index);
    GroundTruth convertBoolQItem(const Json::Value& item, int index);
    GroundTruth convertOpenBookQAItem(const Json::Value& item, int index);
    
    // File paths
    std::string getJsonPath(const std::string& dataset_name) const;
    std::string getTxtPath(const std::string& dataset_name) const;
    std::string getCsvPath(const std::string& dataset_name) const;
    std::string getScriptPath(const std::string& dataset_name) const;
};

/**
 * Few-shot exemplar management
 * Pins exemplars in separate file to prevent overlap with test items
 */
class FewShotManager {
public:
    explicit FewShotManager(const std::string& exemplars_dir = "exemplars");
    ~FewShotManager() = default;
    
    // Exemplar management
    bool loadExemplars(const std::string& dataset_name);
    bool saveExemplars(const std::string& dataset_name, 
                      const std::vector<GroundTruth>& exemplars);
    
    // Few-shot formatting
    std::string formatFewShot(const GroundTruth& item, int num_exemplars = 3) const;
    std::vector<GroundTruth> getExemplars(const std::string& dataset_name, int count) const;
    
    // Validation
    bool validateExemplarOverlap(const std::vector<GroundTruth>& test_items,
                               const std::vector<GroundTruth>& exemplars) const;
    
private:
    std::string exemplars_dir_;
    std::map<std::string, std::vector<GroundTruth>> exemplars_cache_;
    
    // Helper functions
    std::string exemplarsToJson(const std::vector<GroundTruth>& exemplars) const;
    std::vector<GroundTruth> exemplarsFromJson(const Json::Value& json) const;
    std::string generateFewShotPrompt(const std::vector<GroundTruth>& exemplars,
                                    const GroundTruth& item) const;
};

/**
 * Data leakage prevention utilities
 */
namespace DataLeakagePrevention {
    
    // Split validation
    bool validateOfficialSplit(const std::string& dataset_name, const std::string& split);
    std::vector<std::string> getOfficialSplits(const std::string& dataset_name);
    
    // Content overlap detection
    double calculateContentOverlap(const std::string& text1, const std::string& text2);
    bool detectOverlap(const std::vector<GroundTruth>& train_data,
                      const std::vector<GroundTruth>& test_data,
                      double threshold = 0.8);
    
    // Hash-based deduplication
    std::string computeContentHash(const std::string& content);
    std::set<std::string> extractContentHashes(const std::vector<GroundTruth>& data);
    bool checkHashCollision(const std::set<std::string>& train_hashes,
                          const std::set<std::string>& test_hashes);
    
    // Metadata validation
    bool validateRunMeta(const HuggingFaceIntegration::RunMeta& meta);
    bool validateDataIntegrity(const std::vector<GroundTruth>& data);
    
} // namespace DataLeakagePrevention

/**
 * Dataset statistics and validation
 */
namespace DatasetStats {
    
    struct DatasetStatistics {
        std::string dataset_name;
        std::string split;
        int total_items;
        int valid_items;
        double avg_question_length;
        double avg_answer_length;
        std::map<std::string, int> answer_distribution;
        std::vector<std::string> validation_errors;
    };
    
    DatasetStatistics analyzeDataset(const std::vector<GroundTruth>& data,
                                   const std::string& dataset_name,
                                   const std::string& split);
    
    bool validateDatasetQuality(const DatasetStatistics& stats);
    void printDatasetReport(const DatasetStatistics& stats);
    
} // namespace DatasetStats

} // namespace Melvin::Evaluation
