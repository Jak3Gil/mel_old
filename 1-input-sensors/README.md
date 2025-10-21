# 1. Input / Sensors

**Pipeline Stage: Raw sensory input**

This directory contains all components that bring raw data into Melvin's cognitive system.

## Components

### Vision Input
- `vision/` - Camera input processing, vision bridge
- `visualize_camera.py`, `vision_learning.py` - Vision system tools
- `perception/` - V2 perception interfaces

### Audio Input  
- `audio/` - (Empty, audio tokenization moved to stage 2)

### Motor Feedback (Proprioception)
- `*motor*.py` - Motor control and sensor feedback
- `robstride_control.py` - Motor interface
- `listen_serial.py` - Serial sensor data

### I/O Systems
- `io/` - General input/output handlers

## Flow
Raw sensory data → Tokenization (Stage 2)

