# CyberRC Firmware

The CyberRC Firmware runs onboard a Teensy microcontroller, and can emulate a gamepad or joystick, as well as act as an RC trainer output device.
The firmware for CyberRC takes Protobuf encoded messages over Serial as input, and output control commands as an emulated Xbox controller, or through a PPM signal that is usable by almost any RC radio for controlling vehicles.

CyberRC is the "server" counterpart to the client libraries in this repo.
Using a client library, an autonomous controller can send commands to video games or real drones.
The primary use case is the development of control algorithms.

## Teensy Version Support

CyberRC has been developed on the Teensy 4.1, but should be portable to the 4.0, and likely portable to 3.6 and 3.2.

### TODO: Compatibility matrix

# Getting Started - Flashing Firmware to a New Teensy

## Dependencies

- [VS Code](https://code.visualstudio.com/)
- [PlatformIO](https://platformio.org/): embedded development platform (installed through VS Code extension)
- [Xinput](https://github.com/dmadison/ArduinoXInput_Teensy): controller emulation (manual setup required)
- [Nanopb](https://jpa.kapsi.fi/nanopb/): small ansi C protobuf implementation (automatically handled by PlatformIO)

## Preliminaries

0. **Install VS Code**: Download and install [VS Code](https://code.visualstudio.com/) if you haven't already.

1. **Install PlatformIO in VS Code**:
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X)
   - Search for "PlatformIO IDE" and install it
   - Restart VS Code when prompted
   - Navigate to the PlatformIO extension in VS Code, go to "Platforms" and install the "teensy" platform

2. **Install XInput Board Support for Teensy**:
   - XInput is required to enable the Teensy to emulate an Xbox gamepad and appear as a USB game controller to the host system
   > 🚨 **WARNING - CRITICAL FOR PLATFORMIO USERS**
   >
   > The ArduinoXInput_Teensy installation instructions are written for Arduino IDE users. Since we're using PlatformIO, you must adapt these instructions to use the PlatformIO framework directory instead.
   >
   > **Use these PlatformIO paths (NOT the Arduino IDE paths in the documentation):**
   > - Windows: `C:\Users\[username]\.platformio\packages\framework-arduinoteensy`
   > - MacOS: `/Users/[username]/.platformio/packages/framework-arduinoteensy`  
   > - Linux: `/home/[username]/.platformio/packages/framework-arduinoteensy`

   **Installation Steps:**
   1. Download the [ArduinoXInput_Teensy](https://github.com/dmadison/ArduinoXInput_Teensy) repository
   2. Navigate to your PlatformIO framework directory (see paths above)
   3. Replace the `teensy3` and `teensy4` folders with the ones from the ArduinoXInput_Teensy repository
   4. ⚠️ **Warning**: When copying files, select merge to avoid overwriting all existing files - only replace the modified ones
   - **Note for macOS users**: The Library folder is hidden by default in Finder. To access it:
     1. In Finder, press `Cmd + Shift + G` (Go to Folder)
     2. Type `~/Library/` to access your user Library folder
   - For detailed installation instructions, see the [ArduinoXInput_Teensy documentation](https://github.com/dmadison/ArduinoXInput_Teensy)

3. **Configure PlatformIO for XInput**:
   > ⚠️ **WARNING**: You must add `USB_XINPUT` to the `BUILTIN_USB_FLAGS` in `$HOME/.platformio/platforms/teensy/builder/frameworks/arduino.py` for XInput functionality to work with PlatformIO.
   - If you skip this step, 3
   - The lines should look like this when you're done:

     ```python
      BUILTIN_USB_FLAGS = (
          "USB_SERIAL",
          "USB_DUAL_SERIAL",
          "USB_TRIPLE_SERIAL",
          "USB_KEYBOARDONLY",
          "USB_TOUCHSCREEN",
          "USB_HID_TOUCHSCREEN",
          "USB_HID",
          "USB_SERIAL_HID",
          "USB_MIDI",
          "USB_MIDI4",
          "USB_MIDI16",
          "USB_MIDI_SERIAL",
          "USB_MIDI4_SERIAL",
          "USB_MIDI16_SERIAL",
          "USB_AUDIO",
          "USB_MIDI_AUDIO_SERIAL",
          "USB_MIDI16_AUDIO_SERIAL",
          "USB_MTPDISK",
          "USB_RAWHID",
          "USB_FLIGHTSIM",
          "USB_FLIGHTSIM_JOYSTICK",
          "USB_EVERYTHING",
          "USB_DISABLED",
          "USB_MTPDISK_SERIAL",
          "USB_XINPUT"

      )```

4. **Clone and Import Project**:
   - Clone this repository: `git clone https://github.com/friend0/CyberRC.git`
   - Open VS Code and select "Open Folder"
   - Navigate to and select the `CyberRC/firmware` directory
   - PlatformIO should automatically detect the project configuration

5. **Compile the Code and Flash the Device:**
   - Make sure the teensy is connected via USB to your PC
   - Ensure the device is in programming mode by pressing the button on the top of the Teensy board - the red LED will glow a dim red, and will not blink
   - In VS Code, use the toolbar to build and upload the firmware. If you are looking at the main.cpp source file, it will show as a checkmark icon in the top right corner, with a dropdown arrow next to it. Click the arrow and select "Upload" to build and flash the firmware to the Teensy.

## Building and Flashing

Once you've completed the preliminaries above, PlatformIO should automatically detect the project configuration and install the necessary dependencies.

To build and flash:

1. Connect your Teensy via USB
2. In VS Code, use the PlatformIO toolbar to build and upload the firmware

If you encounter issues, refer to the automated setup in `.github/workflows/firmware-ci.yml` for reference.

# Configuration

## Building the firmware in Debug mode

Serial output is extremely useful for troubleshooting errors with the firmware without a JTAG or similar device.
On the other hand, serial output can affect the timing of operations in situations where tight timing constraints are desired. For this reason, we include a `#define DEBUG` variable that is commented by default in `config.h`.
To enable it, simply uncomment this line and rebuild the firmware.

## Building the Firmware for Multiple PPM Output Lines

In the base case for interfacing with vehicles using PPM, we use one line of PPM output.
For the multi-vehicle case - up to 8 outputs for Teensy3, and 10 for Teensy4 - you will need to configure and build the software for the desired number of PPM lines. In `config.h`, change `#define NUM_LINES 1` to match the desired number of output lines.

In your client code, you will set up some addressing scheme corresponding vehicles with some line number, and then include that line number in the PPM command message. Refer to the client library implementations for details on how to do this.

## PPM Output Pins

The firmware uses the following pins for PPM output:

- **Teensy 3.1/3.2**: Pins 5, 6, 9, 10, 20, 21, 22, 23 (default: pin 5)
- **Teensy 4.0/4.1**: Pins 6, 9, 10, 11, 12, 13, 14, 15, 18, 19 (default: pin 6)

By default, only one PPM line is used. See the configuration section below to enable multiple outputs.

## Building the Protobufs _Needed for manual compilation only_

Protobufs for the Teensy should be rebuilt automatically by PlatformIO.
The Teensy implementation in this case relies on `nanopb` which uses it's own protobuf compiler implementation.

If you need to rebuild the protobufs for the any of the test clients, install `protoc` locally, navigate to `src/protos` and run, for example:

`$ protoc --python_out=../../test_client --mypy_out=../../test_client RCData.proto`
