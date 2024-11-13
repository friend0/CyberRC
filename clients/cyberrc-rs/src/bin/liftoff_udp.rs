use binrw::{binrw, BinRead};
use nalgebra::{Quaternion, UnitQuaternion, Vector3, Vector4};
use rand;
use std::net::UdpSocket;
use std::sync::Arc;
use tokio::sync::Mutex;
use tokio::time::{sleep, Duration};

// TODO: configure packet based on the content of the Liftoff config file
#[binrw]
#[br(little)]
#[derive(Debug)]
struct LiftoffPacket {
    timestamp: f32,
    position: [f32; 3],
    attitude: [f32; 4],
    input: [f32; 4],
    motor_num: u8,
    #[br(count = motor_num)]
    motor_rpm: Vec<f32>,
}

const ADDRESS: &str = "127.0.0.1:9001";

fn ruf_to_ned(ruf_quat: UnitQuaternion<f32>) -> UnitQuaternion<f32> {
    // Step 1: Flip the handedness by negating the Z component of the RUF quaternion.
    let flipped_quat = Quaternion::new(ruf_quat.w, ruf_quat.i, ruf_quat.j, -ruf_quat.k);

    // Step 2: Define a 90-degree rotation around the Y-axis to align X (Right) to X (North)
    let rotation_y =
        UnitQuaternion::from_axis_angle(&Vector3::y_axis(), std::f32::consts::FRAC_PI_2);

    // Step 3: Define a -90-degree rotation around the X-axis to align Z (Forward) to Z (Down)
    let rotation_x =
        UnitQuaternion::from_axis_angle(&Vector3::x_axis(), -std::f32::consts::FRAC_PI_2);

    // Step 4: Combine the handedness-adjusted quaternion with the rotation transformations
    // Apply the Y rotation first, then the X rotation
    rotation_x * rotation_y * UnitQuaternion::new_normalize(flipped_quat)
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let shared_data: Arc<Mutex<Option<LiftoffPacket>>> = Arc::new(Mutex::new(None));
    let shared_data_clone = Arc::clone(&shared_data);

    let rec = rerun::RecordingStreamBuilder::new("Liftoff UDP").spawn()?;
    rerun::Logger::new(rec.clone())
        .with_path_prefix("logs")
        .with_filter(rerun::default_log_filter())
        .init()
        .unwrap();

    tokio::spawn(async move {
        let _ = feedback_loop(ADDRESS, shared_data_clone).await;
    });

    loop {
        let mut data_lock = shared_data.lock().await;
        while let Some(sample) = data_lock.take() {
            log_data(
                &rec,
                sample.timestamp,
                Vector3::new(sample.position[0], sample.position[1], sample.position[2]),
                nalgebra::Quaternion::new(
                    sample.attitude[3],
                    sample.attitude[0],
                    sample.attitude[1],
                    sample.attitude[2],
                ),
                sample.input,
            )?;
            println!(
                "Time: {:.2}, Position: {:?}, Attitude: {:?}, Input: {:?}",
                sample.timestamp, sample.position, sample.attitude, sample.input
            );
        }
        // Make polling loop configurable
        sleep(Duration::from_millis(10)).await;
    }
}

pub fn unity_to_rerun_translation(unity_position: Vector3<f32>) -> Vector3<f32> {
    Vector3::new(unity_position[0], unity_position[2], -unity_position[1])
}

pub fn log_data(
    rec: &rerun::RecordingStream,
    time: f32,
    position: Vector3<f32>,
    orientation: Quaternion<f32>,
    input: [f32; 4],
) -> anyhow::Result<()> {
    rec.set_time_sequence("step", (time * 1000.0) as i64);

    rec.log("input/throttle", &rerun::Scalar::new(input[0] as f64))?;
    rec.log("input/yaw", &rerun::Scalar::new(input[1] as f64))?;
    rec.log("input/pitch", &rerun::Scalar::new(input[2] as f64))?;
    rec.log("input/roll", &rerun::Scalar::new(input[3] as f64))?;

    let quat = nalgebra::UnitQuaternion::from_quaternion(orientation);
    let quat = ruf_to_ned(quat);

    let position = unity_to_rerun_translation(position);
    let quad_position = rerun::Transform3D::from_translation_rotation(
        rerun::Vec3D::new(0.0, 0.0, 0.0),
        rerun::Quaternion::from_wxyz([quat.w, quat.i, quat.j, quat.k]),
    );

    rec.log("world/quad", &quad_position.with_axis_length(1.0))?;
    let (quad_roll, quad_pitch, quad_yaw) = quat.euler_angles();
    let quad_euler_angles: Vector3<f32> = Vector3::new(quad_roll, quad_pitch, quad_yaw);
    for (pre, vec) in [("position", position), ("orientation", quad_euler_angles)] {
        for (i, a) in ["x", "y", "z"].iter().enumerate() {
            rec.log(format!("{}/{}", pre, a), &rerun::Scalar::new(vec[i] as f64))?;
        }
    }
    Ok(())
}

async fn feedback_loop(
    address: &str,
    data_lock: Arc<Mutex<Option<LiftoffPacket>>>,
) -> anyhow::Result<()> {
    let mut current_wait = Duration::from_secs(0);
    let mut delay = Duration::from_secs(2);
    let max_wait = Duration::from_secs(60 * 60 * 15);
    let max_delay = Duration::from_secs(30);

    loop {
        let mut buf = [0; 128];
        match UdpSocket::bind(address) {
            Ok(socket) => {
                // println!("Bound to address: {}", address);
                socket.set_read_timeout(Some(Duration::from_secs(15)))?;
                current_wait = Duration::from_secs(0);
                delay = Duration::from_secs(1);
                match socket.recv_from(&mut buf) {
                    Ok((len, _)) => {
                        let mut cursor = std::io::Cursor::new(&buf[..len]);
                        // println!("Buffer length: {:?}", buf);
                        // TODO: more robust handling of packet parsing errors during resets
                        let sample =
                            LiftoffPacket::read(&mut cursor).expect("Failed to read LiftoffPacket");
                        // println!("Received data: {:?}", sample);
                        let mut data_lock = data_lock.lock().await;
                        *data_lock = Some(sample);
                    }
                    Err(e) => {
                        println!("Failed to receive data: {}", e);
                        if current_wait >= max_wait {
                            println!("Failed to receive data on bound address: {}", e);
                            return Err(anyhow::anyhow!("Bind loop exceeded max wait time"));
                        }
                        current_wait += delay;
                        sleep(
                            delay + Duration::from_millis((rand::random::<f64>() * 1000.0) as u64),
                        )
                        .await;
                        delay = (delay * 2).min(max_delay);
                        // break;
                    }
                }
            }
            Err(e) => {
                return Err(anyhow::anyhow!("Bind loop exceeded max wait time"));
            }
        }
    }
    Ok(())
}
