#include <Ethernet.h>
#include <EthernetUdp.h>
#include <XInput.h>

#include "RCData.pb.h"
// #include <SoftwareSerial.h>
// #include <nanopbUdp.h>

#include <nanopbSerial.h>
#include <pb.h>
#include "usb_desc.h"

#define UART_BUFFER_SIZE 24 * 5
#define PI 3.14159265358979323846
#define SERIAL_MODE

// Config
const int SafetyPin = 34;          // Ground this pin to prevent inputs
char uartBuffer[UART_BUFFER_SIZE]; // Buffer to store incoming string
const int ledPin = 13;
unsigned long now, previous;

// SoftwareSerial debug(0, 1);
bool proto_decode_status;

cyberrc_RCData rc_message = cyberrc_RCData_init_zero;
cyberrc_RCData last_rc_message = cyberrc_RCData_init_zero;
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};

// Config Settings
const unsigned long CycleTime = 5000; // ms
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

// Ethernet Setup
EthernetUDP Udp;
IPAddress ip;
unsigned int localPort = 6969;

Stream &proto = Udp;

// Joystick Setup
const int JoyMin = -1500;
const int JoyMax = 1500;
const double angle_precision =
    (2 * PI) / (CycleTime / 4); // 4 because 250 Hz update rate
double angle = 0.0;

void blink_loop(int pin, int delay_ms) {
  bool ledState = false;
  while (true) {
    digitalWrite(pin, ledState);
    delay(delay_ms);
  }
}

#define CLAMP(val, min_val, max_val) ((val) < (min_val) ? (min_val) : ((val) > (max_val) ? (max_val) : (val)))

void setup() {
  // Safety Pin Setup
  pinMode(SafetyPin, INPUT_PULLUP);
  // Debug Serial Setup
  Serial1.begin(115200);
  // xInput Setup
  XInput.setAutoSend(false);
  XInput.begin();

  #ifndef SERIAL_MODE
  // Ethernet Setup
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
  #endif
}


void loop() {
  #ifdef SERIAL_MODE
  if (Serial1.available()) { 
      proto_decode_status = false;
      pb_istream_s pb_in;
      pb_in = pb_istream_from_serial(Serial1, cyberrc_RCData_size);
      proto_decode_status =
          pb_decode(&pb_in, cyberrc_RCData_fields, &rc_message); 
      if (proto_decode_status) {
        if (Serial1.availableForWrite()) {
          // Process XInput Output
          int l_axis_x = CLAMP(rc_message.Throttle, -32768, 32767);
          int l_axis_y = CLAMP(rc_message.Rudder, -32768, 32767);
          int r_axis_x = CLAMP(rc_message.Aileron, -32768, 32767);
          int r_axis_y = CLAMP(rc_message.Elevator, -32768, 32767);

          XInput.setJoystick(JOY_LEFT, l_axis_x, l_axis_y);   // Clockwise
          XInput.setJoystick(JOY_RIGHT, r_axis_x, r_axis_y); // Counter-clockwise
      } else {
            Serial1.printf("Decode failed: %d\n", proto_decode_status);
            // TODO: send the last output up to the limit
      }
    }
  }
  else {
    Serial1.write("No data");
    XInput.setJoystick(JOY_LEFT, 0, 0);
    XInput.setJoystick(JOY_RIGHT, 0, 0);
    delay(5000); 
  }
  #endif

  #ifndef SERIAL_MODE
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    // read the packet into packetBufffer
    char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write("ack");
    Udp.endPacket();
    pb_istream_s pb_in;
    pb_in = pb_istream_from_udp(proto, 36);
    proto_decode_status = pb_decode(&pb_in, cyberrc_RCData_fields, &rc_message);
    if (!proto_decode_status) {
      Serial1.write("Decoding failed");
      blink_loop(ledPin, 250);
    }
  }
  #endif
    // Calculate joystick x/y values using trig
}

// void loop() {

//   if (digitalRead(SafetyPin) == LOW) {
//     return;
//   }
//   unsigned long t = millis(); // Get timestamp for comparison
//   // Send values to PC
//   XInput.send();
// }
