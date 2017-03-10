from __future__ import print_function
import argparse
from offgridpi import SleepyPi
import sys

parser = argparse.ArgumentParser(description='Process some integers.')

parser.add_argument('--sleep-timer', type=int, help='sleep for N seconds', metavar='N')
parser.add_argument('--sleep-alarm', type=str, help='sleep until timestamp X (format: HH:MM)', metavar='X')
parser.add_argument('--stats',action='store_true',help='report power consumption and power supply voltage')
parser.add_argument('--set-suspend-voltage',type=float,help='set minimum voltage to V (if the voltage drops lower, system will shut down). First set the resume voltage. Setting to 0 will disable voltage checks',metavar='V')
parser.add_argument('--set-resume-voltage',type=float,help='when voltage recovers to V, start the system back up. Setting to 0 will disable voltage checks', metavar='V')
args = parser.parse_args()

pi = SleepyPi()

if args.stats == True:
    print("I: {amps}".format(amps=pi.get_rpi_current()))
    print("V: {volts}".format(volts=pi.get_supply_voltage()))
    print('Susp: {v}'.format(v=pi.get_minimum_run_voltage()))
    print('Res: {v}'.format(v=pi.get_resume_voltage()))
    sys.exit(0)

if args.set_suspend_voltage:
    pi.set_minimum_run_voltage(args.set_suspend_voltage)
    sys.exit(0)

if args.set_resume_voltage:
    pi.set_resume_voltage(args.set_resume_voltage)
    sys.exit(0)

if args.sleep_timer:
    pi.sleepTimer(args.sleep_timer)
    print("pi should now get a shutdown command from arduino")
    sys.exit(0)

if args.sleep_alarm:
    s = args.sleep_alarm
    if len(s.split(':')) != 2:
        raise Exception("use HH:MM")
    h = s.split(':')[0]
    m = s.split(':')[1]
    if not h.isdigit() or int(h) > 23 or int(h) < 0:
        raise Exception('invalid hour %s' % h)
    if not m.isdigit() or int(m) > 59 or int(m) < 0:
        raise Exception('invalid minute %s' % m)
    pi.sleepAlarm(int(h),int(m))
    print("pi should now get a shutdown command from arduino")
    sys.exit(0)

print(args)