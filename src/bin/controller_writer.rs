use std::{io, rc, time::Duration};
use std::io::Cursor;
use clap::{Parser};
use prost::Message;
use serialport::{available_ports, SerialPortType, SerialPort, SerialPortBuilder};

pub mod cyberrc {
    include!(concat!(env!("OUT_DIR"), "/cyberrc.rs"));
}

use cyberrc::RcData;

#[derive(Parser, Default, Debug)]
#[command(version, about, long_about=None)]
pub struct Args {
    #[arg(short, long)]
    pub port: Option<String>,
    #[arg(short, long, default_value = "115200")]
    pub baud_rate: u32,
}

#[derive(Debug)]
pub enum Controls {
    Aileron,
    Elevator,
    Throttle,
    Rudder
}

pub fn main() {

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
                break serialport::new(port_name, args.baud_rate)
            }
        }
    }
    .open()
    .unwrap_or_else(|e| {
        eprintln!("{:?}: failed to open port", e);
        std::process::exit(1);
    });

    port.set_timeout(Duration::from_secs(1)).expect("Failed to set timeout");

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
            },
        };
        match choice.trim().parse::<u32>() {
            Ok(choice) if choice > 0 && choice < 6 =>{
            },
            Ok(_) => { 
                eprintln!("Invalid choice. Please select a number from 1-5.");
                continue;
            },
            Err(e) => {
                eprintln!("Failed to read input: {}", e);
                continue;
            },
        };

        println!("Enter a value between -1 and 1:");
        let mut value = String::new();
        match io::stdin().read_line(&mut value) {
            Ok(_) => (),
            Err(e) => {
                eprintln!("Failed to read input: {}", e);
                continue;
            },
        };
        let value: f32 = match value.trim().parse::<f64>() {
            Ok(v) => v as f32,
            Err(_) => {
                eprintln!("Invalid value. Expecting a value between -1 and 1.");
                continue;
            }, 
        };

        // Match the user input to a control
        let _ = match choice.trim() {
            "1" => write_controller(&mut *port, Controls::Aileron, value),
            "2" => write_controller(&mut *port, Controls::Elevator, value),
            "3" => write_controller(&mut *port, Controls::Throttle, value),
            "4" => write_controller(&mut *port, Controls::Rudder, value),
            "5" => {
                println!("Exiting...");
                break;
            },
            _ => {
                eprintln!("Invalid choice. Please select a number from 1-5.");
                continue;
            },
        };
        // todo: handle error
    }
}

pub fn write_controller(writer: &mut dyn SerialPort, channel: Controls, value: f32) -> Result<(), anyhow::Error> {

    let mut rc_data = RcData::default();
    match channel {
        Controls::Aileron => {rc_data.aileron = value;},
        Controls::Elevator => {rc_data.elevator = value;},
        Controls::Throttle => {rc_data.throttle = value;},
        Controls::Rudder => {rc_data.rudder = value;},
    };

    let mut buf= Vec::new();
    rc_data.encode(&mut buf)?;
    writer.write_all(&buf).expect("Failed to write rc_data");
    Ok(())
}