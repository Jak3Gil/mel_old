#!/usr/bin/env python3
"""
Ambient Sound Classification Script for Melvin Audio Pipeline
Uses YAMNet (or custom CNN) for environmental sound detection
"""

import sys
import json
import argparse
import numpy as np
from pathlib import Path

try:
    import tensorflow as tf
    import tensorflow_hub as hub
    TF_AVAILABLE = True
except ImportError:
    TF_AVAILABLE = False
    print("Warning: TensorFlow not installed. Install with: pip install tensorflow tensorflow-hub",
          file=sys.stderr)

# YAMNet class names (521 categories)
YAMNET_CLASS_NAMES = None

def load_yamnet_classes():
    """Load YAMNet class names from CSV"""
    global YAMNET_CLASS_NAMES
    
    if YAMNET_CLASS_NAMES is not None:
        return YAMNET_CLASS_NAMES
    
    # YAMNet uses AudioSet ontology
    # Download from: https://github.com/tensorflow/models/blob/master/research/audioset/yamnet/yamnet_class_map.csv
    try:
        import csv
        import urllib.request
        
        url = "https://raw.githubusercontent.com/tensorflow/models/master/research/audioset/yamnet/yamnet_class_map.csv"
        
        # Try to download class map
        response = urllib.request.urlopen(url)
        lines = response.read().decode('utf-8').split('\n')
        
        class_names = []
        for line in csv.reader(lines):
            if len(line) >= 2:
                class_names.append(line[2])  # Display name
        
        YAMNET_CLASS_NAMES = class_names
        return class_names
        
    except Exception as e:
        print(f"Warning: Could not load YAMNet class names: {e}", file=sys.stderr)
        return None

def load_audio_file(audio_path, target_sr=16000):
    """
    Load audio file and resample to target sample rate
    
    Args:
        audio_path: Path to audio file
        target_sr: Target sample rate (YAMNet uses 16kHz)
    
    Returns:
        numpy array of audio samples
    """
    try:
        # Try librosa first (best quality)
        import librosa
        audio, sr = librosa.load(audio_path, sr=target_sr, mono=True)
        return audio
        
    except ImportError:
        # Fallback to scipy
        try:
            from scipy.io import wavfile
            sr, audio = wavfile.read(audio_path)
            
            # Convert to float
            if audio.dtype == np.int16:
                audio = audio.astype(np.float32) / 32768.0
            elif audio.dtype == np.int32:
                audio = audio.astype(np.float32) / 2147483648.0
            
            # Resample if needed (basic method)
            if sr != target_sr:
                duration = len(audio) / sr
                new_length = int(duration * target_sr)
                audio = np.interp(
                    np.linspace(0, len(audio), new_length),
                    np.arange(len(audio)),
                    audio
                )
            
            return audio
            
        except ImportError:
            raise ImportError("Install librosa (pip install librosa) or scipy (pip install scipy)")

def classify_yamnet(audio_path, top_k=3):
    """
    Classify ambient sound using YAMNet
    
    Args:
        audio_path: Path to audio file
        top_k: Return top K predictions
    
    Returns:
        dict with classifications and confidences
    """
    if not TF_AVAILABLE:
        return {
            "label": "",
            "confidence": 0.0,
            "predictions": [],
            "error": "TensorFlow not installed"
        }
    
    try:
        # Load YAMNet model
        model = hub.load('https://tfhub.dev/google/yamnet/1')
        
        # Load class names
        class_names = load_yamnet_classes()
        
        # Load audio
        audio = load_audio_file(audio_path)
        
        # Run inference
        scores, embeddings, spectrogram = model(audio)
        
        # Get top predictions
        scores_np = scores.numpy()
        mean_scores = np.mean(scores_np, axis=0)  # Average over time
        
        top_indices = np.argsort(mean_scores)[-top_k:][::-1]
        
        predictions = []
        for idx in top_indices:
            label = class_names[idx] if class_names else f"class_{idx}"
            confidence = float(mean_scores[idx])
            predictions.append({
                "label": label,
                "confidence": confidence
            })
        
        return {
            "label": predictions[0]["label"],
            "confidence": predictions[0]["confidence"],
            "predictions": predictions,
            "error": None
        }
        
    except Exception as e:
        return {
            "label": "",
            "confidence": 0.0,
            "predictions": [],
            "error": str(e)
        }

def classify_simple(audio_path):
    """
    Simple rule-based classification (fallback if ML not available)
    
    Args:
        audio_path: Path to audio file
    
    Returns:
        dict with classification
    """
    try:
        audio = load_audio_file(audio_path)
        
        # Simple heuristics
        rms = np.sqrt(np.mean(audio ** 2))
        peak = np.max(np.abs(audio))
        
        # Very basic classification
        if rms < 0.01:
            label = "silence"
            confidence = 0.9
        elif peak > 0.8:
            label = "loud noise"
            confidence = 0.7
        elif rms > 0.1:
            label = "ambient sound"
            confidence = 0.6
        else:
            label = "quiet sound"
            confidence = 0.5
        
        return {
            "label": label,
            "confidence": confidence,
            "predictions": [{"label": label, "confidence": confidence}],
            "error": None
        }
        
    except Exception as e:
        return {
            "label": "",
            "confidence": 0.0,
            "predictions": [],
            "error": str(e)
        }

def main():
    parser = argparse.ArgumentParser(description="Ambient sound classification for Melvin")
    parser.add_argument("audio_file", help="Path to audio file")
    parser.add_argument("--model", default="yamnet",
                       choices=["yamnet", "simple"],
                       help="Classification model")
    parser.add_argument("--top-k", type=int, default=3,
                       help="Return top K predictions")
    parser.add_argument("--json", action="store_true",
                       help="Output JSON instead of plain text")
    
    args = parser.parse_args()
    
    # Check if file exists
    if not Path(args.audio_file).exists():
        print(f"Error: File not found: {args.audio_file}", file=sys.stderr)
        sys.exit(1)
    
    # Run classification
    if args.model == "yamnet":
        result = classify_yamnet(args.audio_file, args.top_k)
    else:
        result = classify_simple(args.audio_file)
    
    # Output
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        if result["error"]:
            print(f"Error: {result['error']}", file=sys.stderr)
            sys.exit(1)
        print(f"{result['label']} ({result['confidence']:.2f})")

if __name__ == "__main__":
    main()

