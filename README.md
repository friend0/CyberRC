# CyberRC

The goal for this project is to allow autonomous controllers to close the loop with simulated systems such as those running in video games.
CyberRC implements a gamepad emulator, and client code so that controllers can send outputs to games as if there were a "real" controller plugged in.
This implemntation taked advantage of the Teensy 4.1's ability to implement an HID and behave like a gamepad or joystick.

The initial use case is to interface automatic control with the game "Liftoff: Micro Drones" using it's inbuilt functionality to output
state data over UDP.

CyberRC is a sister library to CyberTX.
Where CyberTX enables users to send Protobuf encoded commands (and 'plain' serial encoded data from Matlab) to a physical RC with a trainer input (like the Radiomaster Zorro), 
CyberRC enables users to send Protobuf encoded commands to an emulated gamepad.
This is useful in cases where you would like to take outputs from an autonomous controller and feed them into a video game.

