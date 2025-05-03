use prost::Message;
use serialport::SerialPort;
use std::f32::consts::PI;
use std::io::{Read, Write};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::{Duration, Instant};
use std::io;

pub mod cyberrc {
    include!(concat!(env!("OUT_DIR"), "/cyberrc.rs"));
}

use cyberrc::RcData;

#[derive(Debug)]
pub enum Controls {
    Aileron,
    Elevator,
    Throttle,
    Rudder,
}

fn write_controller(
    writer: &mut dyn SerialPort,
    channel: Controls,
    value: i32,
) -> Result<(), anyhow::Error> {
    let mut message = cyberrc::CyberRcMessage::default();
    let mut controller_data = RcData::default();
    match channel {
        Controls::Aileron => controller_data.aileron = value,
        Controls::Elevator => controller_data.elevator = value,
        Controls::Throttle => controller_data.throttle = value,
        Controls::Rudder => controller_data.rudder = value,
    };
    controller_data.arm = 1;
    message.r#type = cyberrc::cyber_rc_message::MessageType::RcData as i32;
    message.payload = controller_data.encode_to_vec();

    let buffer = message.encode_length_delimited_to_vec();
    writer.write_all(&buffer)?;
    Ok(())
}

async fn read_feedback(port: &mut dyn SerialPort) {
    let mut buffer = vec![0; 1024];
    port.set_timeout(Duration::from_millis(1))
        .expect("Failed to set timeout");
    loop {
        match port.read(buffer.as_mut_slice()) {
            Ok(bytes_read) => {
                io::stdout()
                    .write_all(&buffer[..bytes_read])
                    .expect("Failed to write to stdout");
                // io::stdout().flush().expect("Failed to flush stdout");
            }
            Err(ref e) if e.kind() == io::ErrorKind::TimedOut => {
                // Handle timeout if desired; currently, it continues to read
                // eprintln!("Timeout reading from serial port");
                // io::stdout().flush().expect("Failed to flush stdout");
                continue;
            }
            Err(e) => {
                // eprintln!("Error reading from serial port: {}", e);
                // io::stdout().flush().expect("Failed to flush stdout");
                continue;
            }
        }
        tokio::time::sleep(Duration::from_millis(5)).await;
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("Starting controller throttle sinusoidal test...");
    
    let port_name = "/dev/tty.usbserial-B001JE6N"; // Change as needed
    let baud_rate = 460800;
    let mut port = serialport::new(port_name, baud_rate)
        .timeout(Duration::from_millis(10))
        .open()?;
    println!("PORT...");

    // let shared_port = Arc::new(Mutex::new(port));
    // let writer_port = Arc::clone(&shared_port);
    let mut read_port = port.try_clone().expect("Failed to clone port");
    tokio::spawn(async move {
        read_feedback(&mut *read_port).await;
    });

    println!("Sending sinusoidal throttle commands on {} at {} baud", port_name, baud_rate);

    let start_time = Instant::now();
    loop {
        let elapsed = start_time.elapsed().as_secs_f32();
        let value = (elapsed * 2.0 * PI).sin();
        let scaled = (value * 32768.0) as i32;
        // let mut port = writer_port.lock().unwrap();
        write_controller(&mut *port, Controls::Rudder, scaled)?;
        // drop(port);
        thread::sleep(Duration::from_millis(25));
    }
}
