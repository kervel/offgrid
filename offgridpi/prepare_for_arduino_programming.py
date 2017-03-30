from offgridpi import SleepyPi

pi = SleepyPi()

print("enable software bypass jumper")
pi.enableSleepyPiBypass()
print("disable watchdog counter")
pi.disableWatchdog()
