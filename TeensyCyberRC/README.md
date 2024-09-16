# TeensyCyberRC

If CyberRC is the virtual interface from controller, then TeensyCyberRC is the physical interface.
TeensyCyberRC runs onboard a Teensy 4.1, and emulates a gamepad or joystick. 
It consumers Protobuf encoded messages over Serial and outputs them as gamepad outputs.
This allows controllers to send outputs to video games or other applications that require game pad or joystick inputs.

# Rebuilding Protobufs
Protobufs for the Teensy should be rebuilt automatically by PlatformIO.
The Teensy implementation in this case relies on `nanopb` which uses it's own protobuf compiler implementation.

To rebuild the protobufs for the Python GUI test client, install `protoc` locally, navigate to `src/protos` and run:

`$ protoc --python_out=../../test_client --mypy_out=../../test_client RCData.proto`