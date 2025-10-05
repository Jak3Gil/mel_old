#pragma once

#include "EvaluationAdapter.hpp"
#include "ConfidenceCalibration.hpp"
#include "HuggingFaceIntegration.hpp"
#include "AblationTesting.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <json/json.h>

namespace Melvin::Evaluation {

/**
 * Main Evaluation Framework
 * Orchestrates comprehensive evaluation with data leakage prevention,
 * confidence calibration, error taxonomy, and ablation testing
 */
class EvaluationFramework {
public:
    explicit EvaluationFramework(const std::string& base_dir = "evaluation_results");
    ~EvaluationFramework() = default;
    
    // ==================== CORE EVALUATION INTERFACE ====================
    
    /**
     * Run comprehensive evaluation pipeline
     */
    struct EvaluationConfig {
        std::string dataset_name;
        std::string split = "test";  // Use official test splits only
        int max_samples = 100;
        std::string profile = "Balanced";
        int seed = 42;
        
        // Feature flags
        bool enable_calibration = true;
        bool enable_ablation = false;
        bool enable_robustness = false;
        bool enable_long_context = false;
        bool enable_error_taxonomy = true;
        
        // Calibration settings
        std::string calibration_method = "temperature";
        int cv_folds = 5;
        
        // Ablation settings
        std::vector<AblationTesting::AblationConfig> ablation_configs;
        
        // Robustness settings
        std::vector<RobustnessTesting::RobustnessConfig> robustness_configs;
        
        // Thresholds
        double min_accuracy_threshold = 0.7;
        double max_ece_threshold = 0.08;
        double max_latency_threshold = 50.0;  // ms
        int max_fanout_threshold = 16;
    };
    
    struct EvaluationResults {
        // Core metrics
        std::map<std::string, EvaluationMetrics> dataset_metrics;
        
        // Calibration results
        std::map<std::string, ConfidenceCalibrator::CalibrationMetrics> calibration_metrics;
        
        // Ablation results
        std::map<std::string, std::vector<AblationTesting::AblationResult>> ablation_results;
        
        // Robustness results
        std::map<std::string, std::map<RobustnessTesting::TestType, RobustnessTesting::RobustnessResult>> robustness_results;
        
        // Long-context results
        std::vector<LongContextValidation::LongContextResult> long_context_results;
        
        // Error taxonomy
        std::map<std::string, std::map<std::string, std::vector<ErrorBucket>>> error_taxonomy;
        
        // Run metadata
        HuggingFaceIntegration::RunMeta run_meta;
        
        // Validation status
        bool passes_all_thresholds = false;
        std::vector<std::string> threshold_violations;
        
        // Performance summary
        struct PerformanceSummary {
            double mean_accuracy = 0.0;
            double mean_ece = 0.0;
            double mean_latency_ms = 0.0;
            int total_violations = 0;
            bool ci_gate_passed = false;
        } performance_summary;
    };
    
    // Main evaluation pipeline
    EvaluationResults runEvaluation(const EvaluationConfig& config);
    
    // ==================== DATASET PROCESSING ====================
    
    // Process single dataset
    bool processDataset(const std::string& dataset_name, const std::string& split = "test", int max_samples = 100);
    
    // Process multiple datasets
    bool processMultipleDatasets(const std::vector<std::string>& dataset_names, 
                               const std::string& split = "test", int max_samples = 100);
    
    // Load processed data
    bool loadProcessedData(const std::string& dataset_name, std::vector<GroundTruth>& data);
    
    // ==================== CONFIDENCE CALIBRATION ====================
    
    // Fit calibration for dataset
    bool fitCalibration(const std::string& dataset_name, const std::string& method = "temperature");
    
    // Apply calibration to predictions
    std::vector<PredictionResult> calibratePredictions(const std::string& dataset_name,
                                                     const std::vector<PredictionResult>& predictions);
    
    // ==================== ABLATION TESTING ====================
    
    // Run ablation tests
    void runAblationSuite(const std::string& dataset_name, 
                         const std::vector<GroundTruth>& ground_truth);
    
    // Run component-specific ablations
    void runComponentAblation(const std::string& dataset_name,
                            const std::vector<GroundTruth>& ground_truth,
                            const std::string& component);
    
    // ==================== ROBUSTNESS TESTING ====================
    
    // Run robustness tests
    void runRobustnessSuite(const std::string& dataset_name,
                          const std::vector<GroundTruth>& ground_truth);
    
    // ==================== LONG-CONTEXT VALIDATION ====================
    
    // Run long-context validation
    void runLongContextValidation(const std::vector<std::string>& dataset_names);
    
    // ==================== ERROR TAXONOMY ====================
    
    // Generate error taxonomy report
    void generateErrorTaxonomyReport(const std::string& output_dir);
    
    // ==================== CI GATE SYSTEM ====================
    
    // CI gate validation
    struct CIGateConfig {
        // Accuracy thresholds per dataset
        std::map<std::string, double> accuracy_thresholds = {
            {"commonsense_qa", 0.70},
            {"piqa", 0.70},
            {"gsm8k", 0.20},  // Exact match for math
            {"boolq", 0.75},
            {"openbookqa", 0.65}
        };
        
        // Calibration thresholds
        double max_ece_threshold = 0.08;
        double max_brier_threshold = 0.25;
        
        // Performance thresholds
        double max_latency_threshold = 50.0;  // ms
        int max_fanout_threshold = 16;
        int max_guardrail_violations = 0;
        
        // Robustness thresholds
        double max_paraphrase_drop = 0.03;    // 3%
        double max_noise_drop = 0.05;         // 5%
        double max_distractor_drop = 0.05;    // 5%
        
        // Long-context thresholds
        double min_continuity_gain = 0.15;    // 15%
        double min_persona_consistency = 0.85; // 85%
    };
    
    bool validateCIGate(const EvaluationResults& results, const CIGateConfig& config);
    Json::Value generateResultsSummary(const EvaluationResults& results);
    bool saveResultsSummary(const Json::Value& summary, const std::string& filename = "results_summary.json");
    
    // ==================== REPRODUCIBILITY ====================
    
    // Create replay configuration for failed runs
    ReproducibilityHarness::ReplayConfig createReplayConfig(const std::string& dataset,
                                                          const std::string& item_id,
                                                          const EvaluationConfig& config);
    
    // Replay failed prediction
    PredictionResult replayPrediction(const ReproducibilityHarness::ReplayConfig& config);
    
    // ==================== REPORTING ====================
    
    // Generate comprehensive report
    void generateComprehensiveReport(const EvaluationResults& results, 
                                   const std::string& output_file = "evaluation_report.md");
    
    // Generate JSON report for CI/CD
    Json::Value generateJSONReport(const EvaluationResults& results);
    
    // Generate error report
    void generateErrorReport(const EvaluationResults& results,
                           const std::string& output_dir = "artifacts/errors");
    
    // ==================== UTILITIES ====================
    
    // Access to subsystems
    EvaluationManager& getEvaluationManager() { return evaluation_manager_; }
    ConfidenceCalibrator& getCalibrator() { return calibrator_; }
    HuggingFaceIntegration& getHuggingFaceIntegration() { return hf_integration_; }
    AblationTesting& getAblationTesting() { return ablation_testing_; }
    RobustnessTesting& getRobustnessTesting() { return robustness_testing_; }
    LongContextValidation& getLongContextValidation() { return long_context_validation_; }
    ReproducibilityHarness& getReproducibilityHarness() { return repro_harness_; }
    
    // Configuration management
    void setBaseDirectory(const std::string& base_dir);
    std::string getBaseDirectory() const { return base_dir_; }
    
    // Validation
    bool validateConfiguration(const EvaluationConfig& config);
    std::vector<std::string> getValidationErrors(const EvaluationConfig& config);
    
private:
    std::string base_dir_;
    
    // Core subsystems
    EvaluationManager evaluation_manager_;
    ConfidenceCalibrator calibrator_;
    HuggingFaceIntegration hf_integration_;
    AblationTesting ablation_testing_;
    RobustnessTesting robustness_testing_;
    LongContextValidation long_context_validation_;
    ReproducibilityHarness repro_harness_;
    
    // Helper functions
    bool ensureDirectoryExists(const std::string& dir);
    std::string generateTimestamp();
    Json::Value serializeEvaluationConfig(const EvaluationConfig& config);
    EvaluationConfig deserializeEvaluationConfig(const Json::Value& json);
    
    // Threshold validation
    bool validateAccuracyThresholds(const std::map<std::string, EvaluationMetrics>& metrics,
                                  const std::map<std::string, double>& thresholds);
    bool validateCalibrationThresholds(const std::map<std::string, ConfidenceCalibrator::CalibrationMetrics>& metrics,
                                     double max_ece, double max_brier);
    bool validatePerformanceThresholds(const std::map<std::string, EvaluationMetrics>& metrics,
                                     double max_latency, int max_fanout);
    bool validateRobustnessThresholds(const std::map<RobustnessTesting::TestType, RobustnessTesting::RobustnessResult>& results,
                                    const CIGateConfig& config);
    
    // Report generation helpers
    std::string generateMarkdownTable(const std::map<std::string, EvaluationMetrics>& metrics);
    std::string generateAblationMarkdown(const std::map<std::string, std::vector<AblationTesting::AblationResult>>& results);
    std::string generateRobustnessMarkdown(const std::map<RobustnessTesting::TestType, RobustnessTesting::RobustnessResult>& results);
    
    // Performance aggregation
    EvaluationResults::PerformanceSummary calculatePerformanceSummary(const EvaluationResults& results);
};

/**
 * Command-line interface for evaluation framework
 */
class EvaluationCLI {
public:
    static int run(int argc, char* argv[]);
    
private:
    static void printUsage();
    static void printHelp();
    static EvaluationConfig parseCommandLine(int argc, char* argv[]);
};

/**
 * Factory functions for common evaluation scenarios
 */
namespace EvaluationFactory {
    
    // Quick evaluation for single dataset
    EvaluationFramework::EvaluationResults quickEvaluation(const std::string& dataset_name,
                                                          int max_samples = 100);
    
    // Comprehensive evaluation suite
    EvaluationFramework::EvaluationResults comprehensiveEvaluation(int max_samples = 100);
    
    // CI/CD evaluation pipeline
    bool runCIPipeline(const std::vector<std::string>& datasets = {"commonsense_qa", "piqa", "gsm8k"});
    
    // Development evaluation (fast, minimal)
    EvaluationFramework::EvaluationResults developmentEvaluation(const std::string& dataset_name,
                                                               int max_samples = 50);
    
} // namespace EvaluationFactory

} // namespace Melvin::Evaluation
