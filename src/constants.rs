#[repr(u16)]
#[derive(Debug)]
pub enum RadioMasterZorro {
    Aileron,
    Elevator,
    Throttle,
    Rudder,
    Arm,
    Mode,
}

impl RadioMasterZorro {
    pub fn name(&self) -> &str {
        match self {
            RadioMasterZorro::Aileron => "Aileron",
            RadioMasterZorro::Elevator => "Elevator",
            RadioMasterZorro::Throttle => "Throttle",
            RadioMasterZorro::Rudder => "Rudder",
            RadioMasterZorro::Arm => "Arm",
            RadioMasterZorro::Mode => "Mode",
        }
    }
}

impl TryFrom<gilrs::Axis> for RadioMasterZorro {
    type Error = ();
    fn try_from(value: gilrs::Axis) -> Result<Self, Self::Error> {
        match value {
            gilrs::Axis::LeftStickX => Ok(RadioMasterZorro::Aileron),
            gilrs::Axis::LeftStickY => Ok(RadioMasterZorro::Elevator),
            gilrs::Axis::LeftZ => Ok(RadioMasterZorro::Throttle),
            gilrs::Axis::RightStickX => Ok(RadioMasterZorro::Rudder),
            gilrs::Axis::RightStickY => Ok(RadioMasterZorro::Arm),
            gilrs::Axis::RightZ => Ok(RadioMasterZorro::Mode),
            _ => Err(()),
        }
    }
}
