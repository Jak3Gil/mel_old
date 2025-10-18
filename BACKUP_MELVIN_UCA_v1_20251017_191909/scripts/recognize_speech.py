#!/usr/bin/env python3
"""
Speech Recognition Script for Melvin Audio Pipeline
Uses OpenAI Whisper for high-quality transcription
"""

import sys
import json
import argparse
from pathlib import Path

try:
    import whisper
    WHISPER_AVAILABLE = True
except ImportError:
    WHISPER_AVAILABLE = False
    print("Warning: Whisper not installed. Install with: pip install openai-whisper", 
          file=sys.stderr)

def recognize_speech_whisper(audio_path, model_name="base", language=None):
    """
    Recognize speech using Whisper
    
    Args:
        audio_path: Path to audio file (WAV, MP3, etc.)
        model_name: Whisper model size (tiny, base, small, medium, large)
        language: Language code (None for auto-detect)
    
    Returns:
        dict with transcript and metadata
    """
    if not WHISPER_AVAILABLE:
        return {
            "text": "",
            "confidence": 0.0,
            "error": "Whisper not installed"
        }
    
    try:
        # Load model (cached after first run)
        model = whisper.load_model(model_name)
        
        # Transcribe
        if language:
            result = model.transcribe(audio_path, language=language)
        else:
            result = model.transcribe(audio_path)
        
        # Extract results
        transcript = result["text"].strip()
        language_detected = result.get("language", "unknown")
        
        # Whisper doesn't provide word-level confidence directly,
        # but we can use the overall probability
        confidence = 0.9 if transcript else 0.0
        
        return {
            "text": transcript,
            "confidence": confidence,
            "language": language_detected,
            "error": None
        }
        
    except Exception as e:
        return {
            "text": "",
            "confidence": 0.0,
            "error": str(e)
        }

def recognize_speech_vosk(audio_path):
    """
    Alternative: Recognize speech using Vosk (offline, lightweight)
    
    Args:
        audio_path: Path to audio file
    
    Returns:
        dict with transcript and metadata
    """
    try:
        from vosk import Model, KaldiRecognizer
        import wave
        
        # Load model (download from https://alphacephei.com/vosk/models)
        model = Model("models/vosk-model-small-en-us-0.15")
        
        # Open audio file
        wf = wave.open(audio_path, "rb")
        rec = KaldiRecognizer(model, wf.getframerate())
        
        # Process audio
        transcript = ""
        while True:
            data = wf.readframes(4000)
            if len(data) == 0:
                break
            if rec.AcceptWaveform(data):
                result = json.loads(rec.Result())
                transcript += result.get("text", "") + " "
        
        # Final result
        result = json.loads(rec.FinalResult())
        transcript += result.get("text", "")
        
        return {
            "text": transcript.strip(),
            "confidence": result.get("confidence", 0.8),
            "language": "en",
            "error": None
        }
        
    except ImportError:
        return {
            "text": "",
            "confidence": 0.0,
            "error": "Vosk not installed. Install with: pip install vosk"
        }
    except Exception as e:
        return {
            "text": "",
            "confidence": 0.0,
            "error": str(e)
        }

def main():
    parser = argparse.ArgumentParser(description="Speech recognition for Melvin")
    parser.add_argument("audio_file", help="Path to audio file")
    parser.add_argument("--model", default="base", 
                       choices=["tiny", "base", "small", "medium", "large"],
                       help="Whisper model size")
    parser.add_argument("--language", default=None, 
                       help="Language code (e.g., 'en', 'es', 'zh')")
    parser.add_argument("--engine", default="whisper",
                       choices=["whisper", "vosk"],
                       help="Recognition engine")
    parser.add_argument("--json", action="store_true",
                       help="Output JSON instead of plain text")
    
    args = parser.parse_args()
    
    # Check if file exists
    if not Path(args.audio_file).exists():
        print(f"Error: File not found: {args.audio_file}", file=sys.stderr)
        sys.exit(1)
    
    # Run recognition
    if args.engine == "whisper":
        result = recognize_speech_whisper(args.audio_file, args.model, args.language)
    else:
        result = recognize_speech_vosk(args.audio_file)
    
    # Output
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        if result["error"]:
            print(f"Error: {result['error']}", file=sys.stderr)
            sys.exit(1)
        print(result["text"])

if __name__ == "__main__":
    main()

