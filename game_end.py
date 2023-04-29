# game_end.py
import serial
from serial import tools
from serial.tools import list_ports

def get_pico_port():
	global pico_port
	picoVID = "2E8A" # Pico USB Vendor ID
	picoPID = "000A" # Pico USB product ID
	port_list = serial.tools.list_ports.comports()

	for device in port_list:
		if (device.vid != None or device.pid != None):
			if ('{:04X}'.format(device.vid) == picoVID and '{:04X}'.format(device.pid) == picoPID):
				pico_port = device.device
			return device.device
	return None

def wait_for_connection():
	# Loop until serial connection is established
	connection = None
	while connection is None:
		connection = get_pico_port()
		if connection is None:
			time.sleep(1)
			print("Pico not connected")

def send_game_closed_to_pico(port):
    ser = serial.Serial(port, 921600)
    ser.write(b"END\n")
    ser.close

# Wait for the Pico connection
wait_for_connection()

# Send the 'END' message to the Pico
send_game_closed_to_pico(pico_port)
print("Game Ended")
