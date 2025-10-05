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
struct EvaluationMetrics;
struct ErrorBucket;

/**
 * Base class for dataset-specific evaluation adapters
 * Implements exact metrics for reliable benchmarking
 */
class EvaluationAdapter {
public:
    virtual ~EvaluationAdapter() = default;
    
    // Core evaluation interface
    virtual EvaluationMetrics evaluate(const std::vector<PredictionResult>& predictions,
                                     const std::vector<GroundTruth>& ground_truth) = 0;
    
    // Confidence calibration
    virtual double extractConfidence(const PredictionResult& prediction) const;
    virtual double extractAttentionEntropy(const PredictionResult& prediction) const;
    
    // Error taxonomy
    virtual void bucketError(const GroundTruth& item, const PredictionResult& prediction,
                           const GroundTruth& ground_truth, const std::string& error_type);
    
    // Statistical metrics
    double calculateECE(const std::vector<double>& confidences, 
                       const std::vector<bool>& correct, int n_bins = 10) const;
    double calculateBrierScore(const std::vector<double>& confidences, 
                              const std::vector<bool>& correct) const;
    
    // Error bucketing
    const std::map<std::string, std::vector<ErrorBucket>>& getErrorBuckets() const { return error_buckets_; }
    void clearErrorBuckets() { error_buckets_.clear(); }
    
protected:
    std::map<std::string, std::vector<ErrorBucket>> error_buckets_;
    
    // Helper functions
    std::string classifyError(const PredictionResult& prediction, const GroundTruth& ground_truth) const;
};

/**
 * Adapter for Multiple Choice Questions (CommonsenseQA, PIQA, OpenBookQA, BoolQ)
 */
class MCQAdapter : public EvaluationAdapter {
public:
    explicit MCQAdapter(const std::string& dataset_name);
    
    EvaluationMetrics evaluate(const std::vector<PredictionResult>& predictions,
                             const std::vector<GroundTruth>& ground_truth) override;
    
private:
    std::string dataset_name_;
    
    // MCQ-specific helpers
    int extractPredictionIndex(const PredictionResult& prediction) const;
    int extractGoldIndex(const GroundTruth& ground_truth) const;
    std::string classifyMCQError(const PredictionResult& prediction, 
                                const GroundTruth& ground_truth,
                                double attention_entropy, double confidence) const;
};

/**
 * Adapter for Mathematical Reasoning (GSM8K, AQUA-RAT)
 */
class MathAdapter : public EvaluationAdapter {
public:
    explicit MathAdapter(const std::string& dataset_name);
    
    EvaluationMetrics evaluate(const std::vector<PredictionResult>& predictions,
                             const std::vector<GroundTruth>& ground_truth) override;
    
private:
    std::string dataset_name_;
    
    // Math-specific helpers
    double extractFinalNumber(const std::string& text) const;
    double calculateDigitF1(const std::string& pred_str, const std::string& gold_str) const;
    std::string classifyMathError(const PredictionResult& prediction,
                                 const GroundTruth& ground_truth,
                                 double pred_num, double gold_num, double attention_entropy) const;
};

/**
 * Data structures for evaluation
 */
struct PredictionResult {
    std::string item_id;
    std::string answer;
    double confidence = 0.5;
    double attention_entropy = 0.2;
    double top2_margin = 0.0;
    double latency_ms = 0.0;
    int fanout = 0;
    
    // Additional metadata
    std::map<std::string, double> logits;
    double context_overlap = 0.0;
    double leap_similarity = 1.0;
    
    // Calibration
    double calibrated_confidence = -1.0;  // -1 means not calibrated
};

struct GroundTruth {
    std::string item_id;
    std::string answer;
    std::string answer_key;
    std::string question;
    std::vector<std::string> choices;
    
    // Math-specific
    double numerical_answer = 0.0;
    
    // Additional context
    std::string passage;  // For BoolQ
    std::string goal;     // For PIQA
};

struct EvaluationMetrics {
    // Core metrics
    double accuracy = 0.0;
    double calibrated_accuracy = -1.0;  // -1 means not calibrated
    
    // Calibration metrics
    double ece = -1.0;           // Expected Calibration Error
    double brier_score = -1.0;   // Brier Score
    
    // Math-specific metrics
    double exact_match = -1.0;   // -1 means not applicable
    double digit_f1 = -1.0;      // -1 means not applicable
    double consistency = -1.0;   // -1 means not applicable
    
    // Error breakdown
    std::map<std::string, int> error_breakdown;
    
    // Confidence distribution
    struct ConfidenceDistribution {
        double mean = 0.0;
        double std = 0.0;
        double min = 0.0;
        double max = 0.0;
    } confidence_distribution;
    
    // Performance metrics
    double mean_latency_ms = 0.0;
    double p95_latency_ms = 0.0;
    double mean_fanout = 0.0;
    
    // Validation
    bool is_valid = false;
    std::string validation_error;
};

struct ErrorBucket {
    std::string item_id;
    std::string question;
    std::string prediction;
    std::string ground_truth;
    std::string error_type;
    double attention_entropy;
    double confidence;
    double latency_ms;
    
    // JSON serialization
    Json::Value toJson() const;
    static ErrorBucket fromJson(const Json::Value& json);
};

/**
 * Evaluation Manager - coordinates evaluation across datasets
 */
class EvaluationManager {
public:
    EvaluationManager();
    ~EvaluationManager() = default;
    
    // Dataset evaluation
    EvaluationMetrics evaluateDataset(const std::string& dataset_name,
                                    const std::vector<PredictionResult>& predictions,
                                    const std::vector<GroundTruth>& ground_truth);
    
    // Error reporting
    void generateErrorReport(const std::string& output_dir) const;
    
    // Results aggregation
    Json::Value aggregateResults(const std::map<std::string, EvaluationMetrics>& results) const;
    
    // Calibration support
    void setCalibrationEnabled(bool enabled) { calibration_enabled_ = enabled; }
    bool isCalibrationEnabled() const { return calibration_enabled_; }
    
private:
    std::map<std::string, std::unique_ptr<EvaluationAdapter>> adapters_;
    bool calibration_enabled_ = false;
    
    // Helper functions
    std::unique_ptr<EvaluationAdapter> createAdapter(const std::string& dataset_name);
};

/**
 * Utility functions for data loading and processing
 */
namespace Utils {
    
    // JSON serialization
    Json::Value predictionToJson(const PredictionResult& prediction);
    Json::Value groundTruthToJson(const GroundTruth& ground_truth);
    Json::Value metricsToJson(const EvaluationMetrics& metrics);
    
    PredictionResult predictionFromJson(const Json::Value& json);
    GroundTruth groundTruthFromJson(const Json::Value& json);
    EvaluationMetrics metricsFromJson(const Json::Value& json);
    
    // File I/O
    bool loadPredictionsFromFile(const std::string& filename, std::vector<PredictionResult>& predictions);
    bool loadGroundTruthFromFile(const std::string& filename, std::vector<GroundTruth>& ground_truth);
    bool saveMetricsToFile(const std::string& filename, const EvaluationMetrics& metrics);
    
    // Data validation
    bool validatePredictions(const std::vector<PredictionResult>& predictions);
    bool validateGroundTruth(const std::vector<GroundTruth>& ground_truth);
    bool validateDataConsistency(const std::vector<PredictionResult>& predictions,
                               const std::vector<GroundTruth>& ground_truth);
    
    // Statistical helpers
    double calculatePercentile(const std::vector<double>& values, double percentile);
    std::vector<double> calculateConfidenceDistribution(const std::vector<PredictionResult>& predictions);
    
} // namespace Utils

} // namespace Melvin::Evaluation
