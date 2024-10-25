fn main() {
    prost_build::compile_protos(&["protos/RCData.proto"], &["protos/"]).unwrap();
}