#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <json/json.h>

namespace Melvin::Evaluation {

// Forward declarations
struct PredictionResult;
struct GroundTruth;

/**
 * Temperature Scaling for confidence calibration
 * Ensures that "80% confidence" actually means 0.8 probability of being correct
 */
class TemperatureScaling {
public:
    explicit TemperatureScaling(double temperature = 1.0);
    ~TemperatureScaling() = default;
    
    // Core calibration interface
    void fit(const std::vector<std::vector<double>>& logits, 
             const std::vector<int>& labels,
             const std::string& method = "temperature");
    
    std::vector<double> calibrate(const std::vector<double>& logits) const;
    std::vector<std::vector<double>> calibrate(const std::vector<std::vector<double>>& logits) const;
    
    // Accessors
    double getTemperature() const { return temperature_; }
    bool isFitted() const { return is_fitted_; }
    
    // Serialization
    Json::Value toJson() const;
    static TemperatureScaling fromJson(const Json::Value& json);
    
private:
    double temperature_;
    bool is_fitted_;
    
    // Helper functions
    std::vector<double> softmax(const std::vector<double>& logits) const;
    double negativeLogLikelihood(const std::vector<std::vector<double>>& logits,
                               const std::vector<int>& labels,
                               double temp) const;
    
    // Optimization (simple gradient descent)
    double optimizeTemperature(const std::vector<std::vector<double>>& logits,
                             const std::vector<int>& labels);
};

/**
 * Main confidence calibration system
 * Manages calibration across multiple datasets and methods
 */
class ConfidenceCalibrator {
public:
    explicit ConfidenceCalibrator(const std::string& calibration_dir = "calibration");
    ~ConfidenceCalibrator() = default;
    
    // Calibration fitting
    void fitCalibration(const std::string& dataset_name,
                       const std::vector<PredictionResult>& predictions,
                       const std::vector<GroundTruth>& ground_truth,
                       const std::string& method = "temperature",
                       int cv_folds = 5);
    
    // Apply calibration
    std::vector<PredictionResult> calibratePredictions(const std::string& dataset_name,
                                                      const std::vector<PredictionResult>& predictions) const;
    
    // Load/Save calibration
    bool loadCalibration(const std::string& dataset_name);
    void saveCalibration(const std::string& dataset_name) const;
    
    // Metrics calculation
    struct CalibrationMetrics {
        double ece = -1.0;           // Expected Calibration Error
        double brier_score = -1.0;   // Brier Score
        double cv_ece_mean = -1.0;   // Cross-validation ECE mean
        double cv_ece_std = -1.0;    // Cross-validation ECE std
        std::string method;
        double temperature = -1.0;
        bool fitted = false;
    };
    
    const CalibrationMetrics& getMetrics(const std::string& dataset_name) const;
    bool hasCalibration(const std::string& dataset_name) const;
    
    // Calibration validation
    void validateCalibration(const std::string& dataset_name,
                           const std::vector<PredictionResult>& predictions,
                           const std::vector<GroundTruth>& ground_truth) const;
    
    // Report generation
    Json::Value generateCalibrationReport() const;
    void saveCalibrationReport(const std::string& output_file) const;
    
private:
    std::string calibration_dir_;
    std::map<std::string, std::unique_ptr<TemperatureScaling>> calibrators_;
    std::map<std::string, CalibrationMetrics> metrics_;
    
    // Helper functions
    struct CalibrationFeatures {
        std::vector<std::vector<double>> logits;
        std::vector<int> labels;
        std::vector<double> confidences;
    };
    
    CalibrationFeatures extractCalibrationFeatures(const std::vector<PredictionResult>& predictions,
                                                 const std::vector<GroundTruth>& ground_truth,
                                                 const std::string& dataset_name) const;
    
    double calculateECE(const std::vector<double>& confidences,
                       const std::vector<bool>& correct,
                       int n_bins = 10) const;
    
    double calculateBrierScore(const std::vector<double>& confidences,
                             const std::vector<bool>& correct) const;
    
    // Cross-validation
    struct CVResult {
        double mean_ece;
        double std_ece;
        std::vector<double> fold_eces;
    };
    
    CVResult crossValidate(const std::vector<std::vector<double>>& logits,
                          const std::vector<int>& labels,
                          const std::string& method,
                          int cv_folds) const;
    
    // Dataset-specific helpers
    int extractMCQIndex(const std::string& answer) const;
    double extractNumber(const std::string& text) const;
    std::vector<double> confidenceToLogits(double confidence) const;
};

/**
 * Utility functions for calibration
 */
namespace CalibrationUtils {
    
    // Statistical functions
    double calculatePercentile(const std::vector<double>& values, double percentile);
    double calculateMean(const std::vector<double>& values);
    double calculateStd(const std::vector<double>& values);
    
    // Logits manipulation
    std::vector<double> logitsToProbabilities(const std::vector<double>& logits);
    double probabilitiesToConfidence(const std::vector<double>& probabilities);
    std::vector<double> confidenceToBinaryLogits(double confidence);
    
    // Validation
    bool validateLogits(const std::vector<std::vector<double>>& logits);
    bool validateLabels(const std::vector<int>& labels, int num_classes);
    bool validateConfidences(const std::vector<double>& confidences);
    
    // File I/O
    bool saveCalibrationData(const std::string& filename, const Json::Value& data);
    bool loadCalibrationData(const std::string& filename, Json::Value& data);
    
} // namespace CalibrationUtils

} // namespace Melvin::Evaluation
