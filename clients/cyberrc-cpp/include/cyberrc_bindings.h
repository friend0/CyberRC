#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>


/// FFI-safe enum for CyberRC message types
enum class CyberRCMessageTypeC {
  RcData = 0,
  PpmUpdate = 1,
};

struct CyberRCMessageType;

struct Writer;

/// C‑safe wrapper for PpmUpdateAll.
/// Since C cannot express a Rust Vec, we represent the repeated field as a pointer plus count.
struct PpmUpdateAllWrapper {
  int32_t line;
  const int32_t *channel_values;
  size_t channel_values_count;
};

/// C‑safe wrapper for RcData.
/// All fields are plain integers.
struct RcDataWrapper {
  int32_t aileron;
  int32_t elevator;
  int32_t throttle;
  int32_t rudder;
  int32_t arm;
  int32_t mode;
};


extern "C" {

/// Free a PpmUpdateAll message previously allocated by ppmupdate_new.
void free_ppmupdate(PpmUpdateAllWrapper *msg_ptr);

/// Free an RcData message previously allocated by rcdata_new.
void free_rcdata(RcDataWrapper *msg_ptr);

/// Free a serialized byte buffer allocated by rcdata_encode or ppmupdate_encode.
/// 'len' must be the same length returned in the out_len parameter.
void free_serialized_buffer(uint8_t *buf, size_t len);

/// Encode a PpmUpdateAll message into a length-delimited byte buffer.
/// - `wrapper`: The FFI‑safe PpmUpdateAll message.
/// - `out_len`: Pointer to a usize to receive the encoded length.
/// Returns a pointer to a heap‑allocated buffer with the encoded bytes.
/// The caller is responsible for freeing the buffer.
uint8_t *ppmupdate_encode(PpmUpdateAllWrapper wrapper, size_t *out_len);

/// Encode an RcData message into a length-delimited byte buffer.
/// - `wrapper`: The FFI‑safe RcData message provided by the caller.
/// - `out_len`: Pointer to a usize that will receive the encoded buffer’s length.
/// Returns a pointer to a heap‑allocated buffer containing the encoded bytes.
/// The caller must free the returned buffer by calling free_encoded_buffer.
uint8_t *rcdata_encode(RcDataWrapper wrapper, size_t *out_len);

/// Destroy a Writer instance
void writer_destroy(Writer *writer);

/// Create a new Writer instance
Writer *writer_new(const char *port, unsigned int baud_rate);

/// Write a message to the serial port
/// Write a message to the serial port.
/// The data buffer should contain a serialized protobuf message (without length prefix).
/// The message_type flag tells the function which message to decode:
///   0 = RcData, 1 = PpmUpdate.
/// Returns 0 on success or a nonzero error code:
///   1: Null pointer argument, 2: Decoding error, 3: Write error.
unsigned int writer_write(Writer *writer,
                          CyberRCMessageTypeC message_type,
                          const uint8_t *data,
                          unsigned int data_len);

}  // extern "C"
