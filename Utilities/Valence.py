#
#  ____                _  _____  ___  __  _______
# / __ \___  ___ ___  | |/_/ _ \/ _ )/  |/  / __/
#/ /_/ / _ \/ -_) _ \_>  </ ___/ _  / /|_/ /\ \  
#\____/ .__/\__/_//_/_/|_/_/  /____/_/  /_/___/  
#    /_/                                         
#
#
# This script requires a USB to RS485 adapter terminated with a Tyco AMP connector.
# It was tested on Windows using an FTDI cable. It will print battery voltage and temperature data for battery 1 to the console.
# This code is released under GPLv3 and comes with no warrany or guarantees. Use at your own risk!

import serial
import libscrc

sPort = 'COM1'
payloadW = [0x00,0x00,0x01,0x01,0xc0,0x74,0x0d,0x0a,0x00,0x00]
payloadV = [0x01,0x03,0x00,0x45,0x00,0x09,0x94,0x19,0x0d,0x0a]
payloadT = [0x01,0x03,0x00,0x50,0x00,0x07,0x04,0x19,0x0d,0x0a]

s = serial.Serial(
    port = sPort,
    baudrate=9600,
    parity=serial.PARITY_MARK,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1
)
s.write(serial.to_bytes(payloadW))
#print("Woke up serial device")
s.flush()
s.close()

s = serial.Serial(
    port = sPort,
    baudrate=115200,
    parity=serial.PARITY_MARK,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1
)
s.write(serial.to_bytes(payloadV))
outputV = s.readline()
s.write(serial.to_bytes(payloadT))
outputT = s.readline()
s.close()

volts=[(outputV[9] * 256) + outputV[10],
    (outputV[11] * 256) + outputV[12],
    (outputV[13] * 256) + outputV[14],
    (outputV[15] * 256) + outputV[16]
]
volts.append(volts[0]+volts[1]+volts[2]+volts[3])

temps=[(outputT[5] * 256) + outputT[6],
    (outputT[7] * 256) + outputT[8],
    (outputT[9] * 256) + outputT[10],
    (outputT[11] * 256) + outputT[12],
    (outputT[3] * 256) + outputT[4]
]
for value in volts: 
    print(value)
for value in temps: 
    print(value)