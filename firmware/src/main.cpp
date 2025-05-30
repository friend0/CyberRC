#include "RCData.pb.h"
#include "XInput.h"
#include "config.h"
#include "nanopbSerial.h"
#include "utils.h"

// When operating in debug mode, you must ensure that the debugging code is
// actively clearing from the buffer, as there is currently no check on this
// end.
// #define DEBUG

unsigned long now, previous;
const int ledPin = 13;
volatile bool ledState = false;

pb_istream_t stream;
bool proto_decode_status;

// Joystick Setup
const int JoyMin = -1500;
const int JoyMax = 1500;

uint8_t buffer[255];
MessageWrapper message_wrapper;

void toggleLED() {
  ledState = !ledState;           // Toggle LED state
  digitalWrite(ledPin, ledState); // Update the LED pin
}

void setup() {
  previous = millis();
  pinMode(ledPin, OUTPUT); // Configure LED pin as output
  setup_serial();
#ifdef DEBUG
  Serial1.println("Serial Initialized");
#endif // Safety Pin Setup
  initialize_ppm();
#ifdef DEBUG
  Serial1.printf("Initialized PPM %d\n", ppm_output_pins[0]);
#endif // Safety Pin Setup
  // pinMode(SafetyPin, INPUT_PULLUP);
  // xInput Setup
  Serial1.flush();
  XInput.setAutoSend(false);
  XInput.begin();
}

void loop() {
  static long last_blink = 0;
  while (!Serial1.available()) {
    now = millis();
    if (now - last_blink > 500) {
      toggleLED();
      last_blink = millis();
    }
    // Hoodrat watchdog timer - set outputs to zero if no data has been received
    // in the last 5 seconds
    if (now - previous > 5000) {
      // Set PPMs to "zero"
      for (uint8_t i = 0; i < NUM_LINES; i++) {
        for (uint8_t j = 0; j < NUM_PPM_CHANNELS; j++) {
          ppm_output[i]->updateChannel(j, control_defaults[j]);
        }
        ppm_output[i]->begin();
      }

      // TODO: add default values to config header
      // Set XInput to "zero"
      XInput.setJoystick(JOY_LEFT, 0, 32767);
      XInput.setJoystick(JOY_RIGHT, 0, 0);
      XInput.send();
    }
  }

  if (Serial1.available()) {
    ledState = false;
    digitalWrite(ledPin, ledState);
  }

  CLEAR_BUFFER(buffer);

  // Read the first byte of the message, which is the length of the message.
  // Note this implies a message limit of 355 bytes.
  int message_size = Serial1.read();
#ifdef DEBUG
  Serial1.printf("Length of message %d\n", message_size);
#endif
  // With the given message size, read that number of bytes into the buffer.
  size_t bytesRead = read_serial_to_buffer(buffer, message_size);
  if (bytesRead != message_size) {
    // Error reading the message
#ifdef DEBUG
    Serial1.printf("Did not read full message %d\n", bytesRead);
#endif
    return;
  }

#ifdef DEBUG
  Serial1.printf("Buffer\n");
  for (int i = 0; i < bytesRead; i++) {
    Serial1.printf("%02X ", buffer[i]);
  }
  Serial1.println();
#endif
  // Get a stream from the buffer, and initialize the message wrapper.
  stream = pb_istream_from_buffer(buffer, bytesRead);
  message = cyberrc_CyberRCMessage_init_zero;
  // message.payload.funcs.decode = &skip_inner_message_callback;
  if (!pb_decode_noinit(&stream, cyberrc_CyberRCMessage_fields, &message)) {
    // Error decoding the outer message
    return;
  }
#ifdef DEBUG
  Serial1.printf("Decode outer type %d\n", message.type);
#endif
  // The message wrapper is our internal structure for holding the wrapper, and
  // the payload data. Clear the message wrapper to receive the new data.
  memset(&message_wrapper, 0, sizeof(MessageWrapper));
  message_wrapper.type = message.type;
  message_wrapper.channel_values_count = message.channel_values_count;

  // Set the payload callback to decode the inner message based on the type
  // field in the outer message. We will decode the data into the message
  // wrapper.
  stream = pb_istream_from_buffer(buffer, bytesRead);
  message.payload.funcs.decode = decode_inner_message_callback;
  message.payload.arg = &message_wrapper;
  if (!pb_decode(&stream, cyberrc_CyberRCMessage_fields, &message)) {
    // Error decoding the inner message
#ifdef DEBUG
    Serial1.println("Failed to decode inner type");
#endif
    return;
  } else {
    // Succesfully received some message, reset `previous` watchdog timer
    previous = millis();
  }

  // Based on the type of message, determine what interface to write output to.
#ifdef DEBUG
  Serial1.printf("Decode inner type %d\n", message.type);
#endif
  if (message_wrapper.type == cyberrc_CyberRCMessage_MessageType_RCData) {
    controller_data = message_wrapper.payload.controller_data;
    // Process XInput Output
    int l_axis_x = CLAMP(controller_data.Rudder, -32768, 32767);
    int l_axis_y = CLAMP(controller_data.Throttle, -32768, 32767);
    int r_axis_x = CLAMP(controller_data.Aileron, -32768, 32767);
    int r_axis_y = CLAMP(controller_data.Elevator, -32768, 32767);

    double l2 =
        sqrt((double)(l_axis_x * l_axis_x) + (double)(l_axis_y * l_axis_y));
    if (l2 > 32767) {
      double scale = (double)32767 / l2;
      l_axis_x = (int)(l_axis_x * scale);
      l_axis_y = (int)(l_axis_y * scale);
    }
    double r2 =
        sqrt((double)(r_axis_x * r_axis_x) + (double)(r_axis_y * r_axis_y));
    if (r2 > 32767) {
      double scale = (double)32767 / r2;
      r_axis_x = (int)(r_axis_x * scale);
      r_axis_y = (int)(r_axis_y * scale);
    }
    XInput.setJoystick(JOY_LEFT, l_axis_x, l_axis_y);
    XInput.setJoystick(JOY_RIGHT, r_axis_x, r_axis_y);
    XInput.send();
#ifdef DEBUG
    Serial1.printf("Decoded RCData\n");
    Serial1.printf("Aileron: %d\n",
                   message_wrapper.payload.controller_data.Aileron);
    Serial1.printf("Elevator: %d\n", controller_data.Elevator);
    Serial1.printf("Throttle: %d\n", controller_data.Throttle);
    Serial1.printf("Rudder: %d\n", controller_data.Rudder);
#endif
#ifdef DEBUG
    XInput.printDebug(Serial1);
#endif
  } else if (message_wrapper.type ==
             cyberrc_CyberRCMessage_MessageType_PPMUpdate) {
    ppm_data = message_wrapper.payload.ppm_data;
    uint32_t line = ppm_data.line;
    uint32_t *channel_values = message_wrapper.channel_values;
    if (line <= 0 || line > NUM_LINES) {
#ifdef DEBUG
      Serial1.printf("Line %d is out of bounds. Please provide a line in the "
                     "range [1-NUM_LINES]\r\n",
                     line);
#endif
      return;
    }
    for (uint8_t i = 0; i < message_wrapper.channel_values_count; i++) {
      ppm_output[line - 1]->updateChannel(i, channel_values[i]);
    }
  }
}
