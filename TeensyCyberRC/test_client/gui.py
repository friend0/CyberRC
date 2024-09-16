import tkinter as tk
from tkinter import ttk
import serial
import RCData_pb2  # Generated from the .proto file
import threading

# Configure your serial port here
SERIAL_PORT = 'COM3'  # Replace with your serial port (e.g., '/dev/ttyUSB0' on Linux)
BAUD_RATE = 9600

# Initialize serial connection
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

def send_protobuf_message():
    message = RCData_pb2.RCData()
    message.Aileron = slider1.get()
    message.Elevator = slider2.get()
    message.Throttle = slider3.get()
    message.Rudder = slider4.get()
    message.Arm = button1_state.get()
    message.Mode = button2_state.get()

    # Serialize the message to a byte string
    serialized_message = message.SerializeToString()

    # Send the length of the message first (optional but recommended)
    message_length = len(serialized_message).to_bytes(4, byteorder='little')
    ser.write(message_length + serialized_message)

def on_slider_change(event):
    send_protobuf_message()

def on_button_click():
    send_protobuf_message()

# GUI Setup
root = tk.Tk()
root.title("Control Panel")

mainframe = ttk.Frame(root, padding="10")
mainframe.grid(column=0, row=0, sticky=(tk.N, tk.W, tk.E, tk.S))

# Slider 1
ttk.Label(mainframe, text="Aileron").grid(column=1, row=1, sticky=tk.W)
slider1 = tk.Scale(mainframe, from_=0, to=100, orient=tk.HORIZONTAL, command=on_slider_change)
slider1.grid(column=2, row=1, sticky=(tk.W, tk.E))

# Slider 2
ttk.Label(mainframe, text="Elevator").grid(column=1, row=2, sticky=tk.W)
slider2 = tk.Scale(mainframe, from_=0, to=100, orient=tk.HORIZONTAL, command=on_slider_change)
slider2.grid(column=2, row=2, sticky=(tk.W, tk.E))

# Slider 3
ttk.Label(mainframe, text="Throttle").grid(column=1, row=3, sticky=tk.W)
slider3 = tk.Scale(mainframe, from_=0, to=100, orient=tk.HORIZONTAL, command=on_slider_change)
slider3.grid(column=2, row=3, sticky=(tk.W, tk.E))

# Slider 4
ttk.Label(mainframe, text="Rudder").grid(column=1, row=4, sticky=tk.W)
slider4 = tk.Scale(mainframe, from_=0, to=100, orient=tk.HORIZONTAL, command=on_slider_change)
slider4.grid(column=2, row=4, sticky=(tk.W, tk.E))

# Button States
button1_state = tk.BooleanVar()
button2_state = tk.BooleanVar()

# Button 1
button1 = ttk.Checkbutton(mainframe, text="Arm", variable=button1_state, command=on_button_click)
button1.grid(column=1, row=5, sticky=tk.W)

# Button 2
button2 = ttk.Checkbutton(mainframe, text="Mode", variable=button2_state, command=on_button_click)
button2.grid(column=2, row=5, sticky=tk.W)

# Padding for all widgets
for child in mainframe.winfo_children():
    child.grid_configure(padx=5, pady=5)

# Threading to keep GUI responsive
def read_serial():
    while True:
        if ser.in_waiting:
            data = ser.read(ser.in_waiting)
            print("Received:", data)

threading.Thread(target=read_serial, daemon=True).start()

root.mainloop()

# Close the serial connection when the GUI is closed
ser.close()
