use prost_build;
use std::io::Result;

fn main() -> Result<()> {
    let proto_files = &["../../protos/RCData.proto"];
    let proto_include = &["../../protos"];

    prost_build::compile_protos(proto_files, proto_include).expect("Failed to compile protos");
    Ok(())
}
