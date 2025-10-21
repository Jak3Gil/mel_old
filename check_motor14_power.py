#!/usr/bin/env python3
"""
Check if Motor 14 is powered and on CAN bus
"""

print("=" * 70)
print("   MOTOR 14 STATUS CHECK")
print("=" * 70)
print()
print("Please check the following:")
print()
print("1. Is Motor 14 POWERED ON?")
print("   - Check for LED lights on motor")
print("   - Verify power supply is connected")
print("   - Check voltage (should be 12-48V)")
print()
print("2. Is Motor 14 PHYSICALLY PRESENT?")
print("   - Count how many motors you have connected")
print("   - Are there labels showing motor IDs?")
print()
print("3. Is Motor 14 on the CAN BUS?")
print("   - CANH wire connected?")
print("   - CANL wire connected?")
print("   - Same CAN bus as Motor 13?")
print("   - Or on a different CAN bus?")
print()
print("4. Does Motor 14 respond in Robstride Software?")
print("   - Open Robstride software")
print("   - Click 'Scan' or 'Connect'")
print("   - Does it show TWO motors (13 and 14)?")
print("   - Or only ONE motor (13)?")
print()
print("=" * 70)
print()

response = input("Do you see BOTH motors in Robstride software? (yes/no): ").strip().lower()

if response == 'yes':
    print("\n✓ Both motors detected by Robstride")
    print("\nThis means motor 14 IS on the CAN bus.")
    print("We need to find its encoding pattern.")
    print("\nNext step: Capture the hex log when Robstride detects BOTH motors")
    print("  - Close all programs")
    print("  - Start serial monitor on COM3")
    print("  - Open Robstride software")
    print("  - Copy the ENTIRE hex output")
    print("  - Look for TWO different MCU IDs in responses")
    
elif response == 'no':
    print("\n⚠️  Only Motor 13 detected by Robstride")
    print("\nThis means:")
    print("  - Motor 14 might not be powered")
    print("  - Motor 14 might not be on the CAN bus")
    print("  - Motor 14 might be on a different COM port")
    print("  - You might only have one motor")
    print("\nWhat to check:")
    print("  1. Count physical motors - how many do you have?")
    print("  2. Check Device Manager for multiple COM ports")
    print("  3. Try unplugging/replugging Motor 14 power")
    
else:
    print("\nPlease verify motor status and try again")

