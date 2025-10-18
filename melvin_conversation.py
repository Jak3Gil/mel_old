#!/usr/bin/env python3
"""
╔═══════════════════════════════════════════════════════════════════════════╗
║  MELVIN CONVERSATIONAL INTERFACE                                          ║
║  Two-way voice communication: Listen & Speak                              ║
╚═══════════════════════════════════════════════════════════════════════════╝
"""

import sys
import os
import time
import queue
import threading

# Add melvin to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))

try:
    import speech_recognition as sr
    SPEECH_RECOGNITION_AVAILABLE = True
except ImportError:
    SPEECH_RECOGNITION_AVAILABLE = False
    print("⚠️  speech_recognition not installed")
    print("   Install with: pip install SpeechRecognition pyaudio")

# Import from melvin.io module
from melvin.io.text_to_speech import initialize_tts, speak

class MelvinConversation:
    """
    Conversational interface for Melvin
    Enables two-way voice communication
    """
    
    def __init__(self):
        print("🎙️ Initializing Melvin Conversation System...")
        
        # Initialize TTS
        print("\n1. Initializing text-to-speech...")
        if not initialize_tts():
            print("   ⚠️  TTS not available, will use text output")
        
        # Initialize speech recognition
        print("\n2. Initializing speech recognition...")
        if SPEECH_RECOGNITION_AVAILABLE:
            self.recognizer = sr.Recognizer()
            self.microphone = sr.Microphone()
            
            # Adjust for ambient noise
            print("   📊 Calibrating for ambient noise (please be quiet)...")
            with self.microphone as source:
                self.recognizer.adjust_for_ambient_noise(source, duration=2)
            print("   ✅ Calibration complete")
        else:
            self.recognizer = None
            self.microphone = None
        
        # Conversation state
        self.running = False
        self.conversation_history = []
        
        print("\n✅ Melvin Conversation System ready!")
    
    def listen(self, timeout=10):
        """
        Listen for user speech
        
        Args:
            timeout: Maximum time to listen
            
        Returns:
            Recognized text or None
        """
        if not SPEECH_RECOGNITION_AVAILABLE:
            # Fallback to text input
            return input("You: ")
        
        try:
            print("🎧 Listening...")
            
            with self.microphone as source:
                audio = self.recognizer.listen(source, timeout=timeout, phrase_time_limit=10)
            
            print("🔄 Recognizing...")
            
            # Try multiple engines for best results
            text = None
            
            # Try Google (best quality, requires internet)
            try:
                text = self.recognizer.recognize_google(audio)
                print(f"✅ Recognized: \"{text}\"")
                return text
            except sr.UnknownValueError:
                print("⚠️  Could not understand audio")
                return None
            except sr.RequestError:
                print("⚠️  Google Speech Recognition unavailable, trying offline...")
            
            # Try Sphinx (offline, lower quality)
            try:
                text = self.recognizer.recognize_sphinx(audio)
                print(f"✅ Recognized (offline): \"{text}\"")
                return text
            except:
                print("⚠️  Speech recognition failed")
                return None
                
        except sr.WaitTimeoutError:
            print("⏱️  No speech detected")
            return None
        except Exception as e:
            print(f"❌ Error listening: {e}")
            return None
    
    def respond(self, text):
        """
        Generate and speak response
        
        Args:
            text: Response text
        """
        print(f"\n🧠 Melvin: {text}\n")
        speak(text, blocking=True)
    
    def process_input(self, user_input):
        """
        Process user input and generate response
        
        Args:
            user_input: User's speech/text
            
        Returns:
            Response text
        """
        # Add to conversation history
        self.conversation_history.append({
            'role': 'user',
            'text': user_input,
            'timestamp': time.time()
        })
        
        # Simple response system (replace with actual Melvin reasoning)
        response = self.generate_response(user_input)
        
        # Add response to history
        self.conversation_history.append({
            'role': 'melvin',
            'text': response,
            'timestamp': time.time()
        })
        
        return response
    
    def generate_response(self, user_input):
        """
        Generate response to user input
        (This is a placeholder - integrate with actual Melvin brain)
        
        Args:
            user_input: User's input text
            
        Returns:
            Response text
        """
        user_lower = user_input.lower()
        
        # Greeting
        if any(word in user_lower for word in ['hello', 'hi', 'hey', 'greetings']):
            return "Hello! I'm Melvin. How can I help you today?"
        
        # Identity
        elif any(word in user_lower for word in ['who are you', 'what are you', 'your name']):
            return "I am Melvin, an AI assistant with audio perception and reasoning capabilities. I can hear you and speak back."
        
        # Capabilities
        elif any(word in user_lower for word in ['what can you do', 'capabilities', 'help']):
            return "I can listen to your voice, understand speech, reason about concepts, and respond through speech. I'm learning to perceive and understand the world through multiple senses."
        
        # Status
        elif any(word in user_lower for word in ['how are you', 'status']):
            return "I'm functioning well. All my audio systems are online and I'm ready to converse with you."
        
        # Exit
        elif any(word in user_lower for word in ['goodbye', 'bye', 'exit', 'quit', 'stop']):
            return "Goodbye! It was nice talking with you."
        
        # Affirmation
        elif any(word in user_lower for word in ['thank', 'thanks']):
            return "You're welcome! Happy to help."
        
        # Default response
        else:
            return f"I heard you say: {user_input}. I'm still learning to respond to all types of input."
    
    def run_conversation(self):
        """
        Run the main conversation loop
        """
        print("\n")
        print("╔═══════════════════════════════════════════════════════╗")
        print("║  🎙️ MELVIN CONVERSATIONAL MODE                        ║")
        print("║  Speak naturally - I'm listening!                     ║")
        print("╚═══════════════════════════════════════════════════════╝")
        print()
        
        # Welcome message
        self.respond("Hello! I'm Melvin. I can hear you and speak back. How can I help you today?")
        
        self.running = True
        
        while self.running:
            try:
                # Listen for user input
                user_input = self.listen(timeout=30)
                
                if user_input is None:
                    continue
                
                # Check for exit
                if any(word in user_input.lower() for word in ['goodbye', 'bye', 'exit', 'quit', 'stop']):
                    response = self.process_input(user_input)
                    self.respond(response)
                    break
                
                # Process and respond
                response = self.process_input(user_input)
                self.respond(response)
                
            except KeyboardInterrupt:
                print("\n\n🛑 Conversation interrupted")
                self.respond("Conversation ended. Goodbye!")
                break
            except Exception as e:
                print(f"\n❌ Error: {e}")
                continue
        
        self.running = False
        
        # Print conversation summary
        self.print_summary()
    
    def print_summary(self):
        """Print conversation summary"""
        print("\n")
        print("╔═══════════════════════════════════════════════════════╗")
        print("║  📊 CONVERSATION SUMMARY                              ║")
        print("╚═══════════════════════════════════════════════════════╝")
        print()
        print(f"Total exchanges: {len(self.conversation_history) // 2}")
        print()
        
        if len(self.conversation_history) > 0:
            print("Conversation history:")
            for entry in self.conversation_history:
                role = "You" if entry['role'] == 'user' else "Melvin"
                print(f"  {role}: {entry['text']}")
        
        print("\n✅ Conversation complete")

def main():
    """Main entry point"""
    print("\n")
    print("╔═══════════════════════════════════════════════════════╗")
    print("║  🧠 MELVIN - CONVERSATIONAL AI                        ║")
    print("║  Two-Way Voice Communication System                   ║")
    print("╚═══════════════════════════════════════════════════════╝")
    print()
    
    # Check dependencies
    print("Checking dependencies...")
    
    if not SPEECH_RECOGNITION_AVAILABLE:
        print("\n⚠️  Speech recognition not available")
        print("   Install with:")
        print("   pip install SpeechRecognition pyaudio")
        print()
        response = input("Continue with text input only? (y/n): ")
        if response.lower() != 'y':
            return
    
    # Create and run conversation
    try:
        conversation = MelvinConversation()
        conversation.run_conversation()
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()

