# CyberRC

CyberRC implements a gamepad emulator, and PPM generator on an embedded system to allow classical and machine controllers to interface with cyber systems in video games, as well as physical RC vehicles.
CyberRC uses a Teensy 4.1 to simulate an Xbox gamepad, or to generate a PPM signal to drive an RC controller based on input Protobuf messages delivered over Serial.

The initial use case is to interface automatic control with the game "Liftoff: Micro Drones". Liftoff provides state feedback over UDP.
Combined with the CyberRC, closed loop control can be implemented to allow for autonomous flight.

## Implementation Notes

This implementation does not use the "native" PPM library for the teensy due to an issue with the system crashing on initialization.
This is likely a surmountable issue, but a custom implementation is currently being used to save time on debug.
When possible, the native library should be used for better performance and more outputs.
For users requiring the higher resolution PPM, or for more outputs, see the implementation in CyberTX, which does not include firmware for gamepad emulation.
