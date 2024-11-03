#include <Ethernet.h>
#include <EthernetUdp.h>
#include <XInput.h>

#include "RCData.pb.h"
// #include <SoftwareSerial.h>
// #include <nanopbUdp.h>

#include <nanopbSerial.h>
#include <pb.h>
#include "usb_desc.h"

#define SERIAL_MODE
#define DEBUG

// Config
unsigned long now, previous;
// TODO: not implemented. wire to a switch to enable/disable control output
const int SafetyPin = 34;
const int ledPin = 13;
bool buttonState = false;

cyberrc_CyberRCMessage_MessageType message_type = cyberrc_CyberRCMessage_MessageType_RCData;
cyberrc_RCData rc_message = cyberrc_RCData_init_zero;
cyberrc_RCData last_rc_message = cyberrc_RCData_init_zero;

// Config Settings
const unsigned long CycleTime = 5000; // ms

bool proto_decode_status;
#ifdef SERIAL_MODE
uint8_t serial_read_buffer[32768];
uint8_t serial_write_buffer[4096];
pb_istream_t stream;
#else
// Ethernet Setup
EthernetUDP Udp;
IPAddress ip;
unsigned int localPort = 6969;
Stream &proto = Udp;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};
#endif


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
  // pinMode(SafetyPin, INPUT_PULLUP);
  Serial1.begin(921600);
  Serial1.addMemoryForRead(&serial_read_buffer, sizeof(serial_read_buffer));
  Serial1.addMemoryForWrite(&serial_write_buffer, sizeof(serial_write_buffer));

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
  if (Serial1.available() > 0) { 
    int len = Serial1.read();
    stream = pb_istream_from_serial(Serial1, len); 
    rc_message = cyberrc_RCData_init_zero;
    proto_decode_status =
        pb_decode(&stream, cyberrc_RCData_fields, &rc_message); 
    #ifdef DEBUG
      Serial1.printf("Decode status %d\n", proto_decode_status);
    #endif
    if (proto_decode_status) {
      // Process XInput Output
      int l_axis_x = CLAMP(rc_message.Rudder, -32768, 32767);
      int l_axis_y = CLAMP(rc_message.Throttle, -32768, 32767);
      int r_axis_x = CLAMP(rc_message.Aileron, -32768, 32767);
      int r_axis_y = CLAMP(rc_message.Elevator, -32768, 32767);

      XInput.setJoystick(JOY_LEFT, l_axis_x, l_axis_y);
      XInput.setJoystick(JOY_RIGHT, r_axis_x, r_axis_y);
      XInput.send();
      #ifdef DEBUG
        XInput.printDebug(Serial1);
      #endif
    }
  }
  else {
    buttonState = !buttonState;
    digitalWrite(13, buttonState);
    // XInput.send();
    // XInput.printDebug(Serial1);
    // delay(1000); 
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
}
