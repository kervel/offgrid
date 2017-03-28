# What is it 

This is a collection of:

* sleepy pi 2 replacement arduino scripts that allow for setting the sleep/wake timers from the rpi, and for reading the current/voltage from the rpi
* a daemon written in python to be ran on the rpi, that allows to see values and set sleep/wake regime from a MQTT app (eg android MQTT dash), and a free MQTT broker (eg dioty)
* small programs to update retained values on the MQTT broker. this way, we can update the sleep/wake regime remotely.

## features of the arduino part

* pi status tracker that implements a raspberry pi state machine (eg booting, running, halting, ...) without blocking/sleep calls on the arduino so that it interoperates with other software running on the arduino.
* holding the power button on the arduino will initiate a reboot of the rpi (with a 20sec pause between the power off and power on, allowing for safe power cut)
* following I2C registers are available (address 0x36):

  * REG_SIGNATURE = 0 : byte, fixed signature (for easy detection of the arduino), should be 58
  * REG_VOLTAGE = 1 : float32 little endian, supply voltage measured by the sleepy pi
  * REG_CURRENT = 5 : float32 little endian, current measured by the sleepy pi
  * REG_THRESH_SUSPEND_VOLT = 9 : float32 little endian, if the voltage drops below the value, it will suspend
  * REG_THRESH_RESUME_VOLT = 13 : float32 little endian, if voltage goes above voltage, it will get out of suspend
  * REG_ALARM_HOUR = 17 : alarm timer hour (watch out time zone!)
  * REG_ALARM_MINUTE = 19 : alarm timer minute
  * REG_SECONDS = 21 : alarm timeout seconds (either use the alarm, either use the seconds timer)
  * REG_COMMAND = 25 : command register. updating this register initiates an action:


* the following commands are available:

  * CMD_NOTHING=0 : no command
  * CMD_WAIT_ALARM=1 : shutdown the raspberry pi, set RTC alarm and wait for RTC alarm, then restart the raspberry pi 
  * CMD_WAIT_TIMER=2 : same as WAIT_ALARM, but now wait a fixed number of seconds. due to the fact that we only have 16 bits on the RTC, this can be inaccurate!
  * CMD_POWEROFF_EXT=4 : disable external power (untested)
  * CMD_POWERON_EXT=5 : enable external power (untested)

## features of the python daemon

* tries to find network connection. when it has no network connection after 200 secs, it assumes connecting went wrong (eg due to unreliable 3G) and initiates a reboot using the sleepy pi.
* exposes all measurements of the sleepy pi through mqtt retained values
* allows taking a photo by publishing mqtt /[root]/takephoto to 1 (photo will be published to another mqtt topic)
* has a simple sleep/wake scheduler (now 2 behaviours are implemented: always on and cyclic)


# How to get it running:

* follow the official sleepypi 2 instructions to set up the sleepy pi environment on your rpi
* using the arduino IDE (better use the version recommended by the sleepy pi 2 docs, not necessarily the latest version), upload the sketch. If uploading to arduino fails, make sure to check if your getty on ttyS0 is not running anymore.

    check  sudo systemctl stop serial-getty@ttyAMA0.service --> should not be running

* reduce the baudrate of i2c (arduino is not fast enough and raspberry pi does not allow clock stretching): Add the following line to /boot/config.txt:

    dtparam=i2c_baudrate=50000

* create a "run forever" script for the offgridpi_daemon python starting it up with the right credentials

# tweaks increasing reliability


## rpi watchdog





