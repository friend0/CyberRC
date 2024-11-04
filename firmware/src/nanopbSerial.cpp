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

// Function to read data from the serial interface into a buffer
size_t read_serial_to_buffer(uint8_t *buffer, size_t buffer_size) {
    size_t bytesRead = 0;
    unsigned long startTime = micros();

    // Read bytes until the buffer is full or timeout occurs
    while (bytesRead < buffer_size) {
        if (Serial1.available()) {
            buffer[bytesRead++] = Serial1.read();
        } else if (micros() - startTime > 750) {
            // Stop reading if timeout is reached
            break;
        } else {
            delayMicroseconds(10);  // Small delay to wait for incoming data
        }
    }

    return bytesRead;
}

// Custom callback to read bytes from an Arduino Serial object
bool read_from_serial(pb_istream_t *stream, uint8_t *buf, size_t count) {
    HardwareSerial *serial = (HardwareSerial *)stream->state;
    // Wait until enough bytes are available or timeout
    unsigned long startTime = micros();
    while ((size_t)serial->available() < count) {
        if (micros() - startTime > 750) { // 100 microsecond timeout, adjust as needed
            return false;
        }
    }
    
    // Read the required number of bytes
    serial->readBytes(buf, count);
    for (int i = 0; i < count; i++) {
        Serial1.printf("%02X ", buf[i]);
    }
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
