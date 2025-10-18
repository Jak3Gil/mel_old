# K-Scale Architecture - Quick Start Guide

## ✅ Status: TESTED & WORKING

The K-Scale Labs motor control architecture has been successfully implemented and validated with your Robstride motor.

---

## Quick Test

Run the full test suite to verify everything works:

```bash
python3 test_kscale_approach.py
```

**Expected:** All tests pass with green checkmarks ✓

---

## Simple Usage

### Example 1: Basic Movement

```bash
python3 example_kscale_usage.py 1
```

### Example 2: Custom Gains

```bash
python3 example_kscale_usage.py 2
```

### Example 3: State Monitoring

```bash
python3 example_kscale_usage.py 3
```

### Example 5: View Configuration

```bash
python3 example_kscale_usage.py 5
```

---

## Use in Your Own Code

### Minimal Example

```python
from motor_factory import MotorFactory
import time

# Create and connect
motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
motor.connect()
motor.enable()
motor.calibrate()

# Move
for _ in range(50):
    motor.set_position(1.0)  # 1 radian
    time.sleep(0.02)

# Cleanup
motor.disable()
motor.disconnect()
```

### With Custom Gains

```python
motor.set_position(
    position=0.5,      # Target position (rad)
    velocity=0.0,      # Target velocity (rad/s)
    kp=50.0,          # Position gain
    kd=2.0,           # Derivative gain
    torque=0.0        # Feedforward torque (Nm)
)
```

---

## Configuration

Edit `motor_config.yaml` to change motor settings:

```yaml
motors:
  main_motor:
    can_id: 0x01           # Motor CAN ID
    control:
      default_kp: 30.0     # Default position gain
      default_kd: 1.5      # Default derivative gain
```

---

## Adding More Motors

1. Edit `motor_config.yaml`:

```yaml
motors:
  left_motor:
    type: "robstride"
    can_id: 0x01
    # ... config ...
  
  right_motor:
    type: "robstride"
    can_id: 0x02
    # ... config ...
```

2. Use in code:

```python
motors = MotorFactory.create_from_file('motor_config.yaml')

for name, motor in motors.items():
    motor.connect()
    motor.enable()

# Control all motors
for _ in range(50):
    for motor in motors.values():
        motor.set_position(0.5)
    time.sleep(0.02)
```

---

## Files

| File | Purpose |
|------|---------|
| `motor_config.yaml` | Motor configuration |
| `motor_interface.py` | Abstract motor interface |
| `robstride_kscale.py` | Robstride implementation |
| `motor_factory.py` | Factory for creating motors |
| `test_kscale_approach.py` | Full test suite |
| `example_kscale_usage.py` | Usage examples |

---

## Advantages

✅ **Clean abstraction** - No more direct SLCAN commands  
✅ **Configuration-driven** - Change parameters without code changes  
✅ **Multi-motor ready** - Easy to add more motors  
✅ **State tracking** - Automatic position/velocity tracking  
✅ **Factory pattern** - Clean object creation  
✅ **Extensible** - Easy to add new motor types  

---

## Comparison

### Before (Original Code)
```python
motor = RobstrideMotor(can_id=0x01)
motor.connect()
motor.enable()
motor.send_command(
    position=0.5,
    velocity=0.0,
    kp=30.0,  # hardcoded
    kd=1.5,   # hardcoded
    torque=0.0
)
```

### After (K-Scale Architecture)
```python
motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
motor.connect()
motor.enable()
motor.set_position(0.5)  # Uses defaults from config
```

**Result:** Cleaner, more maintainable, and scalable code!

---

## Next Steps

1. ✅ Test with: `python3 test_kscale_approach.py`
2. ✅ Try examples: `python3 example_kscale_usage.py 1`
3. ✅ Integrate into your Melvin project
4. 🔜 Add more motors as needed
5. 🔜 Build higher-level control (limbs, trajectories, etc.)

---

## Support

- **Full documentation:** See `KSCALE_ARCHITECTURE_SUCCESS.md`
- **Original working code:** Still available in `robstride_motor.py`
- **Configuration reference:** See `motor_config.yaml`

---

**Status:** ✅ Ready for production use!

