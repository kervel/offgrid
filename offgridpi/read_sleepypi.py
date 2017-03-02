from __future__ import print_function
from smbus import SMBus
import struct

bus = SMBus(1)
address = 0x36

def i2c_get_float32le(bus,address,register):
    b1 = bus.read_byte_data(address,register)
    b2 = bus.read_byte_data(address,register+1)
    b3 = bus.read_byte_data(address,register+2)
    b4 = bus.read_byte_data(address,register+3)
    val = bytearray([b1,b2,b3,b4])
    return struct.unpack('<f',val)[0]


def get_supply_voltage():
    return i2c_get_float32le(bus,address,1)

def get_rpi_current():
    return i2c_get_float32le(bus,address,5)

def detect_sleepy():
    fixed = bus.read_byte_data(address,0)
    if fixed == 58:
        return True
    print("fixed is {f}".format(f=fixed) )
    return False

if detect_sleepy():
    volt = get_supply_voltage()
    amps = get_rpi_current()
    print("i get got {v} volts of supply and am using {i} amps".format(v=volt, i=amps))
