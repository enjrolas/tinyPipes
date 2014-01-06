import serial

ser = serial.Serial('/dev/tty.usbserial-A901N8FN', 115200)
while True:
     print ser.readline()
