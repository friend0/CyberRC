#include "nanopbSerial.h"
#include <Stream.h>

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

static bool pb_stream_read(pb_istream_t *stream, pb_byte_t *buf, size_t count)
{
    Stream *s = reinterpret_cast<Stream *>(stream->state);
    size_t read = s->readBytes(buf, count);
    return read == count;
};

pb_istream_s pb_istream_from_serial(Stream &s, size_t msglen)
{
    s.setTimeout(0);
#ifndef PB_NO_ERRMSG
    return {pb_stream_read, &s, msglen, 0};
#else
    for (int i = 0; i < (int)sizeof(ppm_message); i++)
    {
        Serial.read();
    }
    return {pb_stream_read, &s, msglen};
#endif
};



bool decode_channel_values(pb_istream_t *stream, const pb_field_iter_t *field, void **arg)
{
    int i = 0;
    uint32_t value;
    u_int32_t* values = static_cast<u_int32_t*>(*arg);
    while (stream->bytes_left)
    {
        if (!pb_decode_varint32(stream, &value))
            return false;
        values[i] = value;
        i++;
    }
    return true;
}