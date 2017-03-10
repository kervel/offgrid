from __future__ import print_function
try:
    from smbus import SMBus
except:
    from smbus2 import SMBus

import struct

REG_SIGNATURE = 0
REG_VOLTAGE = 1
REG_CURRENT = 5
REG_THRESH_SUSPEND_VOLT = 9
REG_THRESH_RESUME_VOLT = 13
REG_ALARM_HOUR = 17
REG_ALARM_MINUTE = 19
REG_SECONDS = 21
REG_COMMAND = 25

CMD_NOTHING=0
CMD_WAIT_ALARM=1
CMD_WAIT_TIMER=2
CMD_POWEROFF_EXT=4
CMD_POWERON_EXT=5


class SleepyPi():
    def __init__(self):
        self.bus = SMBus(1)
        self.address = 0x36
        if not self.detect_sleepy():
            raise Exception("sleepy pi not detected")

    def i2c_get_float32le(self,register):
        b1 = self.bus.read_byte_data(self.address,register)
        b2 = self.bus.read_byte_data(self.address,register+1)
        b3 = self.bus.read_byte_data(self.address,register+2)
        b4 = self.bus.read_byte_data(self.address,register+3)
        val = bytearray([b1,b2,b3,b4])
        return struct.unpack('<f',val)[0]

    def get_supply_voltage(self):
        return self.i2c_get_float32le(REG_VOLTAGE)

    def get_rpi_current(self):
        return self.i2c_get_float32le(REG_CURRENT)

    def detect_sleepy(self):
        fixed = self.bus.read_byte_data(self.address,REG_SIGNATURE)
        if fixed == 58:
            return True
        print("fixed is {f}".format(f=fixed) )
        return False

    def get_minimum_run_voltage(self):
        return self.i2c_get_float32le(REG_THRESH_SUSPEND_VOLT)

    def get_resume_voltage(self):
        return self.i2c_get_float32le(REG_THRESH_RESUME_VOLT)

    def set_minimum_run_voltage(self,val):
        bytearray = struct.pack('<f',val)
        self.bus.write_i2c_block_dat(self.address, REG_THRESH_SUSPEND_VOLT, bytearray)

    def set_resume_voltage(self,val):
        bytearray = struct.pack('<f',val)
        self.bus.write_i2c_block_data(self.address, REG_THRESH_RESUME_VOLT, bytearray)

    def set_alarm_hour_reg(self,val):
        bytearray = struct.pack('<H',val)
        self.bus.write_i2c_block_data(self.address,REG_ALARM_HOUR, bytearray)

    def set_alarm_minutes_reg(self,val):
        bytearray = struct.pack('<H',val)
        self.bus.write_i2c_block_data(self.address,REG_ALARM_MINUTE, bytearray)

    def set_sleep_timer_reg(self,seconds):
        bytearray = struct.pack('<I',seconds)
        self.bus.write_i2c_block_data(self.address,REG_SECONDS, bytearray)

    def sendCommand(self,cmd):
        self.bus.write_byte_data(self,REG_COMMAND,cmd)

    def sleepTimer(self,seconds):
        self.set_sleep_timer_reg(seconds)
        self.sendCommand(CMD_WAIT_TIMER)

    def sleepAlarm(self,hour,minute):
        self.set_alarm_hour_reg(hour)
        self.set_alarm_minutes_reg(minute)
        self.sendCommand(CMD_WAIT_ALARM)

    def enableExtPower(self):
        self.sendCommand(CMD_POWERON_EXT)

    def disableExtPower(self):
        self.sendCommand(CMD_POWEROFF_EXT)