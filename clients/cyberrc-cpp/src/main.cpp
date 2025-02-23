#include "cyberrc_bindings.h"
#include <cstdint>
#include <stdint.h>
#include <stdio.h>

int main() {

  // Initialize Writer instance - the serial port name my look different
  // depending on your system.
  Writer *writer = writer_new("/dev/tty.usbserial-B001JE6N", 115200);
  if (!writer) {
    printf("Failed to initialize Writer!\n");
    return 1;
  }

  // Construct a PPM message
  int32_t channels[] = {1000, 1500, 2000, 2500};
  PpmUpdateAllWrapper msg = {.line = 1,
                             .channel_values = channels, // Point to the array
                             .channel_values_count =
                                 sizeof(channels) / sizeof(channels[0])};
  // Encode the message
  size_t *out_len = 0;
  uint8_t *msg_enc = ppmupdate_encode(msg, out_len);

  // Write the message
  uint32_t result = writer_write(writer, CyberRCMessageTypeC::PpmUpdate,
                                 msg_enc, sizeof(msg_enc));

  if (result == 0) {
    printf("Message successfully written to serial port!\n");
  } else {
    printf("Error sending message. Code: %d\n", result);
  }

  // Free memory - you should destroy the client when you are done with it,
  // and you should free the message and the encoded buffer. Depending on your
  // applications structure, you may want to free the message/buffer on every
  // loop if you are not reusing them.
  free_ppmupdate(&msg);
  free_serialized_buffer(msg_enc, sizeof(msg_enc));
  // Destroy Writer instance
  writer_destroy(writer);

  return 0;
}
