use prost_build;
use std::env;
use std::fs;
use std::io::Result;
use std::path::Path;
use std::process::Command;

fn main() -> Result<()> {
    let proto_files = &["../../firmware/src/protos/RCData.proto"];
    let proto_include = &["../../firmware/src/protos"];

    prost_build::compile_protos(proto_files, proto_include).expect("Failed to compile protos");

    // Generate C bindings using cbindgen
    println!("Generating C bindings...");
    generate_c_bindings()?;

    Ok(())
}

fn generate_c_bindings() -> Result<()> {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let include_path = Path::new(&crate_dir).join("../cyberrc-cpp/include");

    // Ensure the `include/` directory exists
    if !include_path.exists() {
        fs::create_dir_all(&include_path)?;
    }

    let output_path = include_path.join("cyberrc_bindings.h");

    cbindgen::generate(crate_dir)
        .expect("Failed to generate C bindings")
        .write_to_file(output_path);

    Ok(())
}
