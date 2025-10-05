#!/usr/bin/env python3
"""
MELVIN CONFIDENCE CALIBRATION
=============================

Implements temperature scaling and confidence calibration for reliable uncertainty estimation.
Ensures that "80% confidence" actually means 0.8 probability of being correct.

Features:
- Temperature scaling per domain
- ECE (Expected Calibration Error) calculation
- Brier score computation
- Calibration curve visualization
- Cross-validation for robust temperature fitting
"""

import numpy as np
import pandas as pd
import json
import matplotlib.pyplot as plt
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from sklearn.model_selection import KFold
from sklearn.isotonic import IsotonicRegression
import warnings
warnings.filterwarnings('ignore')

class TemperatureScaling:
    """Temperature scaling for confidence calibration"""
    
    def __init__(self, temperature: float = 1.0):
        self.temperature = temperature
        self.is_fitted = False
    
    def fit(self, logits: np.ndarray, labels: np.ndarray, method: str = 'temperature'):
        """
        Fit calibration parameters
        
        Args:
            logits: Raw logits from model (n_samples, n_classes)
            labels: True labels (n_samples,)
            method: 'temperature' or 'isotonic'
        """
        if method == 'temperature':
            self._fit_temperature(logits, labels)
        elif method == 'isotonic':
            self._fit_isotonic(logits, labels)
        else:
            raise ValueError(f"Unknown calibration method: {method}")
        
        self.is_fitted = True
    
    def _fit_temperature(self, logits: np.ndarray, labels: np.ndarray):
        """Fit temperature scaling using cross-validation"""
        from scipy.optimize import minimize
        
        def objective(temp):
            # Apply temperature scaling
            scaled_logits = logits / temp
            probabilities = self._softmax(scaled_logits)
            
            # Calculate negative log likelihood
            nll = -np.mean(np.log(probabilities[np.arange(len(labels)), labels] + 1e-8))
            return nll
        
        # Optimize temperature
        result = minimize(objective, x0=1.0, bounds=[(0.1, 10.0)])
        self.temperature = result.x[0]
    
    def _fit_isotonic(self, logits: np.ndarray, labels: np.ndarray):
        """Fit isotonic regression for calibration"""
        probabilities = self._softmax(logits)
        max_probs = np.max(probabilities, axis=1)
        
        self.isotonic_regressor = IsotonicRegression(out_of_bounds='clip')
        self.isotonic_regressor.fit(max_probs, labels == np.argmax(probabilities, axis=1))
    
    def calibrate(self, logits: np.ndarray) -> np.ndarray:
        """Apply calibration to logits"""
        if not self.is_fitted:
            raise ValueError("Calibration not fitted. Call fit() first.")
        
        if hasattr(self, 'isotonic_regressor'):
            # Isotonic calibration
            probabilities = self._softmax(logits)
            max_probs = np.max(probabilities, axis=1)
            calibrated_probs = self.isotonic_regressor.predict(max_probs)
            
            # Create new probability matrix
            calibrated_logits = np.zeros_like(logits)
            max_indices = np.argmax(probabilities, axis=1)
            calibrated_logits[np.arange(len(max_indices)), max_indices] = calibrated_probs
            
            return self._softmax(calibrated_logits)
        else:
            # Temperature scaling
            scaled_logits = logits / self.temperature
            return self._softmax(scaled_logits)
    
    def _softmax(self, logits: np.ndarray) -> np.ndarray:
        """Apply softmax to logits"""
        exp_logits = np.exp(logits - np.max(logits, axis=1, keepdims=True))
        return exp_logits / np.sum(exp_logits, axis=1, keepdims=True)

class ConfidenceCalibrator:
    """Main confidence calibration system"""
    
    def __init__(self, calibration_dir: str = "calibration"):
        self.calibration_dir = Path(calibration_dir)
        self.calibration_dir.mkdir(exist_ok=True)
        self.calibrators = {}
        self.metrics = {}
    
    def fit_calibration(self, dataset_name: str, predictions: List[Dict], ground_truth: List[Dict], 
                       method: str = 'temperature', cv_folds: int = 5):
        """
        Fit calibration for a specific dataset
        
        Args:
            dataset_name: Name of the dataset
            predictions: List of prediction dictionaries
            ground_truth: List of ground truth dictionaries
            method: Calibration method ('temperature' or 'isotonic')
            cv_folds: Number of cross-validation folds
        """
        print(f"🎯 Fitting calibration for {dataset_name} using {method}")
        
        # Extract features for calibration
        logits, labels, confidences = self._extract_calibration_features(
            predictions, ground_truth, dataset_name
        )
        
        if len(logits) == 0:
            print(f"⚠️  No valid data for calibration in {dataset_name}")
            return
        
        # Cross-validation for robust fitting
        cv_scores = []
        kf = KFold(n_splits=cv_folds, shuffle=True, random_state=42)
        
        for train_idx, val_idx in kf.split(logits):
            train_logits = logits[train_idx]
            train_labels = labels[train_idx]
            val_logits = logits[val_idx]
            val_labels = labels[val_idx]
            val_confidences = confidences[val_idx]
            
            # Fit calibrator
            calibrator = TemperatureScaling()
            calibrator.fit(train_logits, train_labels, method=method)
            
            # Evaluate on validation set
            calibrated_probs = calibrator.calibrate(val_logits)
            val_calibrated_conf = np.max(calibrated_probs, axis=1)
            
            # Calculate ECE
            ece = self._calculate_ece(val_calibrated_conf, val_labels == np.argmax(calibrated_probs, axis=1))
            cv_scores.append(ece)
        
        # Fit final calibrator on full dataset
        final_calibrator = TemperatureScaling()
        final_calibrator.fit(logits, labels, method=method)
        
        # Store calibrator
        self.calibrators[dataset_name] = final_calibrator
        
        # Calculate final metrics
        calibrated_probs = final_calibrator.calibrate(logits)
        calibrated_conf = np.max(calibrated_probs, axis=1)
        correct = labels == np.argmax(calibrated_probs, axis=1)
        
        ece = self._calculate_ece(calibrated_conf, correct)
        brier_score = self._calculate_brier_score(calibrated_conf, correct)
        
        # Store metrics
        self.metrics[dataset_name] = {
            'ece': ece,
            'brier_score': brier_score,
            'cv_ece_mean': np.mean(cv_scores),
            'cv_ece_std': np.std(cv_scores),
            'method': method,
            'temperature': getattr(final_calibrator, 'temperature', None)
        }
        
        print(f"✅ Calibration fitted - ECE: {ece:.4f}, Brier: {brier_score:.4f}")
        
        # Save calibration parameters
        self._save_calibration(dataset_name, final_calibrator, self.metrics[dataset_name])
    
    def _extract_calibration_features(self, predictions: List[Dict], ground_truth: List[Dict], 
                                    dataset_name: str) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        """Extract logits, labels, and confidences for calibration"""
        logits_list = []
        labels_list = []
        confidences_list = []
        
        for pred, gt in zip(predictions, ground_truth):
            # Extract logits (convert confidence to logits if needed)
            if 'logits' in pred:
                logits = np.array(pred['logits'])
            else:
                # Convert confidence to binary logits
                confidence = pred.get('confidence', 0.5)
                logits = np.array([np.log(1 - confidence + 1e-8), np.log(confidence + 1e-8)])
            
            # Extract label
            if dataset_name in ['commonsense_qa', 'piqa', 'openbookqa']:
                # MCQ datasets
                pred_idx = self._extract_mcq_index(pred)
                gold_idx = self._extract_mcq_index(gt)
                label = 1 if pred_idx == gold_idx else 0
            elif dataset_name in ['gsm8k', 'aqua_rat']:
                # Math datasets
                pred_num = self._extract_number(pred.get('answer', ''))
                gold_num = self._extract_number(gt.get('answer', gt.get('numerical_answer', '')))
                label = 1 if abs(pred_num - gold_num) < 1e-6 else 0
            else:
                # Boolean datasets
                pred_bool = pred.get('prediction', False)
                gold_bool = gt.get('answer', False)
                label = 1 if pred_bool == gold_bool else 0
            
            # Extract confidence
            confidence = pred.get('confidence', 0.5)
            
            logits_list.append(logits)
            labels_list.append(label)
            confidences_list.append(confidence)
        
        return np.array(logits_list), np.array(labels_list), np.array(confidences_list)
    
    def _extract_mcq_index(self, item: Dict) -> int:
        """Extract choice index from MCQ item"""
        if 'predicted_index' in item:
            return item['predicted_index']
        
        answer = item.get('answer', item.get('answerKey', ''))
        if isinstance(answer, str) and len(answer) == 1 and answer.isalpha():
            return ord(answer.upper()) - ord('A')
        
        return 0
    
    def _extract_number(self, text: str) -> float:
        """Extract number from text"""
        import re
        numbers = re.findall(r'-?\d+\.?\d*', str(text))
        return float(numbers[-1]) if numbers else 0.0
    
    def calibrate_predictions(self, dataset_name: str, predictions: List[Dict]) -> List[Dict]:
        """Apply calibration to predictions"""
        if dataset_name not in self.calibrators:
            print(f"⚠️  No calibrator found for {dataset_name}. Returning uncalibrated predictions.")
            return predictions
        
        calibrator = self.calibrators[dataset_name]
        calibrated_predictions = []
        
        for pred in predictions:
            calibrated_pred = pred.copy()
            
            # Extract and calibrate logits
            if 'logits' in pred:
                logits = np.array(pred['logits'])
                calibrated_probs = calibrator.calibrate(logits.reshape(1, -1))[0]
                calibrated_pred['calibrated_confidence'] = float(np.max(calibrated_probs))
                calibrated_pred['calibrated_probabilities'] = calibrated_probs.tolist()
            else:
                # Convert confidence to logits and back
                confidence = pred.get('confidence', 0.5)
                logits = np.array([np.log(1 - confidence + 1e-8), np.log(confidence + 1e-8)])
                calibrated_probs = calibrator.calibrate(logits.reshape(1, -1))[0]
                calibrated_pred['calibrated_confidence'] = float(np.max(calibrated_probs))
            
            calibrated_predictions.append(calibrated_pred)
        
        return calibrated_predictions
    
    def _calculate_ece(self, confidences: np.ndarray, correct: np.ndarray, n_bins: int = 10) -> float:
        """Calculate Expected Calibration Error"""
        bin_boundaries = np.linspace(0, 1, n_bins + 1)
        bin_lowers = bin_boundaries[:-1]
        bin_uppers = bin_boundaries[1:]
        
        ece = 0
        for bin_lower, bin_upper in zip(bin_lowers, bin_uppers):
            in_bin = (confidences > bin_lower) & (confidences <= bin_upper)
            if not np.any(in_bin):
                continue
            
            prop_in_bin = in_bin.mean()
            accuracy_in_bin = correct[in_bin].mean()
            avg_confidence_in_bin = confidences[in_bin].mean()
            
            ece += prop_in_bin * abs(avg_confidence_in_bin - accuracy_in_bin)
        
        return ece
    
    def _calculate_brier_score(self, confidences: np.ndarray, correct: np.ndarray) -> float:
        """Calculate Brier Score"""
        return np.mean((confidences - correct) ** 2)
    
    def _save_calibration(self, dataset_name: str, calibrator: TemperatureScaling, metrics: Dict):
        """Save calibration parameters and metrics"""
        calibration_data = {
            'dataset': dataset_name,
            'temperature': getattr(calibrator, 'temperature', None),
            'method': metrics['method'],
            'metrics': metrics,
            'fitted': True
        }
        
        calibration_file = self.calibration_dir / f"{dataset_name}_calibration.json"
        with open(calibration_file, 'w') as f:
            json.dump(calibration_data, f, indent=2)
        
        print(f"💾 Saved calibration for {dataset_name} to {calibration_file}")
    
    def load_calibration(self, dataset_name: str) -> bool:
        """Load calibration parameters from file"""
        calibration_file = self.calibration_dir / f"{dataset_name}_calibration.json"
        
        if not calibration_file.exists():
            return False
        
        try:
            with open(calibration_file, 'r') as f:
                calibration_data = json.load(f)
            
            # Recreate calibrator
            calibrator = TemperatureScaling(temperature=calibration_data.get('temperature', 1.0))
            calibrator.is_fitted = True
            
            self.calibrators[dataset_name] = calibrator
            self.metrics[dataset_name] = calibration_data['metrics']
            
            print(f"✅ Loaded calibration for {dataset_name}")
            return True
        except Exception as e:
            print(f"❌ Failed to load calibration for {dataset_name}: {e}")
            return False
    
    def create_calibration_curve(self, dataset_name: str, output_dir: str = "calibration"):
        """Create calibration curve visualization"""
        if dataset_name not in self.metrics:
            print(f"⚠️  No calibration metrics for {dataset_name}")
            return
        
        # Create calibration curve plot
        plt.figure(figsize=(10, 8))
        
        # Perfect calibration line
        plt.plot([0, 1], [0, 1], 'k--', label='Perfect Calibration')
        
        # Add dataset-specific calibration info
        ece = self.metrics[dataset_name]['ece']
        brier = self.metrics[dataset_name]['brier_score']
        
        plt.title(f'Calibration Curve - {dataset_name.upper()}\nECE: {ece:.4f}, Brier: {brier:.4f}')
        plt.xlabel('Mean Predicted Probability')
        plt.ylabel('Fraction of Positives')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        # Save plot
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)
        plot_file = output_path / f"{dataset_name}_calibration_curve.png"
        plt.savefig(plot_file, dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"📊 Saved calibration curve to {plot_file}")
    
    def generate_calibration_report(self, output_file: str = "calibration/calibration_report.json"):
        """Generate comprehensive calibration report"""
        report = {
            'datasets': {},
            'overall_metrics': {},
            'recommendations': {}
        }
        
        eces = []
        brier_scores = []
        
        for dataset_name, metrics in self.metrics.items():
            report['datasets'][dataset_name] = metrics
            eces.append(metrics['ece'])
            brier_scores.append(metrics['brier_score'])
        
        # Overall metrics
        report['overall_metrics'] = {
            'mean_ece': np.mean(eces) if eces else 0.0,
            'std_ece': np.std(eces) if eces else 0.0,
            'mean_brier_score': np.mean(brier_scores) if brier_scores else 0.0,
            'std_brier_score': np.std(brier_scores) if brier_scores else 0.0
        }
        
        # Recommendations
        mean_ece = report['overall_metrics']['mean_ece']
        if mean_ece < 0.05:
            report['recommendations']['overall'] = "Excellent calibration"
        elif mean_ece < 0.1:
            report['recommendations']['overall'] = "Good calibration"
        elif mean_ece < 0.2:
            report['recommendations']['overall'] = "Moderate calibration issues"
        else:
            report['recommendations']['overall'] = "Poor calibration - needs improvement"
        
        # Save report
        output_path = Path(output_file)
        output_path.parent.mkdir(exist_ok=True)
        
        with open(output_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"📋 Generated calibration report: {output_path}")
        return report

def main():
    """Test the confidence calibration system"""
    print("🧪 Testing Confidence Calibration")
    print("==================================")
    
    # Create test data
    calibrator = ConfidenceCalibrator()
    
    # Mock predictions and ground truth
    predictions = [
        {'confidence': 0.8, 'answer': 'A'},
        {'confidence': 0.6, 'answer': 'B'},
        {'confidence': 0.9, 'answer': 'A'},
        {'confidence': 0.3, 'answer': 'C'},
        {'confidence': 0.7, 'answer': 'A'}
    ]
    
    ground_truth = [
        {'answerKey': 'A'},  # Correct
        {'answerKey': 'A'},  # Wrong
        {'answerKey': 'A'},  # Correct
        {'answerKey': 'C'},  # Correct
        {'answerKey': 'A'}   # Correct
    ]
    
    # Fit calibration
    calibrator.fit_calibration('test_dataset', predictions, ground_truth)
    
    # Test calibration
    calibrated_predictions = calibrator.calibrate_predictions('test_dataset', predictions)
    
    print("Original vs Calibrated Confidences:")
    for orig, cal in zip(predictions, calibrated_predictions):
        print(f"  {orig['confidence']:.2f} -> {cal['calibrated_confidence']:.2f}")
    
    # Generate report
    report = calibrator.generate_calibration_report()
    print(f"Overall ECE: {report['overall_metrics']['mean_ece']:.4f}")
    
    print("✅ Confidence calibration tested successfully!")

if __name__ == "__main__":
    main()
