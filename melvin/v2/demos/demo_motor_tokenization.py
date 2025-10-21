#!/usr/bin/env python3
"""
Motor Tokenization Demo - Connect Real Motors to Graph-Based Cognition

This demonstrates how motor state (position, velocity, torque) gets
tokenized into graph concepts that Melvin can reason about.

Flow:
1. Read Robstride motor feedback (position, velocity, torque)
2. Tokenize into NodeIDs via MotorBridge
3. Store in semantic memory (graph)
4. Reason about motor state
5. Detokenize desired state back to motor commands

This allows Melvin to:
- Learn motor patterns in semantic memory
- Predict consequences of actions
- Reason about motor goals ("move Motor13 to 2.5 rad")
- Integrate motor state with vision/audio
"""

import sys
import os
import serial
import time
import struct

# Add parent directories to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

class MotorFeedbackReader:
    """Read position, velocity, torque from Robstride motors."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        
        # Motor encodings
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
    
    def connect(self):
        """Connect and initialize motors."""
        print(f"\n[MotorFeedback] Connecting to {self.port}...")
        self.ser = serial.Serial(self.port, self.baud, timeout=1.0)
        time.sleep(0.5)
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        
        # Initialize
        self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        self.ser.read(1000)
        
        print("[MotorFeedback] Connected!\n")
        return True
    
    def decode_feedback(self, data):
        """
        Decode motor response into position, velocity, torque.
        
        Returns:
            dict with keys: position (rad), velocity (rad/s), torque (Nm)
        """
        if len(data) < 10:
            return None
        
        try:
            # Find data payload (skip AT header)
            payload = data[6:] if len(data) > 6 else data
            
            if len(payload) >= 8:
                # Position (16-bit)
                pos_raw = struct.unpack('>h', payload[0:2])[0]
                
                # Velocity (12-bit)
                vel_raw = (payload[2] << 4) | (payload[3] >> 4)
                
                # Torque (12-bit)
                torque_raw = ((payload[3] & 0x0F) << 8) | payload[4]
                
                # Convert to actual units
                position = pos_raw * (25.0 / 65535) - 12.5  # -12.5 to +12.5 rad
                velocity = vel_raw * (90.0 / 4095) - 45.0   # -45 to +45 rad/s
                torque = torque_raw * (36.0 / 4095) - 18.0  # -18 to +18 Nm
                
                return {
                    'position': position,
                    'velocity': velocity,
                    'torque': torque
                }
        except Exception as e:
            print(f"[MotorFeedback] Decode error: {e}")
        
        return None
    
    def read_motor_state(self, motor_id=13):
        """Read current state of specified motor."""
        # Build query command
        motor_encoding = self.motor13_encoding if motor_id == 13 else self.motor14_encoding
        
        # Send query (using stop command to get status)
        stop_cmd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + stop_cmd + bytes([0x0d, 0x0a])
        
        self.ser.reset_input_buffer()
        self.ser.write(cmd)
        time.sleep(0.05)
        
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            feedback = self.decode_feedback(response)
            
            if feedback:
                feedback['motor_id'] = motor_id
                return feedback
        
        return None
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()


class MotorTokenizer:
    """
    Tokenize motor state into graph concepts.
    
    This is a Python version of the C++ MotorBridge for demonstration.
    In production, you'd call the C++ version via ctypes or pybind11.
    """
    
    def __init__(self, position_bins=25, velocity_bins=18, torque_bins=18):
        self.position_bins = position_bins
        self.velocity_bins = velocity_bins
        self.torque_bins = torque_bins
        
        # Concept caches
        self.position_concepts = {}  # value -> node_id
        self.velocity_concepts = {}
        self.torque_concepts = {}
        self.motor_nodes = {}        # motor_id -> node_id
        
        # Simple node ID counter (in real system, comes from SemanticBridge)
        self.next_node_id = 1000
        
        # Store previous states for novelty detection
        self.prev_states = {}
    
    def discretize_position(self, position):
        """Bin position into discrete concepts."""
        bin_size = 25.0 / self.position_bins
        normalized = position + 12.5
        bin_idx = int(normalized / bin_size)
        bin_idx = max(0, min(bin_idx, self.position_bins - 1))
        
        # Return center of bin
        return (bin_idx * bin_size + bin_size / 2.0) - 12.5
    
    def discretize_velocity(self, velocity):
        """Bin velocity into discrete concepts."""
        bin_size = 90.0 / self.velocity_bins
        normalized = velocity + 45.0
        bin_idx = int(normalized / bin_size)
        bin_idx = max(0, min(bin_idx, self.velocity_bins - 1))
        
        return (bin_idx * bin_size + bin_size / 2.0) - 45.0
    
    def discretize_torque(self, torque):
        """Bin torque into discrete concepts."""
        bin_size = 36.0 / self.torque_bins
        normalized = torque + 18.0
        bin_idx = int(normalized / bin_size)
        bin_idx = max(0, min(bin_idx, self.torque_bins - 1))
        
        return (bin_idx * bin_size + bin_size / 2.0) - 18.0
    
    def get_or_create_concept(self, value, concept_dict, label_prefix):
        """Get existing concept or create new one."""
        if value in concept_dict:
            return concept_dict[value]
        
        # Create new concept
        node_id = self.next_node_id
        self.next_node_id += 1
        
        concept_dict[value] = node_id
        
        label = f"{label_prefix}_{value:.2f}"
        print(f"  [Created concept] NodeID={node_id} Label='{label}'")
        
        return node_id
    
    def tokenize(self, motor_state):
        """
        Convert motor state to graph concepts.
        
        Args:
            motor_state: dict with keys motor_id, position, velocity, torque
        
        Returns:
            dict with:
                - motor_node: NodeID for motor
                - position_node: NodeID for discretized position
                - velocity_node: NodeID for discretized velocity
                - torque_node: NodeID for discretized torque
                - raw_state: original values
                - salience: how interesting (0-1)
                - novelty: how unexpected (0-1)
        """
        motor_id = motor_state['motor_id']
        position = motor_state['position']
        velocity = motor_state['velocity']
        torque = motor_state['torque']
        
        print(f"\n=== Tokenizing Motor {motor_id} State ===")
        print(f"  Raw: pos={position:.3f}rad, vel={velocity:.3f}rad/s, torque={torque:.3f}Nm")
        
        # Get motor node
        if motor_id not in self.motor_nodes:
            self.motor_nodes[motor_id] = self.next_node_id
            self.next_node_id += 1
            print(f"  [Created motor] Motor{motor_id} -> NodeID={self.motor_nodes[motor_id]}")
        
        motor_node = self.motor_nodes[motor_id]
        
        # Discretize and get concepts
        pos_discrete = self.discretize_position(position)
        vel_discrete = self.discretize_velocity(velocity)
        torque_discrete = self.discretize_torque(torque)
        
        print(f"  Discretized: pos={pos_discrete:.2f}rad, vel={vel_discrete:.1f}rad/s, torque={torque_discrete:.2f}Nm")
        
        position_node = self.get_or_create_concept(pos_discrete, self.position_concepts, "Position")
        velocity_node = self.get_or_create_concept(vel_discrete, self.velocity_concepts, "Velocity")
        torque_node = self.get_or_create_concept(torque_discrete, self.torque_concepts, "Torque")
        
        # Compute salience (how interesting?)
        velocity_salience = abs(velocity) / 45.0
        torque_salience = abs(torque) / 18.0
        
        # Position change salience
        position_change_salience = 0.0
        if motor_id in self.prev_states:
            prev_pos = self.prev_states[motor_id]['position']
            pos_change = abs(position - prev_pos)
            position_change_salience = min(pos_change / 0.1, 1.0)
        
        salience = 0.4 * velocity_salience + 0.3 * torque_salience + 0.3 * position_change_salience
        salience = min(salience, 1.0)
        
        # Compute novelty (how unexpected?)
        novelty = 0.0
        if motor_id in self.prev_states:
            prev = self.prev_states[motor_id]
            # Simple prediction: assume velocity stays same
            predicted_pos = prev['position'] + prev['velocity'] * 0.05  # 50ms timestep
            position_error = abs(position - predicted_pos)
            novelty = min(position_error / 2.0, 1.0)
        
        # Store for next time
        self.prev_states[motor_id] = motor_state.copy()
        
        percept = {
            'motor_node': motor_node,
            'position_node': position_node,
            'velocity_node': velocity_node,
            'torque_node': torque_node,
            'raw_state': motor_state,
            'salience': salience,
            'novelty': novelty
        }
        
        print(f"  Tokenized: Motor{motor_id}[{motor_node}] -> Pos[{position_node}] Vel[{velocity_node}] Torque[{torque_node}]")
        print(f"  Salience: {salience:.3f}, Novelty: {novelty:.3f}")
        
        # In real system, would create edges:
        # semantic_bridge.add_relation(motor_node, position_node, relation_type=1)
        # semantic_bridge.add_relation(motor_node, velocity_node, relation_type=2)
        # semantic_bridge.add_relation(motor_node, torque_node, relation_type=3)
        
        return percept
    
    def detokenize(self, motor_node, target_position_node):
        """
        Convert desired position concept back to motor command.
        
        Args:
            motor_node: NodeID of motor
            target_position_node: NodeID of desired position
        
        Returns:
            dict with motor_id and target position value
        """
        # Find motor ID
        motor_id = None
        for mid, nid in self.motor_nodes.items():
            if nid == motor_node:
                motor_id = mid
                break
        
        # Find position value
        target_position = None
        for value, nid in self.position_concepts.items():
            if nid == target_position_node:
                target_position = value
                break
        
        return {
            'motor_id': motor_id,
            'target_position': target_position
        }
    
    def print_semantic_memory(self):
        """Print all concepts created so far."""
        print("\n" + "=" * 70)
        print("SEMANTIC MEMORY (Motor Concepts)")
        print("=" * 70)
        
        print(f"\nMotor Nodes: {len(self.motor_nodes)}")
        for motor_id, node_id in self.motor_nodes.items():
            print(f"  Motor{motor_id} -> NodeID {node_id}")
        
        print(f"\nPosition Concepts: {len(self.position_concepts)}")
        for value, node_id in sorted(self.position_concepts.items()):
            print(f"  Position_{value:.2f}rad -> NodeID {node_id}")
        
        print(f"\nVelocity Concepts: {len(self.velocity_concepts)}")
        for value, node_id in sorted(self.velocity_concepts.items()):
            print(f"  Velocity_{value:.1f}rad_s -> NodeID {node_id}")
        
        print(f"\nTorque Concepts: {len(self.torque_concepts)}")
        for value, node_id in sorted(self.torque_concepts.items()):
            print(f"  Torque_{value:.2f}Nm -> NodeID {node_id}")
        
        print("=" * 70)


def demo_motor_tokenization():
    """Main demonstration."""
    print("=" * 70)
    print("MOTOR TOKENIZATION DEMO")
    print("Graph-Based Motor Cognition")
    print("=" * 70)
    
    # Connect to motors
    reader = MotorFeedbackReader()
    if not reader.connect():
        print("[ERROR] Could not connect to motors")
        return
    
    # Create tokenizer
    tokenizer = MotorTokenizer(
        position_bins=25,    # 1.0 rad resolution
        velocity_bins=18,    # 5 rad/s resolution
        torque_bins=18       # 2 Nm resolution
    )
    
    print("\n[1] Reading Motor 13 state 10 times...")
    print("    Watch how motor state gets tokenized into graph concepts!")
    print()
    
    percepts = []
    
    try:
        for i in range(10):
            print(f"\n--- Reading {i+1}/10 ---")
            
            # Read motor state
            state = reader.read_motor_state(motor_id=13)
            
            if state:
                # Tokenize into graph concepts
                percept = tokenizer.tokenize(state)
                percepts.append(percept)
                
                # Show what enters working memory / global workspace
                if percept['salience'] > 0.3:
                    print(f"  → HIGH SALIENCE! Would enter global workspace")
                
                if percept['novelty'] > 0.5:
                    print(f"  → HIGH NOVELTY! Unexpected state")
            else:
                print("  [WARNING] No feedback from motor")
            
            time.sleep(0.5)
        
        # Show semantic memory
        tokenizer.print_semantic_memory()
        
        # Demonstrate reasoning
        print("\n" + "=" * 70)
        print("REASONING EXAMPLE")
        print("=" * 70)
        print("\nSuppose Melvin reasons: 'I should move Motor13 to 2.5 radians'")
        print("\nStep 1: Find target position concept")
        target_pos_discrete = tokenizer.discretize_position(2.5)
        print(f"  2.5 rad discretizes to bin: {target_pos_discrete:.2f} rad")
        
        if target_pos_discrete in tokenizer.position_concepts:
            target_node = tokenizer.position_concepts[target_pos_discrete]
            print(f"  Concept already exists: NodeID {target_node}")
        else:
            print(f"  Would create new concept: Position_{target_pos_discrete:.2f}rad")
            target_node = tokenizer.get_or_create_concept(
                target_pos_discrete, 
                tokenizer.position_concepts, 
                "Position"
            )
        
        print("\nStep 2: Detokenize to motor command")
        motor_node = tokenizer.motor_nodes[13]
        command = tokenizer.detokenize(motor_node, target_node)
        print(f"  Motor {command['motor_id']} target: {command['target_position']:.2f} rad")
        
        print("\nStep 3: Send to motor controller")
        print(f"  Would compute control: current={percepts[-1]['raw_state']['position']:.3f}rad, target={command['target_position']:.2f}rad")
        error = command['target_position'] - percepts[-1]['raw_state']['position']
        print(f"  Error: {error:.3f} rad")
        
        print("\n" + "=" * 70)
        print("KEY INSIGHT")
        print("=" * 70)
        print("""
Motor state is now represented as graph concepts!

This means:
✓ Motor state can participate in reasoning
✓ Patterns learned in semantic memory (position → next position)
✓ Predictions possible (where will motor be next?)
✓ Integration with vision/audio (see object → move motor)
✓ Language generation ("Motor13 is at 2.5 radians")
✓ Goal-driven control (reason about desired states)

The graph-based representation allows Melvin to THINK about motors,
not just react to them!
        """)
        
    except KeyboardInterrupt:
        print("\n\nStopped by user")
    finally:
        reader.disconnect()
        print("\nDemo complete!")


if __name__ == "__main__":
    demo_motor_tokenization()





