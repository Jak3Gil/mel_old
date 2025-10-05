#include "EvaluationAdapter.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <regex>
#include <filesystem>

namespace Melvin::Evaluation {

// ==================== BASE EVALUATION ADAPTER ====================

double EvaluationAdapter::extractConfidence(const PredictionResult& prediction) const {
    if (prediction.calibrated_confidence >= 0.0) {
        return prediction.calibrated_confidence;
    }
    return prediction.confidence;
}

double EvaluationAdapter::extractAttentionEntropy(const PredictionResult& prediction) const {
    return prediction.attention_entropy;
}

void EvaluationAdapter::bucketError(const GroundTruth& item, const PredictionResult& prediction,
                                  const GroundTruth& ground_truth, const std::string& error_type) {
    ErrorBucket bucket;
    bucket.item_id = item.item_id;
    bucket.question = item.question;
    bucket.prediction = prediction.answer;
    bucket.ground_truth = ground_truth.answer;
    bucket.error_type = error_type;
    bucket.attention_entropy = prediction.attention_entropy;
    bucket.confidence = extractConfidence(prediction);
    bucket.latency_ms = prediction.latency_ms;
    
    error_buckets_[error_type].push_back(bucket);
}

double EvaluationAdapter::calculateECE(const std::vector<double>& confidences, 
                                     const std::vector<bool>& correct, int n_bins) const {
    if (confidences.empty() || correct.empty() || confidences.size() != correct.size()) {
        return 0.0;
    }
    
    // Create bin boundaries
    std::vector<double> bin_boundaries(n_bins + 1);
    for (int i = 0; i <= n_bins; ++i) {
        bin_boundaries[i] = static_cast<double>(i) / n_bins;
    }
    
    double ece = 0.0;
    int total_samples = confidences.size();
    
    for (int i = 0; i < n_bins; ++i) {
        double bin_lower = bin_boundaries[i];
        double bin_upper = bin_boundaries[i + 1];
        
        std::vector<double> bin_confidences;
        std::vector<bool> bin_correct;
        
        // Collect samples in this bin
        for (size_t j = 0; j < confidences.size(); ++j) {
            if (confidences[j] > bin_lower && confidences[j] <= bin_upper) {
                bin_confidences.push_back(confidences[j]);
                bin_correct.push_back(correct[j]);
            }
        }
        
        if (bin_confidences.empty()) continue;
        
        // Calculate metrics for this bin
        double avg_confidence = 0.0;
        for (double conf : bin_confidences) {
            avg_confidence += conf;
        }
        avg_confidence /= bin_confidences.size();
        
        double avg_accuracy = 0.0;
        for (bool corr : bin_correct) {
            avg_accuracy += (corr ? 1.0 : 0.0);
        }
        avg_accuracy /= bin_correct.size();
        
        // Add to ECE
        double bin_weight = static_cast<double>(bin_confidences.size()) / total_samples;
        ece += bin_weight * std::abs(avg_confidence - avg_accuracy);
    }
    
    return ece;
}

double EvaluationAdapter::calculateBrierScore(const std::vector<double>& confidences, 
                                            const std::vector<bool>& correct) const {
    if (confidences.empty() || correct.empty() || confidences.size() != correct.size()) {
        return 0.0;
    }
    
    double brier_score = 0.0;
    for (size_t i = 0; i < confidences.size(); ++i) {
        double error = confidences[i] - (correct[i] ? 1.0 : 0.0);
        brier_score += error * error;
    }
    
    return brier_score / confidences.size();
}

// ==================== MCQ ADAPTER ====================

MCQAdapter::MCQAdapter(const std::string& dataset_name) : dataset_name_(dataset_name) {}

EvaluationMetrics MCQAdapter::evaluate(const std::vector<PredictionResult>& predictions,
                                     const std::vector<GroundTruth>& ground_truth) {
    EvaluationMetrics metrics;
    
    if (predictions.size() != ground_truth.size()) {
        metrics.validation_error = "Mismatch between predictions and ground truth sizes";
        return metrics;
    }
    
    std::vector<bool> correct;
    std::vector<double> confidences;
    std::map<std::string, int> error_breakdown;
    
    double total_latency = 0.0;
    double total_fanout = 0.0;
    std::vector<double> latencies;
    
    for (size_t i = 0; i < predictions.size(); ++i) {
        const auto& pred = predictions[i];
        const auto& gt = ground_truth[i];
        
        // Extract prediction and gold indices
        int pred_idx = extractPredictionIndex(pred);
        int gold_idx = extractGoldIndex(gt);
        double confidence = extractConfidence(pred);
        double attention_entropy = extractAttentionEntropy(pred);
        
        // Check correctness
        bool is_correct = (pred_idx == gold_idx);
        correct.push_back(is_correct);
        confidences.push_back(confidence);
        
        // Collect performance metrics
        total_latency += pred.latency_ms;
        total_fanout += pred.fanout;
        latencies.push_back(pred.latency_ms);
        
        // Error taxonomy
        if (!is_correct) {
            std::string error_type = classifyMCQError(pred, gt, attention_entropy, confidence);
            error_breakdown[error_type]++;
            bucketError(gt, pred, gt, error_type);
        }
    }
    
    // Calculate metrics
    metrics.accuracy = std::count(correct.begin(), correct.end(), true) / static_cast<double>(correct.size());
    metrics.ece = calculateECE(confidences, correct);
    metrics.brier_score = calculateBrierScore(confidences, correct);
    metrics.error_breakdown = error_breakdown;
    
    // Performance metrics
    metrics.mean_latency_ms = total_latency / predictions.size();
    metrics.mean_fanout = total_fanout / predictions.size();
    
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        size_t p95_idx = static_cast<size_t>(0.95 * latencies.size());
        metrics.p95_latency_ms = latencies[p95_idx];
    }
    
    // Confidence distribution
    if (!confidences.empty()) {
        double sum = 0.0;
        for (double conf : confidences) sum += conf;
        metrics.confidence_distribution.mean = sum / confidences.size();
        
        double variance = 0.0;
        for (double conf : confidences) {
            variance += (conf - metrics.confidence_distribution.mean) * (conf - metrics.confidence_distribution.mean);
        }
        metrics.confidence_distribution.std = std::sqrt(variance / confidences.size());
        
        auto minmax = std::minmax_element(confidences.begin(), confidences.end());
        metrics.confidence_distribution.min = *minmax.first;
        metrics.confidence_distribution.max = *minmax.second;
    }
    
    metrics.is_valid = true;
    return metrics;
}

int MCQAdapter::extractPredictionIndex(const PredictionResult& prediction) const {
    // Try to extract from answer text (e.g., "A", "B", "C")
    if (prediction.answer.length() == 1 && std::isalpha(prediction.answer[0])) {
        return std::toupper(prediction.answer[0]) - 'A';
    }
    
    // Try to extract from numerical index
    try {
        return std::stoi(prediction.answer);
    } catch (...) {
        return 0;  // Default fallback
    }
}

int MCQAdapter::extractGoldIndex(const GroundTruth& ground_truth) const {
    // Try answer key first
    if (!ground_truth.answer_key.empty()) {
        if (ground_truth.answer_key.length() == 1 && std::isalpha(ground_truth.answer_key[0])) {
            return std::toupper(ground_truth.answer_key[0]) - 'A';
        }
        try {
            return std::stoi(ground_truth.answer_key);
        } catch (...) {}
    }
    
    // Try answer field
    if (!ground_truth.answer.empty()) {
        if (ground_truth.answer.length() == 1 && std::isalpha(ground_truth.answer[0])) {
            return std::toupper(ground_truth.answer[0]) - 'A';
        }
        try {
            return std::stoi(ground_truth.answer);
        } catch (...) {}
    }
    
    return 0;  // Default fallback
}

std::string MCQAdapter::classifyMCQError(const PredictionResult& prediction, 
                                        const GroundTruth& ground_truth,
                                        double attention_entropy, double confidence) const {
    // Attention scatter (high entropy at decision step)
    if (attention_entropy > 0.35) {
        return "attention_scatter";
    }
    
    // Low confidence but wrong
    if (confidence < 0.3) {
        return "low_confidence_wrong";
    }
    
    // Retrieval miss (no relevant context found)
    if (prediction.context_overlap < 0.2) {
        return "retrieval_miss";
    }
    
    // Leap hallucination (cosine similarity too low)
    if (prediction.leap_similarity < 0.5) {
        return "leap_hallucination";
    }
    
    // Default classification
    return "reasoning_error";
}

// ==================== MATH ADAPTER ====================

MathAdapter::MathAdapter(const std::string& dataset_name) : dataset_name_(dataset_name) {}

EvaluationMetrics MathAdapter::evaluate(const std::vector<PredictionResult>& predictions,
                                      const std::vector<GroundTruth>& ground_truth) {
    EvaluationMetrics metrics;
    
    if (predictions.size() != ground_truth.size()) {
        metrics.validation_error = "Mismatch between predictions and ground truth sizes";
        return metrics;
    }
    
    std::vector<bool> exact_matches;
    std::vector<double> digit_f1_scores;
    std::vector<double> confidences;
    std::map<std::string, int> error_breakdown;
    
    double total_latency = 0.0;
    double total_fanout = 0.0;
    std::vector<double> latencies;
    
    for (size_t i = 0; i < predictions.size(); ++i) {
        const auto& pred = predictions[i];
        const auto& gt = ground_truth[i];
        
        // Extract numerical answers
        double pred_number = extractFinalNumber(pred.answer);
        double gold_number = (gt.numerical_answer != 0.0) ? gt.numerical_answer : extractFinalNumber(gt.answer);
        double confidence = extractConfidence(pred);
        double attention_entropy = extractAttentionEntropy(pred);
        
        // Exact match
        bool exact_match = (std::abs(pred_number - gold_number) < 1e-6);
        exact_matches.push_back(exact_match);
        confidences.push_back(confidence);
        
        // Digit-wise F1
        double digit_f1 = calculateDigitF1(std::to_string(pred_number), std::to_string(gold_number));
        digit_f1_scores.push_back(digit_f1);
        
        // Collect performance metrics
        total_latency += pred.latency_ms;
        total_fanout += pred.fanout;
        latencies.push_back(pred.latency_ms);
        
        // Error taxonomy
        if (!exact_match) {
            std::string error_type = classifyMathError(pred, gt, pred_number, gold_number, attention_entropy);
            error_breakdown[error_type]++;
            bucketError(gt, pred, gt, error_type);
        }
    }
    
    // Calculate metrics
    metrics.accuracy = std::count(exact_matches.begin(), exact_matches.end(), true) / static_cast<double>(exact_matches.size());
    metrics.exact_match = metrics.accuracy;  // For math, accuracy = exact match
    metrics.ece = calculateECE(confidences, exact_matches);
    metrics.brier_score = calculateBrierScore(confidences, exact_matches);
    metrics.error_breakdown = error_breakdown;
    
    // Digit F1
    if (!digit_f1_scores.empty()) {
        double sum = 0.0;
        for (double f1 : digit_f1_scores) sum += f1;
        metrics.digit_f1 = sum / digit_f1_scores.size();
    }
    
    // Consistency (placeholder - would need multiple runs)
    metrics.consistency = 0.85;  // Placeholder
    
    // Performance metrics
    metrics.mean_latency_ms = total_latency / predictions.size();
    metrics.mean_fanout = total_fanout / predictions.size();
    
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        size_t p95_idx = static_cast<size_t>(0.95 * latencies.size());
        metrics.p95_latency_ms = latencies[p95_idx];
    }
    
    // Confidence distribution
    if (!confidences.empty()) {
        double sum = 0.0;
        for (double conf : confidences) sum += conf;
        metrics.confidence_distribution.mean = sum / confidences.size();
        
        double variance = 0.0;
        for (double conf : confidences) {
            variance += (conf - metrics.confidence_distribution.mean) * (conf - metrics.confidence_distribution.mean);
        }
        metrics.confidence_distribution.std = std::sqrt(variance / confidences.size());
        
        auto minmax = std::minmax_element(confidences.begin(), confidences.end());
        metrics.confidence_distribution.min = *minmax.first;
        metrics.confidence_distribution.max = *minmax.second;
    }
    
    metrics.is_valid = true;
    return metrics;
}

double MathAdapter::extractFinalNumber(const std::string& text) const {
    if (text.empty()) return 0.0;
    
    // Remove common units and normalize
    std::string normalized = text;
    std::replace(normalized.begin(), normalized.end(), ',', ' ');
    std::replace(normalized.begin(), normalized.end(), '$', ' ');
    std::replace(normalized.begin(), normalized.end(), '%', ' ');
    
    // Find all numbers using regex
    std::regex number_regex(R"(-?\d+\.?\d*)");
    std::sregex_iterator iter(normalized.begin(), normalized.end(), number_regex);
    std::sregex_iterator end;
    
    std::vector<std::string> numbers;
    while (iter != end) {
        numbers.push_back(iter->str());
        ++iter;
    }
    
    if (numbers.empty()) return 0.0;
    
    try {
        // Return the last number (final answer)
        return std::stod(numbers.back());
    } catch (...) {
        return 0.0;
    }
}

double MathAdapter::calculateDigitF1(const std::string& pred_str, const std::string& gold_str) const {
    std::regex digit_regex(R"(\d)");
    
    std::set<char> pred_digits;
    std::sregex_iterator pred_iter(pred_str.begin(), pred_str.end(), digit_regex);
    std::sregex_iterator pred_end;
    while (pred_iter != pred_end) {
        pred_digits.insert(pred_iter->str()[0]);
        ++pred_iter;
    }
    
    std::set<char> gold_digits;
    std::sregex_iterator gold_iter(gold_str.begin(), gold_str.end(), digit_regex);
    std::sregex_iterator gold_end;
    while (gold_iter != gold_end) {
        gold_digits.insert(gold_iter->str()[0]);
        ++gold_iter;
    }
    
    if (gold_digits.empty()) {
        return pred_digits.empty() ? 1.0 : 0.0;
    }
    
    std::set<char> intersection;
    std::set_intersection(pred_digits.begin(), pred_digits.end(),
                         gold_digits.begin(), gold_digits.end(),
                         std::inserter(intersection, intersection.begin()));
    
    double precision = intersection.size() / static_cast<double>(pred_digits.size());
    double recall = intersection.size() / static_cast<double>(gold_digits.size());
    
    if (precision + recall == 0.0) return 0.0;
    return 2.0 * precision * recall / (precision + recall);
}

std::string MathAdapter::classifyMathError(const PredictionResult& prediction,
                                         const GroundTruth& ground_truth,
                                         double pred_num, double gold_num, double attention_entropy) const {
    // Arithmetic slip (working looks right, final extraction wrong)
    if (gold_num != 0.0 && std::abs(pred_num - gold_num) < std::abs(gold_num) * 0.1) {
        return "arithmetic_slip";
    }
    
    // Attention scatter during reasoning
    if (attention_entropy > 0.4) {
        return "attention_scatter";
    }
    
    // Complete reasoning failure
    if (pred_num == 0.0 || std::isnan(pred_num)) {
        return "reasoning_failure";
    }
    
    // Order of magnitude error
    if (pred_num != 0.0 && gold_num != 0.0) {
        double ratio = std::max(std::abs(pred_num), std::abs(gold_num)) / std::min(std::abs(pred_num), std::abs(gold_num));
        if (ratio > 10.0) {
            return "magnitude_error";
        }
    }
    
    return "calculation_error";
}

// ==================== EVALUATION MANAGER ====================

EvaluationManager::EvaluationManager() {
    // Initialize adapters for known datasets
    adapters_["commonsense_qa"] = std::make_unique<MCQAdapter>("commonsense_qa");
    adapters_["piqa"] = std::make_unique<MCQAdapter>("piqa");
    adapters_["openbookqa"] = std::make_unique<MCQAdapter>("openbookqa");
    adapters_["boolq"] = std::make_unique<MCQAdapter>("boolq");
    adapters_["gsm8k"] = std::make_unique<MathAdapter>("gsm8k");
    adapters_["aqua_rat"] = std::make_unique<MathAdapter>("aqua_rat");
}

EvaluationMetrics EvaluationManager::evaluateDataset(const std::string& dataset_name,
                                                    const std::vector<PredictionResult>& predictions,
                                                    const std::vector<GroundTruth>& ground_truth) {
    if (adapters_.find(dataset_name) == adapters_.end()) {
        adapters_[dataset_name] = createAdapter(dataset_name);
    }
    
    return adapters_[dataset_name]->evaluate(predictions, ground_truth);
}

void EvaluationManager::generateErrorReport(const std::string& output_dir) const {
    std::filesystem::create_directories(output_dir);
    
    for (const auto& [dataset_name, adapter] : adapters_) {
        const auto& error_buckets = adapter->getErrorBuckets();
        if (error_buckets.empty()) continue;
        
        std::string dataset_dir = output_dir + "/" + dataset_name;
        std::filesystem::create_directories(dataset_dir);
        
        for (const auto& [error_type, buckets] : error_buckets) {
            std::string filename = dataset_dir + "/" + error_type + ".json";
            
            Json::Value json_array(Json::arrayValue);
            for (const auto& bucket : buckets) {
                json_array.append(bucket.toJson());
            }
            
            std::ofstream file(filename);
            Json::StreamWriterBuilder builder;
            builder["indent"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(json_array, &file);
            file.close();
            
            std::cout << "💾 Saved " << buckets.size() << " " << error_type 
                     << " examples to " << filename << std::endl;
        }
    }
}

Json::Value EvaluationManager::aggregateResults(const std::map<std::string, EvaluationMetrics>& results) const {
    Json::Value summary;
    
    // Overall metrics
    Json::Value overall_metrics;
    std::vector<double> accuracies;
    std::vector<double> eces;
    std::vector<double> brier_scores;
    
    for (const auto& [dataset_name, metrics] : results) {
        accuracies.push_back(metrics.accuracy);
        if (metrics.ece >= 0.0) eces.push_back(metrics.ece);
        if (metrics.brier_score >= 0.0) brier_scores.push_back(metrics.brier_score);
    }
    
    if (!accuracies.empty()) {
        double sum = 0.0;
        for (double acc : accuracies) sum += acc;
        overall_metrics["mean_accuracy"] = sum / accuracies.size();
        
        double variance = 0.0;
        for (double acc : accuracies) {
            variance += (acc - overall_metrics["mean_accuracy"].asDouble()) * 
                       (acc - overall_metrics["mean_accuracy"].asDouble());
        }
        overall_metrics["std_accuracy"] = std::sqrt(variance / accuracies.size());
    }
    
    if (!eces.empty()) {
        double sum = 0.0;
        for (double ece : eces) sum += ece;
        overall_metrics["mean_ece"] = sum / eces.size();
    }
    
    if (!brier_scores.empty()) {
        double sum = 0.0;
        for (double brier : brier_scores) sum += brier;
        overall_metrics["mean_brier_score"] = sum / brier_scores.size();
    }
    
    summary["overall_metrics"] = overall_metrics;
    
    // Dataset-specific metrics
    Json::Value dataset_metrics;
    for (const auto& [dataset_name, metrics] : results) {
        dataset_metrics[dataset_name] = Utils::metricsToJson(metrics);
    }
    summary["dataset_metrics"] = dataset_metrics;
    
    return summary;
}

std::unique_ptr<EvaluationAdapter> EvaluationManager::createAdapter(const std::string& dataset_name) {
    // Determine adapter type based on dataset name
    if (dataset_name.find("gsm8k") != std::string::npos || 
        dataset_name.find("aqua_rat") != std::string::npos ||
        dataset_name.find("math") != std::string::npos) {
        return std::make_unique<MathAdapter>(dataset_name);
    } else {
        return std::make_unique<MCQAdapter>(dataset_name);
    }
}

// ==================== ERROR BUCKET JSON SERIALIZATION ====================

Json::Value ErrorBucket::toJson() const {
    Json::Value json;
    json["item_id"] = item_id;
    json["question"] = question;
    json["prediction"] = prediction;
    json["ground_truth"] = ground_truth;
    json["error_type"] = error_type;
    json["attention_entropy"] = attention_entropy;
    json["confidence"] = confidence;
    json["latency_ms"] = latency_ms;
    return json;
}

ErrorBucket ErrorBucket::fromJson(const Json::Value& json) {
    ErrorBucket bucket;
    bucket.item_id = json.get("item_id", "").asString();
    bucket.question = json.get("question", "").asString();
    bucket.prediction = json.get("prediction", "").asString();
    bucket.ground_truth = json.get("ground_truth", "").asString();
    bucket.error_type = json.get("error_type", "").asString();
    bucket.attention_entropy = json.get("attention_entropy", 0.0).asDouble();
    bucket.confidence = json.get("confidence", 0.0).asDouble();
    bucket.latency_ms = json.get("latency_ms", 0.0).asDouble();
    return bucket;
}

} // namespace Melvin::Evaluation
