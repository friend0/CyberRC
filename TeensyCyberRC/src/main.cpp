#include <Ethernet.h>
#include <EthernetUdp.h>
#include <XInput.h>

#include <nanopbSerial.h>
#include <pb.h>
#include "RCData.pb.h"

cyberrc_RCData rc_message = cyberrc_RCData_init_zero;
cyberrc_RCData last_rc_message = cyberrc_RCData_init_zero;
Stream& proto = Udp;

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

// Config Settings
const unsigned long CycleTime = 5000;  // ms
const int SafetyPin = 0;  // Ground this pin to prevent inputs
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

// Button Setup
const int NumButtons = 10;
const int Buttons[NumButtons] = {
	BUTTON_A,
	BUTTON_B,
	BUTTON_X,
	BUTTON_Y,
	BUTTON_LB,
	BUTTON_RB,
	BUTTON_BACK,
	BUTTON_START,
	BUTTON_L3,
	BUTTON_R3,
};

const unsigned long ButtonDuration = CycleTime / NumButtons;
unsigned long buttonTimeLast = 0;
int buttonID = 0;

// DPad Setup
const int NumDirections = 4;
const unsigned long DPadDuration = CycleTime / NumDirections;
unsigned long dpadTimeLast = 0;
int dpadPosition = 0;

// Ethernet Setup
EthernetUDP Udp;
IPAddress ip;
unsigned int localPort = 6969;

// Triggers
const int TriggerMax = 255;  // uint8_t max
const unsigned long TriggerDuration = CycleTime / (TriggerMax * 2);  // Go up and down
unsigned long triggerTimeLast = 0;
uint8_t triggerVal = 0;
boolean triggerDirection = 0;
#include <SoftwareSerial.h>

SoftwareSerial debug(0, 1);

// Config Settings
const unsigned long CycleTime = 5000;  // ms
const int SafetyPin = 34;  // Ground this pin to prevent inputs


// Joystick Setup
const int JoyMax = 32767;  // int16_t max
const double angle_precision = (2 * PI) / (CycleTime / 4);  // 4 because 250 Hz update rate
double angle = 0.0;

const int ledPin = 13;

void blink_loop(int pin, int delay_ms) {
    bool ledState = false;
    while (true) {
        digitalWrite(pin, ledState);
        delay(delay_ms);
    }
}

void setup() {
	pinMode(SafetyPin, INPUT_PULLUP);
	XInput.setAutoSend(false);  // Wait for all controls before sending
	XInput.begin();
    // TODO: replace delays with a blink loop
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        blink_loop(ledPin, 1000);
    }
    if (Ethernet.linkStatus() == LinkOFF) {
        blink_loop(ledPin, 3000);
    }
    if (Ethernet.begin(mac) == 0) {
        // TODO: blink led
        blink_loop(ledPin, 5000);
    }
    ip = Ethernet.localIP();
    Ethernet.begin(mac, ip);
    Udp.begin(ip);
}

void loop() {
    bool proto_decode_status;

    int packetSize = Udp.parsePacket();
    if (packetSize) {
        // read the packet into packetBufffer
        char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
        Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        // send a reply to the IP address and port that sent us the packet we received
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write("ack");
        Udp.endPacket();
        pb_istream_s pb_in;
        pb_in = pb_istream_from_serial(proto, 36);
        proto_decode_status = pb_decode(&pb_in, cyberrc_RCData_fields, &rc_message);
        if (!proto_decode_status) {
            blink_loop(ledPin, 250);
        }
    }
    
  debug.begin(115200);
}

void loop() {


	if (digitalRead(SafetyPin) == LOW) {
		return;
	}

	unsigned long t = millis();  // Get timestamp for comparison

	// DPad
	if (t - dpadTimeLast >= DPadDuration) {  // If enough time has passed, change the dpad
		XInput.setDpad(dpadPosition == 0, dpadPosition == 1, dpadPosition == 2, dpadPosition == 3);

		dpadPosition++;  // Increment the dpad counter
		if (dpadPosition >= NumDirections) dpadPosition = 0;  // Go back to 0 if we hit the limit
		dpadTimeLast = t;  // Save time we last did this
	}

	// Buttons
	if (t - buttonTimeLast >= ButtonDuration) {  // If enough time has passed, change the button pressed
		for (int i = 0; i < NumButtons; i++) {
			XInput.release((XInputControl)Buttons[i]);  // Relase all buttons
		}
		
		XInput.press((XInputControl)Buttons[buttonID]); // Press the next button
		buttonID++;  // Increment the button counter
		if (buttonID >= NumButtons) buttonID = 0;  // Go back to 0 if we hit the limit

		buttonTimeLast = t;  // Save time we last did this
	}

	// Calculate joystick x/y values using trig
	int axis_x = sin(angle) * JoyMax;
	int axis_y = cos(angle) * JoyMax;

	angle += angle_precision;
	if (angle >= 360) {
		angle -= 360;
	}

	XInput.setJoystick(JOY_LEFT, axis_x, axis_y);  // Clockwise
	XInput.setJoystick(JOY_RIGHT, -axis_x, axis_y);  // Counter-clockwise

	// Send values to PC
	XInput.send();
}
