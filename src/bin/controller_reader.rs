
fn handle_controller() {
    // Initialize gilrs
    let mut gilrs = Gilrs::new().expect("Failed to initialize gilrs");

    println!("Waiting for gamepad input...");

    // Event loop
    loop {
        while let Some(Event { id, event, time }) = gilrs.next_event() {
            let datetime: DateTime<Local> = time.into();
            println!("Gamepad {} at time {}: {:?}", id, datetime, event);

            // Handle button press events
            match event {
                EventType::ButtonPressed(button, _) => {
                    println!("Button {:?} pressed on gamepad {}", button, id);

                    // Exit loop if the 'South' button is pressed
                    if button == Button::South {
                        println!("'South' button pressed. Exiting.");
                        return;
                    }
                }
                EventType::ButtonChanged(button, value, code) => {
                    println!(
                        "Button {:?} changed on gamepad {} value {}",
                        button, id, value
                    );
                }
                EventType::AxisChanged(axis, value, _) => {
                    let axis = constants::RadioMasterZorro::try_from(axis).unwrap();
                    println!(
                        "Axis {:?} (str {:?}) changed on gamepad {} value {}",
                        axis,
                        constants::RadioMasterZorro::name(&axis),
                        id,
                        value
                    );
                }
                EventType::Connected => {
                    println!("Gamepad {} connected", id);
                }
                EventType::Disconnected => {
                    println!("Gamepad {} disconnected", id);
                }
                _ => {}
            }
        }
    }
}
