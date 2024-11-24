#ifndef NANO_PB_H
#define NANO_PB_H

#include "pb_encode.h"
#include "pb_decode.h"
#include "RCData.pb.h"
// TODO: configure this to match paramters on windows PC
#include "PulsePosition.h"

#include <Arduino.h>
#include "config.h"

extern uint8_t SERIAL_READ_BUFFER[32768];
extern uint8_t SERIAL_WRITE_BUFFER[4096];

// Structure to hold the decoded payload
typedef struct
{
  int type;
  u_int8_t channel_values_count;
  u_int32_t channel_values[MAX_NUM_CHANNELS];

  union
  {
    cyberrc_RCData controller_data;
    cyberrc_PPMUpdateAll ppm_data;
  } payload;
} MessageWrapper;

typedef struct
{
  u_int8_t channel_values_count;
  u_int32_t *channel_values[MAX_NUM_CHANNELS];
} PPMPayload;

// Message Wrapper
extern cyberrc_CyberRCMessage message;
// RC Messages
extern cyberrc_RCData controller_data;
extern cyberrc_RCData last_controller_data;
// PPM Messages
extern cyberrc_PPMUpdateAll ppm_data;

pb_ostream_s pb_ostream_from_serial(Print& p);

pb_istream_s pb_istream_from_serial(Stream& s, size_t msglen);

size_t read_serial_to_buffer(uint8_t *buffer, size_t buffer_size);

bool decode_channel_values(pb_istream_t *stream, const pb_field_iter_t *field, void **arg);

bool skip_inner_message_callback(pb_istream_t *stream, const pb_field_t *field, void **arg);

bool decode_channel_values(pb_istream_t *stream, const pb_field_iter_t *field, void **arg);

bool decode_inner_message_callback(pb_istream_t *stream, const pb_field_t *field, void **arg);

#endif