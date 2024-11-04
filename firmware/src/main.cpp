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

cyberrc_CyberRCMessage message = cyberrc_CyberRCMessage_init_zero;
cyberrc_RCData controller_data = cyberrc_RCData_init_zero;
cyberrc_RCData last_controller_data = cyberrc_RCData_init_zero;

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

#define CLAMP(val, min_val, max_val) ((val) < (min_val) ? (min_val) : ((val) > (max_val) ? (max_val) : (val)))

#define NUM_CHANNELS 4
// Structure to hold the decoded payload
typedef struct
{
  int type;
  u_int32_t channel_values[NUM_CHANNELS];

  union
  {
    cyberrc_RCData controller_data;
    cyberrc_PPMUpdateAll ppm_data;
  } payload;
} DecodedPayload;

// Callback function for the first pass to skip the inner field
bool skip_inner_message_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
  // Indicate to skip the field during the first pass
  stream->errmsg = "Skipping inner message during type read";
  return false;
}

bool decode_channel_values(pb_istream_t *stream, const pb_field_iter_t *field, void **arg)
{
  int i = 0;
  uint32_t value;
  u_int32_t *values = static_cast<u_int32_t *>(*arg);
  while (stream->bytes_left)
  {
    if (!pb_decode_varint32(stream, &value))
      return false;
    values[i] = value;
    i++;
  }
  return true;
}

// Callback function to decode the nested message based on the type field in OuterMessage
bool decode_inner_message_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
  DecodedPayload *decoded_payload = (DecodedPayload *)*arg;
  if (decoded_payload->type == 1)
  {
    cyberrc_RCData msg = cyberrc_RCData_init_zero;
    if (!pb_decode(stream, cyberrc_RCData_fields, &msg))
    {
      return false;
    }
    decoded_payload->payload.controller_data = msg;
  }
  else if (decoded_payload->type == 0)
  {
    cyberrc_PPMUpdateAll msg = cyberrc_PPMUpdateAll_init_zero;
    msg.channel_values.arg = decoded_payload->channel_values;
    msg.channel_values.funcs.decode = &decode_channel_values;
    if (!pb_decode(stream, cyberrc_PPMUpdateAll_fields, &msg))
    {
      return false;
    }
    decoded_payload->payload.ppm_data = msg;
  }
  else
  {
    return false;
  }
  return true;
}

void setup()
{
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
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    blink_loop(ledPin, 1000);
  }
  if (Ethernet.linkStatus() == LinkOFF)
  {
    blink_loop(ledPin, 3000);
  }
  if (Ethernet.begin(mac) == 0)
  {
    // TODO: blink led
    blink_loop(ledPin, 5000);
  }
  ip = Ethernet.localIP();
  Ethernet.begin(mac, ip);
  Udp.begin(ip);
#endif
}

void loop()
{
  while (!Serial1.available())
  {
  }

  uint8_t buffer[cyberrc_RCData_size];

  int len = Serial1.read();
#ifdef DEBUG
  Serial1.printf("Length of message %d\n", len);
#endif
  size_t bytesRead = read_serial_to_buffer(buffer, len);
  if (bytesRead != len)
  {
    // Error reading the message
#ifdef DEBUG
  Serial1.printf("Did not read full message %d\n", bytesRead);
#endif
    return;
  }

#ifdef DEBUG
  Serial1.printf("Buffer\n");
  for (int i = 0; i < bytesRead; i++)
  {
    Serial1.printf("%02X ", buffer[i]);
  }
  Serial1.println();
#endif
  stream = pb_istream_from_buffer(buffer, bytesRead);
  message = cyberrc_CyberRCMessage_init_zero;
  // message.payload.funcs.decode = &skip_inner_message_callback;

  if (!pb_decode_noinit(&stream, cyberrc_CyberRCMessage_fields, &message))
  {
    // Error decoding the message
    return;
  }
#ifdef DEBUG
  Serial1.printf("Decode outer type %d\n", message.type);
#endif
  DecodedPayload decoded_payload;
  decoded_payload.type = message.type;
  // memset(decoded_payload.channel_values, 0, sizeof(decoded_payload.channel_values));
  // memset(&decoded_payload.payload.controller_data, 0, sizeof(decoded_payload.payload.controller_data));
  // memset(&decoded_payload.payload.ppm_data, 0, sizeof(decoded_payload.payload.ppm_data));

  stream = pb_istream_from_buffer(buffer, bytesRead);
  message.payload.funcs.decode = decode_inner_message_callback;
  message.payload.arg = &decoded_payload;
  if (!pb_decode(&stream, cyberrc_CyberRCMessage_fields, &message))
  {
    // Error decoding the message
    return;
  }

#ifdef DEBUG
  Serial1.printf("Decode inner type %d\n", message.type);
#endif
  if (decoded_payload.type == cyberrc_CyberRCMessage_MessageType_RCData)
  {
    controller_data = decoded_payload.payload.controller_data;
    // Process XInput Output
    int l_axis_x = CLAMP(controller_data.Rudder, -32768, 32767);
    int l_axis_y = CLAMP(controller_data.Throttle, -32768, 32767);
    int r_axis_x = CLAMP(controller_data.Aileron, -32768, 32767);
    int r_axis_y = CLAMP(controller_data.Elevator, -32768, 32767);

    XInput.setJoystick(JOY_LEFT, l_axis_x, l_axis_y);
    XInput.setJoystick(JOY_RIGHT, r_axis_x, r_axis_y);
    XInput.send();
#ifdef DEBUG
    Serial1.printf("Decoded RCData\n");
    Serial1.printf("Aileron: %d\n", decoded_payload.payload.controller_data.Aileron);
    Serial1.printf("Elevator: %d\n", controller_data.Elevator);
    Serial1.printf("Throttle: %d\n", controller_data.Throttle);
    Serial1.printf("Rudder: %d\n", controller_data.Rudder);
#endif
#ifdef DEBUG
    XInput.printDebug(Serial1);
#endif
  } else if (decoded_payload.type == 0)
  {
    cyberrc_PPMUpdateAll msg = cyberrc_PPMUpdateAll_init_zero;
    msg.channel_values.arg = decoded_payload.channel_values;
    msg.channel_values.funcs.decode = &decode_channel_values;
    if (!pb_decode(&stream, cyberrc_PPMUpdateAll_fields, &msg))
    {
      return;
    }
    decoded_payload.payload.ppm_data = msg;
  }
}