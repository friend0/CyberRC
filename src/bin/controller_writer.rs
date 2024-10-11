use clap::{App, Arg};
use prost::Message;
use serialport::prelude::*;

pub mod cyberrc {
    include!(concat!(env!("OUT_DIR"), "/cyberrc.rs"));
}

use cyberrc::RcData;

#derive(Parser, Default, Debug)]
#[command(name = "controller_writer")]
pub struct Args {
    #[clap(short, long, default_value = "/dev/ttyACM0")]
    pub port: &str,
    #[clap(short, long, default_value = "115200")]
    pub baud_rate: u32,
}

#[derive(Subcommand, Debug)]
enum Controls {
    Aileron,
    Elevator,
    Throttle,
    Rudder,
}

pub fn main() {

    let args = Args::parse();

    let mut settings: SerialPortSettings = Default::default();
    settings.baud_rate = args.baud_rate;
    settings.timeout = std::time::Duration::from_secs(1);
    let port = serialport::open_with_settings(args.port, &settings).expect("Failed to open port");
    let mut writer = port.try_clone().expect("Failed to clone port");
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
        io::stdout().flush().unwrap();
        io::stdin()
            .read_line(&mut choice)
            .expect("Failed to read input");

        let mut value = String::new();
        io::stdin().read_line(&mut value).expect("Failed to read input");
        let value: f32 = value.trim().parse().expect("Please type a floating point number");

        // Match the user input to a control
        match choice.trim() {
            "1" => handle_input(&port, Controls::Aileron, value),
            "2" => handle_input(&port, Controls::Elevator, value)
            "3" => handle_input(&port, Controls::Throttle, value),
            "4" => handle_input(&port, Controls::Rudder, value),
            "5" => {
                println!("Exiting...");
                break;
            }
            _ => println!("Invalid choice, please try again."),
        }

    }
}

pub fn write_controller(writer: serialport, channel: Controls, value: f32)-> RcData {
    println!("Controller: a: {}, e: {}, t: {}, r: {}, arm: {}, mode: {}", a, e, t, r, arm, mode);
    rc_data.encode(&mut buf).expect("Failed to encode rc_data");
    writer.write_all(&buf).expect("Failed to write rc_data");
}
