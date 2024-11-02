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

// Custom callback to read bytes from an Arduino Serial object
bool read_from_serial(pb_istream_t *stream, uint8_t *buf, size_t count) {
    HardwareSerial *serial = (HardwareSerial *)stream->state;
    
    // Wait until enough bytes are available or timeout
    unsigned long startTime = millis();
    while ((size_t)serial->available() < sizeof(count)) {
        if (millis() - startTime > 1000) { // 1-second timeout, adjust as needed
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