from __future__ import print_function
import argparse
from offgridpi import SleepyPi

parser = argparse.ArgumentParser(description='Process some integers.')

parser.add_argument('--sleep-timer', type=int, help='sleep for N seconds', metavar='N')


args = parser.parse_args()

print(args)