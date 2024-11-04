use clap::Parser;
use prost::bytes::{buf, Buf};
use prost::Message;
use rerun::external::arrow2::io::ipc::read;
use rerun::external::arrow2::io::print;
use serialport::{available_ports, SerialPort, SerialPortBuilder, SerialPortType};
use std::io::Cursor;
use std::io::{self, BufRead, BufReader, Write};
use std::thread;
use std::{rc, time::Duration};
use tokio::time::sleep;

pub mod cyberrc {
    include!(concat!(env!("OUT_DIR"), "/cyberrc.rs"));
}

use cyberrc::RcData;

#[derive(Parser, Default, Debug)]
#[command(version, about, long_about=None)]
pub struct Args {
    #[arg(short, long)]
    pub port: Option<String>,
    #[arg(short, long, default_value = "921600")]
    pub baud_rate: u32,
}

#[derive(Debug)]
pub enum Controls {
    Aileron,
    Elevator,
    Throttle,
    Rudder,
}

#[tokio::main]
async fn main() {
    println!("CyberRC Controller Writer");
    let args = Args::parse();
    let mut port = match args.port {
        Some(port_name) => serialport::new(port_name, args.baud_rate),
        None => {
            let ports = serialport::available_ports().expect("Failed to list available ports");
            loop {
                println!("\nSelect an available port:");
                for (i, p) in ports.iter().enumerate() {
                    println!("{}: {}", i, p.port_name);
                }
                let mut choice = String::new();
                io::stdin()
                    .read_line(&mut choice)
                    .expect("Failed to read input");
                let value: usize = choice.trim().parse().expect("Please type a number");
                let port_name = ports[value].port_name.clone();
                break serialport::new(port_name, args.baud_rate);
            }
        }
    }
    .open()
    .unwrap_or_else(|e| {
        eprintln!("{:?}: failed to open port", e);
        std::process::exit(1);
    });

    port.flush().expect("Failed to flush port");
    port.set_timeout(Duration::from_millis(1))
        .expect("Failed to set timeout");

    let mut read_port = port.try_clone().expect("Failed to clone port");
    tokio::spawn(async move {
        read_feedback(&mut *read_port).await;
    });
    loop {
        // Display options and parse user input
        println!("\nSelect an aircraft control to modify:");
        println!("1. Aileron");
        println!("2. Elevator");
        println!("3. Throttle");
        println!("4. Rudder");
        println!("5. Quit");

        // Get the user choice
        let mut choice = String::new();
        // io::stdout().flush().unwrap();
        match io::stdin().read_line(&mut choice) {
            Ok(_) => (),
            Err(e) => {
                eprintln!("Failed to read input: {}", e);
                continue;
            }
        };
        match choice.trim().parse::<u32>() {
            Ok(choice) if choice > 0 && choice < 6 => {}
            Ok(_) => {
                eprintln!("Invalid choice. Please select a number from 1-5.");
                continue;
            }
            Err(e) => {
                eprintln!("Failed to read input: {}", e);
                continue;
            }
        };

        println!("Enter a value between -1 and 1:");
        let mut value = String::new();
        match io::stdin().read_line(&mut value) {
            Ok(_) => (),
            Err(e) => {
                eprintln!("Failed to read input: {}", e);
                continue;
            }
        };
        let value: i32 = match value.trim().parse::<i32>() {
            Ok(v) => v as i32,
            Err(_) => {
                eprintln!("Invalid value. Expecting a value between -1 and 1.");
                continue;
            }
        };

        // Match the user input to a control
        let write_result = match choice.trim() {
            "1" => write_controller(&mut *port, Controls::Aileron, value),
            "2" => write_controller(&mut *port, Controls::Elevator, value),
            "3" => write_controller(&mut *port, Controls::Throttle, value),
            "4" => write_controller(&mut *port, Controls::Rudder, value),
            "5" => {
                println!("Exiting...");
                break;
            }
            _ => {
                eprintln!("Invalid choice. Please select a number from 1-5.");
                continue;
            }
        };

        match write_result {
            Ok(_) => {
                println!("Successfully wrote to port");
            }
            Err(e) => {
                eprintln!("Failed to write to port: {}", e);
                continue;
            }
        };
        // todo: handle error
    }
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

pub fn write_controller(
    writer: &mut dyn SerialPort,
    channel: Controls,
    value: i32,
) -> Result<(), anyhow::Error> {
    let mut message = cyberrc::CyberRcMessage::default();
    let mut controller_data = RcData::default();
    match channel {
        Controls::Aileron => {
            controller_data.aileron = value;
        }
        Controls::Elevator => {
            controller_data.elevator = value;
        }
        Controls::Throttle => {
            controller_data.throttle = value;
        }
        Controls::Rudder => {
            controller_data.rudder = value;
        }
    };
    message.r#type = cyberrc::cyber_rc_message::MessageType::RcData as i32;
    message.payload = controller_data.encode_to_vec();
    println!("Type: {}", message.r#type);
    println!("Payload: {:?}", message.payload);

    let buffer = message.encode_length_delimited_to_vec();
    print!("Writing to port: ");
    for byte in &buffer {
        print!("{:02X} ", byte);
    }
    println!();
    writer.write_all(&buffer)?;
    Ok(())
}
