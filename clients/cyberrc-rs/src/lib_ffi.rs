use crate::Writer;
use crate::{CyberRCMessageType, CyberRcMessage};
use prost::Message;
use std::ffi::{CStr, CString};
use std::io::Write;
use std::os::raw::{c_char, c_uint};
use std::ptr;

use crate::cyberrc::{PpmUpdateAll, RcData};

/// FFI-safe enum for CyberRC message types
#[repr(C)]
pub enum CyberRCMessageTypeC {
    RcData = 0,
    PpmUpdate = 1,
}

/// Create a new Writer instance
#[no_mangle]
pub extern "C" fn writer_new(port: *const c_char, baud_rate: c_uint) -> *mut Writer {
    let c_str = unsafe { CStr::from_ptr(port) };
    let port_string = match c_str.to_str() {
        Ok(s) => s.to_string(),
        Err(_) => return std::ptr::null_mut(),
    };

    match serialport::new(port_string, baud_rate)
        .open()
        .map(|port| Writer {
            serial_port: port,
            baud_rate,
        }) {
        Ok(writer) => Box::into_raw(Box::new(writer)),
        Err(_) => std::ptr::null_mut(),
    }
}

/// Write a message to the serial port
/// Write a message to the serial port.
/// The data buffer should contain a serialized protobuf message (without length prefix).
/// The message_type flag tells the function which message to decode:
///   0 = RcData, 1 = PpmUpdate.
/// Returns 0 on success or a nonzero error code:
///   1: Null pointer argument, 2: Decoding error, 3: Write error.
#[no_mangle]
pub extern "C" fn writer_write(
    writer: *mut Writer,
    message_type: CyberRCMessageTypeC,
    data: *const u8,
    data_len: c_uint,
) -> c_uint {
    if writer.is_null() || data.is_null() {
        return 1; // Error: Null pointer provided.
    }
    let writer_ref = unsafe { &mut *writer };
    let data_slice = unsafe { std::slice::from_raw_parts(data, data_len as usize) };

    // Depending on the message_type flag, decode the data into the proper message.
    let message = match message_type {
        CyberRCMessageTypeC::RcData => {
            // Attempt to decode an RcData message.
            match RcData::decode(data_slice) {
                Ok(decoded) => CyberRCMessageType::RcData(decoded),
                Err(_) => return 2, // Error: Decoding RcData failed.
            }
        }
        CyberRCMessageTypeC::PpmUpdate => {
            // Attempt to decode a PpmUpdateAll message.
            match PpmUpdateAll::decode(data_slice) {
                Ok(decoded) => CyberRCMessageType::PpmUpdate(decoded),
                Err(_) => return 2, // Error: Decoding PpmUpdateAll failed.
            }
        }
    };

    // Build the wrapper message.
    let mut wrapper = crate::CyberRcMessage {
        channel_values_count: match &message {
            CyberRCMessageType::RcData(_) => 0,
            CyberRCMessageType::PpmUpdate(ref ppm) => ppm.channel_values.len() as i32,
        },
        r#type: match message {
            CyberRCMessageType::RcData(_) => {
                crate::cyberrc::cyber_rc_message::MessageType::RcData as i32
            }
            CyberRCMessageType::PpmUpdate(_) => {
                crate::cyberrc::cyber_rc_message::MessageType::PpmUpdate as i32
            }
        },
        payload: match message {
            CyberRCMessageType::RcData(mut data) => {
                // Set the arm field to 32767 as in the original implementation.
                data.arm = 32767;
                data.encode_to_vec()
            }
            CyberRCMessageType::PpmUpdate(ref data) => data.encode_to_vec(),
        },
    };

    // Use encode_length_delimited_to_vec() to serialize the wrapper.
    let buffer = wrapper.encode_length_delimited_to_vec();

    // Write the length-delimited message to the serial port.
    match writer_ref.serial_port.write_all(&buffer) {
        Ok(_) => 0,  // Success.
        Err(_) => 3, // Error writing to the serial port.
    }
}

/// Destroy a Writer instance
#[no_mangle]
pub extern "C" fn writer_destroy(writer: *mut Writer) {
    if !writer.is_null() {
        unsafe { drop(Box::from_raw(writer)) };
    }
}

/// C‑safe wrapper for RcData.
/// All fields are plain integers.
#[repr(C)]
pub struct RcDataWrapper {
    pub aileron: i32,
    pub elevator: i32,
    pub throttle: i32,
    pub rudder: i32,
    pub arm: i32,
    pub mode: i32,
}

/// Convert an RcDataWrapper into an RcData (prost‑generated).
impl From<RcDataWrapper> for RcData {
    fn from(wrapper: RcDataWrapper) -> Self {
        RcData {
            aileron: wrapper.aileron,
            elevator: wrapper.elevator,
            throttle: wrapper.throttle,
            rudder: wrapper.rudder,
            arm: wrapper.arm,
            mode: wrapper.mode,
        }
    }
}

/// Encode an RcData message into a length-delimited byte buffer.
/// - `wrapper`: The FFI‑safe RcData message provided by the caller.
/// - `out_len`: Pointer to a usize that will receive the encoded buffer’s length.
/// Returns a pointer to a heap‑allocated buffer containing the encoded bytes.
/// The caller must free the returned buffer by calling free_encoded_buffer.
#[no_mangle]
pub extern "C" fn rcdata_encode(wrapper: RcDataWrapper, out_len: *mut usize) -> *mut u8 {
    let rcdata: RcData = wrapper.into();
    // Use prost’s length-delimited encoding.
    let encoded = rcdata.encode_length_delimited_to_vec();
    let len = encoded.len();
    unsafe {
        if !out_len.is_null() {
            *out_len = len;
        }
    }
    // Leak the Vec by converting it into a boxed slice, then into a raw pointer.
    Box::into_raw(encoded.into_boxed_slice()) as *mut u8
}

/// Free an RcData message previously allocated by rcdata_new.
#[no_mangle]
pub extern "C" fn free_rcdata(msg_ptr: *mut RcDataWrapper) {
    if msg_ptr.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(msg_ptr));
    }
}

/// C‑safe wrapper for PpmUpdateAll.
/// Since C cannot express a Rust Vec, we represent the repeated field as a pointer plus count.
#[repr(C)]
pub struct PpmUpdateAllWrapper {
    pub line: i32,
    pub channel_values: *const i32, // pointer to an array of int32_t values
    pub channel_values_count: usize, // number of elements in the array
}

/// Convert a PpmUpdateAllWrapper into a PpmUpdateAll.
/// This conversion copies the channel values from the C array.
impl From<PpmUpdateAllWrapper> for PpmUpdateAll {
    fn from(wrapper: PpmUpdateAllWrapper) -> Self {
        let values = unsafe {
            // Safety: Caller must guarantee that channel_values points to an array
            // with at least channel_values_count valid int32_t values.
            std::slice::from_raw_parts(wrapper.channel_values, wrapper.channel_values_count)
        };
        PpmUpdateAll {
            line: wrapper.line,
            channel_values: values.to_vec(),
        }
    }
}

/// Encode a PpmUpdateAll message into a length-delimited byte buffer.
/// - `wrapper`: The FFI‑safe PpmUpdateAll message.
/// - `out_len`: Pointer to a usize to receive the encoded length.
/// Returns a pointer to a heap‑allocated buffer with the encoded bytes.
/// The caller is responsible for freeing the buffer.
#[no_mangle]
pub extern "C" fn ppmupdate_encode(wrapper: PpmUpdateAllWrapper, out_len: *mut usize) -> *mut u8 {
    let ppm: PpmUpdateAll = wrapper.into();
    let encoded = ppm.encode_length_delimited_to_vec();
    let len = encoded.len();
    unsafe {
        if !out_len.is_null() {
            *out_len = len;
        }
    }
    Box::into_raw(encoded.into_boxed_slice()) as *mut u8
}

/// Free a PpmUpdateAll message previously allocated by ppmupdate_new.
#[no_mangle]
pub extern "C" fn free_ppmupdate(msg_ptr: *mut PpmUpdateAllWrapper) {
    if msg_ptr.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(msg_ptr));
    }
}

/// Free a serialized byte buffer allocated by rcdata_encode or ppmupdate_encode.
/// 'len' must be the same length returned in the out_len parameter.
#[no_mangle]
pub extern "C" fn free_serialized_buffer(buf: *mut u8, len: usize) {
    if buf.is_null() {
        return;
    }
    unsafe {
        // Reconstruct the boxed slice and drop it.
        let _ = Box::from_raw(std::slice::from_raw_parts_mut(buf, len));
    }
}
