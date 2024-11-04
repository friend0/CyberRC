use binrw::{binrw, BinRead};
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
    motor_num: u8,
    #[br(count = motor_num)]
    motor_rpm: Vec<f32>,
}

const ADDRESS: &str = "0.0.0.0:9001";

#[tokio::main]
async fn main() {
    let shared_data = Arc::new(Mutex::new(None));
    let shared_data_clone = Arc::clone(&shared_data);

    tokio::spawn(async move {
        let _ = feedback_loop(ADDRESS, shared_data_clone).await;
    });

    loop {
        let mut data_lock = shared_data.lock().await;
        while let Some(sample) = data_lock.take() {
            println!("{:?}", sample);
        }
        // Make polling loop configurable
        sleep(Duration::from_secs(1)).await;
    }
}

async fn rerun_plot(data_lock: Arc<Mutex<Option<LiftoffPacket>>>) -> anyhow::Result<()> {
    re_log::setup_logging();
    let session = RecordingStreamBuilder::new("liftoff tracking")
        .connect()
        .unwrap();
    loop {
        let mut data_lock = data_lock.lock().await;
        while let Some(sample) = data_lock.take() {
            println!("{:?}", sample);
            // Liftoff position plotting - sample data is left handed ENU, with Z north, X east, Y up
            let position =
                rerun::Position3D::new(sample.position[2], sample.position[0], sample.position[1]);
            session.log("liftoff tracking", &rerun::Points3D::new(vec![position]))?;
        }

        // Make polling loop configurable
        sleep(Duration::from_secs(1)).await;
    }
}
async fn feedback_loop(
    address: &str,
    data_lock: Arc<Mutex<Option<LiftoffPacket>>>,
) -> anyhow::Result<()> {
    let mut buf = [0; 256];
    let mut current_wait = Duration::from_secs(0);
    let mut delay = Duration::from_secs(2);
    let max_wait = Duration::from_secs(60 * 60 * 15);
    let max_delay = Duration::from_secs(30);

    loop {
        match UdpSocket::bind(address) {
            Ok(socket) => {
                println!("Bound to address: {}", address);
                socket.set_read_timeout(Some(Duration::from_secs(15)))?;
                current_wait = Duration::from_secs(0);
                delay = Duration::from_secs(1);
                loop {
                    match socket.recv_from(&mut buf) {
                        Ok((len, _)) => {
                            let mut cursor = std::io::Cursor::new(&buf[..len]);
                            // TODO: more robust handling of packet parsing errors during resets
                            let sample = LiftoffPacket::read_be(&mut cursor)
                                .expect("Failed to read LiftoffPacket");
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
                                delay
                                    + Duration::from_millis(
                                        (rand::random::<f64>() * 1000.0) as u64,
                                    ),
                            )
                            .await;
                            delay = (delay * 2).min(max_delay);
                            // break;
                        }
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

