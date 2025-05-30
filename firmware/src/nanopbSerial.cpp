#include "nanopbSerial.h"
#include <Stream.h>
#include "RCData.pb.h"
#include <Arduino.h>

// Message Wrapper
cyberrc_CyberRCMessage message = cyberrc_CyberRCMessage_init_zero;
// RC Messages
cyberrc_RCData controller_data = cyberrc_RCData_init_zero;
cyberrc_RCData last_controller_data = cyberrc_RCData_init_zero;
// PPM Messages
cyberrc_PPMUpdateAll ppm_data = cyberrc_PPMUpdateAll_init_zero;

static bool pb_print_write(pb_ostream_t *stream, const pb_byte_t *buf, size_t count)
{
    Print *p = reinterpret_cast<Print *>(stream->state);
    size_t written = p->write(buf, count);
    return written == count;
};

pb_ostream_s pb_ostream_from_serial(Print &p)
{
    return {pb_print_write, &p, SIZE_MAX, 0};
};

// Function to read data from the serial interface into a buffer
size_t read_serial_to_buffer(uint8_t *buffer, size_t buffer_size) {
    size_t bytesRead = 0;
    unsigned long startTime = millis();
    // Read bytes until the buffer is full or timeout occurs
    while (bytesRead < buffer_size) {
        if (Serial1.available()) {
            buffer[bytesRead++] = Serial1.read();
            // Tune this timeout to the baudrate and maximum message size
        } else if (millis() - startTime > 5) {
            // Stop reading if timeout is reached
            break;
        } else {
          // Small delay to wait for incoming data
          // Tune this value to the baudrate and maximum message size
          delayMicroseconds(20);  // Small delay to wait for incoming data
        }
    }

    return bytesRead;
}

// Custom callback to read bytes from an Arduino Serial object
bool read_from_serial(pb_istream_t *stream, uint8_t *buf, size_t count) {
    HardwareSerial *serial = (HardwareSerial *)stream->state;
    // Wait until enough bytes are available or timeout
    unsigned long startTime = millis();
    while ((size_t)serial->available() < count) {
        if (millis() - startTime > 5) {
            return false;
        }
    }
    // Read the required number of bytes
    serial->readBytes(buf, count);
    return true;
}

// Function to create a Nanopb input stream from a Serial object
pb_istream_t pb_istream_from_serial(Stream &serial, size_t msglen) {
    return pb_istream_t {
        .callback = &read_from_serial,
        .state = &serial,
        .bytes_left = msglen,
        .errmsg = NULL
    };
}

// Callback function for the first pass to skip the inner field
bool skip_inner_message_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
  // Indicate to skip the field during the first pass
  stream->errmsg = "Skipping inner message during type read";
  return false;
}

bool decode_channel_values(pb_istream_t *stream, const pb_field_iter_t *field, void **arg)
{
  PPMPayload *context = (PPMPayload*)(*arg);
  int i = 0;
  uint32_t value;
  while (stream->bytes_left && i < context->channel_values_count)
  {
    if (!pb_decode_varint32(stream, &value))
      return false;
    context->channel_values[i] = value;
    i++;
  }
  if (i != context->channel_values_count || stream ->bytes_left) {
    return false;
  }
  return true;
}

// Callback function to decode the nested message based on the type field in OuterMessage
bool decode_inner_message_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
  MessageWrapper *decoded_payload = (MessageWrapper *)*arg;
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
    PPMPayload ppm_payload = {
        static_cast<uint8_t>(decoded_payload->channel_values_count),
        {1500, 1500, 1000, 1500}
    };
    msg.channel_values.arg = &ppm_payload;
    msg.channel_values.funcs.decode = decode_channel_values;
    if (!pb_decode(stream, cyberrc_PPMUpdateAll_fields, &msg))
    {
      return false;
    }
    decoded_payload->payload.ppm_data = msg;
    decoded_payload->channel_values_count = ppm_payload.channel_values_count;
    for (int i = 0; i < ppm_payload.channel_values_count; i++) {
        decoded_payload->channel_values[i] = ppm_payload.channel_values[i];
    }
  }
  else
  {
    return false;
  }
  return true;
}
