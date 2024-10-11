#include <Arduino.h>
#include <nanopbSerial.h>
#include <pb.h>
#include "RCData.pb.h"

cyberrc_RCData rc_message = cyberrc_RCData_init_zero;
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
        proto_decode_status = pb_decode(&pb_in, cyberrc_RCData_fields, &rc_message);
        if (!proto_decode_status) {
            return;
        }
        // You can extend this logic for more buttons/axes based on serial input
    } else {
        Joystick.X(0);
        Joystick.button(1, val);
        val = !val;
    }

    delay(250);  // Small delay for stability
}
