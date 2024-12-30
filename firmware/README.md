# TeensyCyberRC

The embedded code for CyberRC takes Protobuf encoded messages over Serial as input, and outputs gamepad commands.
This allows controllers to send outputs to video games or other applications that require game pad or joystick inputs. The primary use case is the  development of control algorithms, and training ML policies.

If CyberRC is the virtual interface from an autonomous controller, then TeensyCyberRC is the physical interface.
TeensyCyberRC runs onboard a Teensy microcontroller, and emulates a gamepad or joystick.



# Teensy Support
CyberRC has been developed on the Teensy 4.1, but should be portable to the 4.0, and likely portable to 3.6 and 3.2.

# Dependencies and Development

## Dependencies
- Arduino IDE
- Teensy Loader: communicates with the bootloader to flash programs. Follow the instructions [here](https://www.pjrc.com/teensy/loader.html). It comes in a CLI version, or can be installed with the Teensy board files using the araduino IDE.
- Xinput: controller emulation
- Nanopb: small ansi C protobuf implementation

## Development
I'm using Platformio on VSCode. Platformio should pick up the config in this repo, and install the dependencies that you need to build and deploy this repository. 

You will need to install the Xinput boards package for teensy following the instructions in the repository.
Note that you will need to locate the directory of the teensy board files used by platformio, which will be distinct from those installed by the Arduino IDE. Find it [here](https://github.com/dmadison/ArduinoXInput_Teensy).

I'm including the complete `usb_desc.h` entry here:

```
#elif defined(USB_XINPUT)
  #define VENDOR_ID		0x045e
  #define PRODUCT_ID		0x028e
  #define MANUFACTURER_NAME	{0x00A9,'M','i','c','r','o','s','o','f','t'}
  #define MANUFACTURER_NAME_LEN	10
  #define PRODUCT_NAME		{'C','o','n','t','r','o','l','l','e','r'}
  #define PRODUCT_NAME_LEN	10
  #define DEVICE_CLASS 0xFF
  #define DEVICE_SUBCLASS 0xFF
  #define DEVICE_PROTOCOL 0xFF
  #define BCD_DEVICE 0x0114
  #define DEVICE_ATTRIBUTES 0xA0
  #define DEVICE_POWER	0xFA
  #define EP0_SIZE		64
  #define NUM_ENDPOINTS         6
  #define NUM_INTERFACE		4
  #define XINPUT_INTERFACE 0
  #define XINPUT_RX_ENDPOINT	2
  #define XINPUT_RX_SIZE 8
  #define XINPUT_TX_ENDPOINT	1
  #define XINPUT_TX_SIZE 20
  #define CONFIG_DESC_SIZE 153
  #define ENDPOINT1_CONFIG ENDPOINT_TRANSMIT_INTERRUPT
  #define ENDPOINT2_CONFIG ENDPOINT_RECEIVE_INTERRUPT
  #define ENDPOINT3_CONFIG ENDPOINT_TRANSMIT_INTERRUPT
  #define ENDPOINT4_CONFIG ENDPOINT_RECEIVE_INTERRUPT
  #define ENDPOINT5_CONFIG ENDPOINT_TRANSMIT_INTERRUPT + ENDPOINT_RECEIVE_INTERRUPT
  #define ENDPOINT6_CONFIG ENDPOINT_TRANSMIT_INTERRUPT
#endif
```

In normal Arduino development, you select the desired mode of USB operation through the editor. With platformio, this configuration is set in the platformio.ini. By default, platformio filters these build configurations to a default list that is populated to the stock options. You will need to add `USB_XINPUT` to the `BUILTIN_USB_FLAGS`
in `$HOME/.platformio/platforms/teensy/builder/frameworks/arduino.py` (Linux path).

# Configuring the Teensy for Xinput and HW Serial
By default, the Teensy doesn't come with a usb descriptor that enables Xinput.
There are some common adaptations used to add an Xinput descriptor, but it does not enable additional hardware serial. Some manual interventions are required to make this work.

# Building the Protobufs *Needed for manual compilation only*

Protobufs for the Teensy should be rebuilt automatically by PlatformIO.
The Teensy implementation in this case relies on `nanopb` which uses it's own protobuf compiler implementation.

If you need to rebuild the protobufs for the any of the test clients, install `protoc` locally, navigate to `src/protos` and run, for example:

`$ protoc --python_out=../../test_client --mypy_out=../../test_client RCData.proto`