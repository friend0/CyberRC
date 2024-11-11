use prost::Message;
use serialport::{SerialPort, SerialPortBuilder, SerialPortType};
use std::io::Write;
use std::time::Duration;

use mockall::{automock, predicate::*};
pub mod constants;

pub mod cyberrc {
    include!(concat!(env!("OUT_DIR"), "/cyberrc.rs"));
}

#[derive(Debug)]
pub enum Controls {
    Aileron,
    Elevator,
    Throttle,
    Rudder,
}

pub struct Writer {
    serial_port: Box<dyn SerialPort>,
    pub baud_rate: u32,
}

impl Writer {
    pub fn new(port: String, baud_rate: u32) -> Result<Self, anyhow::Error> {
        let mut port = serialport::new(port, baud_rate)
            .open()
            .map_err(|e| anyhow::anyhow!(e))?;
        port.set_timeout(Duration::from_secs(1))?;
        Ok(Self {
            serial_port: port,
            baud_rate,
        })
    }

    pub fn write(&mut self, message: cyberrc::RcData) -> Result<(), anyhow::Error> {
        let buffer = message.encode_length_delimited_to_vec();
        self.serial_port
            .write_all(&buffer)
            .map_err(|e| anyhow::anyhow!(e))
    }
}

// Add this to your tests module
#[cfg(test)]
mod tests {
    use serialport::SerialPort;
    use serialport::{ClearBuffer, DataBits, FlowControl, Parity, StopBits};
    use std::io::{self, Read, Write};
    use std::time::Duration;

    pub struct MockSerialPort {
        // Fields to store internal state or expected behavior
        pub read_data: Vec<u8>,
        pub write_data: Vec<u8>,
        pub baud_rate: u32,
        pub timeout: Duration,
    }

    impl MockSerialPort {
        pub fn new() -> Self {
            MockSerialPort {
                read_data: Vec::new(),
                write_data: Vec::new(),
                baud_rate: 9600,
                timeout: Duration::from_secs(1),
            }
        }

        // Add methods to set expectations or control behavior if needed
        pub fn set_read_data(&mut self, data: Vec<u8>) {
            self.read_data = data;
        }

        pub fn get_written_data(&self) -> &[u8] {
            &self.write_data
        }
    }

    impl Read for MockSerialPort {
        fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
            let len = std::cmp::min(buf.len(), self.read_data.len());
            buf[..len].copy_from_slice(&self.read_data[..len]);
            self.read_data.drain(..len);
            Ok(len)
        }
    }

    impl Write for MockSerialPort {
        fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
            self.write_data.extend_from_slice(buf);
            Ok(buf.len())
        }

        fn flush(&mut self) -> io::Result<()> {
            // For testing purposes, you might not need to implement any behavior here
            Ok(())
        }
    }

    impl SerialPort for MockSerialPort {
        fn set_break(&self) -> serialport::Result<()> {
            Ok(())
        }

        fn clear_break(&self) -> serialport::Result<()> {
            Ok(())
        }

        fn name(&self) -> Option<String> {
            Some("MockSerialPort".to_string())
        }

        fn baud_rate(&self) -> serialport::Result<u32> {
            Ok(self.baud_rate)
        }

        fn data_bits(&self) -> serialport::Result<DataBits> {
            Ok(DataBits::Eight)
        }

        fn flow_control(&self) -> serialport::Result<FlowControl> {
            Ok(FlowControl::None)
        }

        fn parity(&self) -> serialport::Result<Parity> {
            Ok(Parity::None)
        }

        fn stop_bits(&self) -> serialport::Result<StopBits> {
            Ok(StopBits::One)
        }

        fn timeout(&self) -> Duration {
            self.timeout
        }

        fn set_baud_rate(&mut self, baud_rate: u32) -> serialport::Result<()> {
            self.baud_rate = baud_rate;
            Ok(())
        }

        fn set_data_bits(&mut self, _data_bits: DataBits) -> serialport::Result<()> {
            Ok(())
        }

        fn set_flow_control(&mut self, _flow_control: FlowControl) -> serialport::Result<()> {
            Ok(())
        }

        fn set_parity(&mut self, _parity: Parity) -> serialport::Result<()> {
            Ok(())
        }

        fn set_stop_bits(&mut self, _stop_bits: StopBits) -> serialport::Result<()> {
            Ok(())
        }

        fn set_timeout(&mut self, timeout: Duration) -> serialport::Result<()> {
            self.timeout = timeout;
            Ok(())
        }

        fn write_request_to_send(&mut self, _level: bool) -> serialport::Result<()> {
            Ok(())
        }

        fn write_data_terminal_ready(&mut self, _level: bool) -> serialport::Result<()> {
            Ok(())
        }

        fn read_clear_to_send(&mut self) -> serialport::Result<bool> {
            Ok(true)
        }

        fn read_data_set_ready(&mut self) -> serialport::Result<bool> {
            Ok(true)
        }

        fn read_ring_indicator(&mut self) -> serialport::Result<bool> {
            Ok(false)
        }

        fn read_carrier_detect(&mut self) -> serialport::Result<bool> {
            Ok(false)
        }

        fn bytes_to_read(&self) -> serialport::Result<u32> {
            Ok(self.read_data.len() as u32)
        }

        fn bytes_to_write(&self) -> serialport::Result<u32> {
            Ok(self.write_data.len() as u32)
        }

        fn clear(&self, _buffer_to_clear: ClearBuffer) -> serialport::Result<()> {
            Ok(())
        }

        fn try_clone(&self) -> serialport::Result<Box<dyn SerialPort>> {
            // For simplicity, return a new instance
            Ok(Box::new(MockSerialPort::new()))
        }
    }

    #[test]
    fn test_new() {
        let port = MockSerialPort::new();
        assert_eq!(port.baud_rate, 9600);
    }

    #[test]
    fn test_write() {
        let port = MockSerialPort::new();
        let mut writer = super::Writer {
            serial_port: Box::new(port),
            baud_rate: 9600,
        };
        let message = super::cyberrc::RcData::default();
        let res = writer.write(message);
        assert!(res.is_ok());
    }
}
