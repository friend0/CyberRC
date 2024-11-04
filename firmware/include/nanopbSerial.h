#ifndef NANO_PB_H
#define NANO_PB_H

#include "pb_encode.h"
#include "pb_decode.h"
#include <Arduino.h>

pb_ostream_s pb_ostream_from_serial(Print& p);
pb_istream_s pb_istream_from_serial(Stream& s, size_t msglen);

size_t read_serial_to_buffer(uint8_t *buffer, size_t buffer_size);

bool decode_channel_values(pb_istream_t *stream, const pb_field_iter_t *field, void **arg);

#endif

