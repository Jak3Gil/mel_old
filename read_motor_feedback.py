#!/usr/bin/env python3
"""
Read Robstride Motor Feedback Data
Decodes position, velocity, torque from motor responses
"""

import serial
import time
import struct

class MotorFeedbackReader:
    """Read and decode motor feedback."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.can_id_encoded = bytes([0xe8, 0x6c])
    
    def connect(self):
        """Connect to motor."""
        print(f"Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            
            # Initialize
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Scan
            scan_cmd = bytes([
                0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a
            ])
            self.ser.write(scan_cmd)
            time.sleep(0.5)
            self.ser.read(1000)
            
            print("Connected and initialized\n")
            return True
            
        except Exception as e:
            print(f"Error: {e}")
            return False
    
    def decode_feedback(self, data):
        """
        Decode motor feedback data.
        Motor sends back position, velocity, torque in response.
        """
        if len(data) < 8:
            return None
        
        try:
            # Try to decode the response
            # Format might be: AT [header] [position] [velocity] [torque] [temp] [error]
            
            # Look for response pattern
            if data[:2] == b'AT':
                print(f"    Raw response: {data.hex()}")
                print(f"    Length: {len(data)} bytes")
                
                # Try to find data payload
                if len(data) >= 10:
                    # Skip AT and header bytes, look for data
                    payload = data[6:] if len(data) > 6 else data
                    print(f"    Payload: {payload.hex()}")
                    
                    # Motor feedback is typically 8 bytes
                    if len(payload) >= 8:
                        # Decode as signed integers
                        # Position (16-bit)
                        pos_raw = struct.unpack('>h', payload[0:2])[0]
                        
                        # Velocity (12-bit)
                        vel_raw = (payload[2] << 4) | (payload[3] >> 4)
                        
                        # Torque (12-bit)
                        torque_raw = ((payload[3] & 0x0F) << 8) | payload[4]
                        
                        # Convert to actual values
                        position = pos_raw * (25.0 / 65535) - 12.5  # Map to -12.5 to +12.5 rad
                        velocity = vel_raw * (90.0 / 4095) - 45.0   # Map to -45 to +45 rad/s
                        torque = torque_raw * (36.0 / 4095) - 18.0  # Map to -18 to +18 Nm
                        
                        return {
                            'position': position,
                            'velocity': velocity,
                            'torque': torque,
                            'raw_position': pos_raw,
                            'raw_velocity': vel_raw,
                            'raw_torque': torque_raw
                        }
            
            return None
            
        except Exception as e:
            print(f"    Decode error: {e}")
            return None
    
    def send_and_read(self, command_data):
        """Send command and read response."""
        # Build command
        cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08
        ]) + command_data + bytes([0x0d, 0x0a])
        
        # Clear buffer
        self.ser.reset_input_buffer()
        
        # Send
        self.ser.write(cmd)
        time.sleep(0.05)  # Wait for response
        
        # Read response
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            return response
        
        return None
    
    def read_position(self):
        """Read current motor position."""
        # Send a query command or movement command to get feedback
        # Using stop command to get status without moving
        stop_cmd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        
        response = self.send_and_read(stop_cmd)
        
        if response:
            print(f"\n  Motor Response:")
            feedback = self.decode_feedback(response)
            
            if feedback:
                print(f"  Position: {feedback['position']:.4f} rad")
                print(f"  Velocity: {feedback['velocity']:.4f} rad/s")
                print(f"  Torque:   {feedback['torque']:.4f} Nm")
                return feedback
            else:
                print(f"  Could not decode feedback")
                return None
        else:
            print(f"  No response from motor")
            return None
    
    def continuous_monitoring(self, duration=10):
        """Monitor motor feedback continuously."""
        print(f"\nMonitoring motor for {duration} seconds...")
        print("Press Ctrl+C to stop\n")
        
        start_time = time.time()
        count = 0
        
        try:
            while (time.time() - start_time) < duration:
                print(f"[{count}]", end=" ")
                self.read_position()
                count += 1
                time.sleep(0.5)
                
        except KeyboardInterrupt:
            print("\n\nStopped by user")
    
    def test_movement_with_feedback(self):
        """Test movement and read position feedback."""
        print("\n" + "=" * 70)
        print("MOVEMENT WITH FEEDBACK")
        print("=" * 70)
        
        print("\n[1] Reading initial position...")
        initial = self.read_position()
        
        print("\n[2] Moving forward 1 second...")
        forward_cmd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        
        # Move
        for i in range(20):  # 20 x 50ms = 1 second
            response = self.send_and_read(forward_cmd)
            if response and i % 5 == 0:
                print(f"  [{i*50}ms]", end=" ")
                self.decode_feedback(response)
            time.sleep(0.05)
        
        # Stop
        stop_cmd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        self.send_and_read(stop_cmd)
        
        print("\n[3] Reading final position...")
        final = self.read_position()
        
        if initial and final:
            delta = final['position'] - initial['position']
            print(f"\n  Position change: {delta:.4f} rad")
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("\nDisconnected")


def main():
    """Main test program."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR FEEDBACK READER")
    print("=" * 70 + "\n")
    
    reader = MotorFeedbackReader()
    
    if not reader.connect():
        return
    
    try:
        print("Options:")
        print("  1 - Read position once")
        print("  2 - Continuous monitoring (10 seconds)")
        print("  3 - Test movement with feedback")
        
        choice = input("\nEnter choice (1/2/3): ").strip()
        
        if choice == '1':
            print("\n" + "=" * 70)
            reader.read_position()
            
        elif choice == '2':
            reader.continuous_monitoring(10)
            
        elif choice == '3':
            reader.test_movement_with_feedback()
        
        else:
            print("Invalid choice")
        
    except KeyboardInterrupt:
        print("\n\nStopped")
    
    finally:
        reader.disconnect()


if __name__ == "__main__":
    main()

