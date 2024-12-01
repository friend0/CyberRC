use prost_build;
use std::io::Result;

fn main() -> Result<()> {
    let proto_files = &["../../firmware/src/protos/RCData.proto"];
    let proto_include = &["../../firmware/src/protos"];

    prost_build::compile_protos(proto_files, proto_include).expect("Failed to compile protos");
    Ok(())
}
