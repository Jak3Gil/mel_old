#!/usr/bin/env python3
"""
Text-to-Speech Module for Melvin
Enables Melvin to speak responses through speakers
"""

import sys
import platform

# Try multiple TTS engines for best compatibility
TTS_ENGINE = None

def initialize_tts():
    """Initialize the best available TTS engine"""
    global TTS_ENGINE
    
    # Try macOS 'say' command first (most reliable on macOS)
    if platform.system() == 'Darwin':
        try:
            import subprocess
            subprocess.run(['say', 'TTS initialized'], check=True, 
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            TTS_ENGINE = ('say', None)
            print("✅ Using macOS 'say' command")
            return True
        except:
            pass
    
    # Try espeak (Linux)
    if platform.system() == 'Linux':
        try:
            import subprocess
            subprocess.run(['espeak', 'TTS initialized'], check=True,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            TTS_ENGINE = ('espeak', None)
            print("✅ Using espeak (Linux)")
            return True
        except:
            pass
    
    print("❌ No TTS engine available")
    print("   Install: pip install pyttsx3")
    return False

def speak(text, blocking=True):
    """
    Speak text through speakers
    
    Args:
        text: Text to speak
        blocking: If True, wait for speech to complete
    """
    global TTS_ENGINE
    
    if not TTS_ENGINE:
        if not initialize_tts():
            print(f"[SPEAK] {text}")  # Fallback to print
            return False
    
    engine_type, engine = TTS_ENGINE
    
    try:
        if engine_type == 'pyttsx3':
            engine.say(text)
            if blocking:
                engine.runAndWait()
            return True
            
        elif engine_type == 'say':
            import subprocess
            if blocking:
                subprocess.run(['say', text], check=True)
            else:
                subprocess.Popen(['say', text])
            return True
            
        elif engine_type == 'espeak':
            import subprocess
            if blocking:
                subprocess.run(['espeak', text], check=True)
            else:
                subprocess.Popen(['espeak', text])
            return True
            
    except Exception as e:
        print(f"❌ TTS error: {e}")
        print(f"[SPEAK] {text}")  # Fallback to print
        return False
    
    return False

def set_voice_properties(rate=None, volume=None, voice=None):
    """
    Configure voice properties
    
    Args:
        rate: Speech rate (words per minute)
        volume: Volume (0.0 to 1.0)
        voice: Voice ID or name
    """
    global TTS_ENGINE
    
    if not TTS_ENGINE or TTS_ENGINE[0] != 'pyttsx3':
        return
    
    engine = TTS_ENGINE[1]
    
    if rate is not None:
        engine.setProperty('rate', rate)
    
    if volume is not None:
        engine.setProperty('volume', volume)
    
    if voice is not None:
        voices = engine.getProperty('voices')
        for v in voices:
            if voice.lower() in v.name.lower() or voice == v.id:
                engine.setProperty('voice', v.id)
                break

def list_voices():
    """List available voices"""
    try:
        import pyttsx3
        engine = pyttsx3.init()
        voices = engine.getProperty('voices')
        print("\n🎙️ Available Voices:")
        for i, voice in enumerate(voices):
            print(f"   {i}. {voice.name} ({voice.id})")
        return voices
    except:
        print("⚠️  pyttsx3 not available")
        return []

# Quick test
if __name__ == "__main__":
    print("🎙️ Text-to-Speech Test\n")
    
    if initialize_tts():
        print("\nTesting voice...")
        speak("Hello, I am Melvin. I can now speak through your speakers.")
        
        print("\nListing available voices...")
        list_voices()
        
        print("\n✅ TTS test complete")
    else:
        print("\n❌ TTS not available")
        print("Install with: pip install pyttsx3")

