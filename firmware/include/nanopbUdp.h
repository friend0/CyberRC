#ifndef NANO_PB_H
#define NANO_PB_H

#include "pb_encode.h"
#include "pb_decode.h"
#include <Arduino.h>

pb_ostream_s pb_ostream_from_udp(Print& p);
pb_istream_s pb_istream_from_udp(Stream& s, size_t msglen);

bool decode_channel_values_udp(pb_istream_t *stream, const pb_field_iter_t *field, void **arg);

#endif

