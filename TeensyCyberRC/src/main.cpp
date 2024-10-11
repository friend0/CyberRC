#include <Arduino.h>
#include <nanopbSerial.h>
#include <pb.h>
#include "RCData.pb.h"

cyberrc_RCData rc_message = cyberrc_RCData_init_zero;
cyberrc_RCData last_rc_message = cyberrc_RCData_init_zero;
Stream& proto = Serial;

void setup() {
    // Start the serial communication
    // Serial.begin(115200);

    // Initialize the Joystick emulation
    Joystick.begin();
    bool val = 0;
}

void loop() {
    static bool val = 0;
    // Check if serial data is available
    bool proto_decode_status;
    if (Serial.available() > 0) { 

        pb_istream_s pb_in;
        pb_in = pb_istream_from_serial(proto, 36);
        proto_decode_status = pb_decode(&pb_in, cyberrc_RCData_fields, &c_message);
        if (proto_decode_status) {
            message_handler(rc_message, last_rc_message);
            last_rc_message = rc_message;
        } else {
          Serial.println("Failed to read message");
        }
        // You can extend this logic for more buttons/axes based on serial input
    } else {
        Joystick.X(0);
        Joystick.button(1, val);
        val = !val;
    }

    delay(250);  // Small delay for stability
}

void messaage_handler(cyberrc_RCData &rc_message, cyberrc_RCData &last_rc_message)
    // Handle the message here
    if (rc_message.throttle != last_rc_message.throttle) {
      Joystick.Y(rc_message.throttle);
    }
    if (rc_message.roll != last_rc_message.roll) {
      Joystick.RotateZ(rc_message.roll);
    }
    if (rc_message.pitch != last_rc_message.pitch) {
      Joystick.Z(rc_message.pitch);
    }
    if (rc_message.yaw != last_rc_message.yaw) {
      Joystick.X(rc_message.yaw);
    }
    // TODO: figure out the correct maping for arm and mode buttons
}
