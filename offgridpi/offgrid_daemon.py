import paho.mqtt.client as mqtt
import netifaces
from time import sleep
from cv2 import *
import cv2
import offgridpi
import argparse
import datetime
import os
from offgridpi import wake_sleep_sheduler
from collections import namedtuple
from subprocess import Popen, PIPE, STDOUT


state = {
    'connected' : 0
}


def shell_command(client,userdata,message):
    cmd = message.payload
    r = Popen(cmd, shell=True,stdout=PIPE,stderr=STDOUT)
    output = r.communicate(input='')[0]
    rtopic = '/shell/out'
    client.publish(state['rootkey'] + rtopic, output.decode())




def disk_usage(path):
    """Return disk usage associated with path."""
    st = os.statvfs(path)
    free = (st.f_bavail * st.f_frsize)
    total = (st.f_blocks * st.f_frsize)
    used = (st.f_blocks - st.f_bfree) * st.f_frsize
    try:
        percent = ret = (float(used) / total) * 100
    except ZeroDivisionError:
        percent = 0
    # NB: the percentage is -5% than what shown by df due to
    # reserved blocks that we are currently not considering:
    # http://goo.gl/sWGbH
    return round(percent,1)


def build_ifaces_topics():
    res_dct = {}
    interfaces = [x for x in netifaces.interfaces() if not x == 'lo']
    for i in interfaces:
        addrs = netifaces.ifaddresses(i)
        for family in addrs.keys():
            if family in (2,10):
                ix = 0
                for adr in addrs[family]:
                    res_dct['/addr/' + i + '/' + str(family) + '/' + str(ix)] = adr['addr']
                    ix = ix + 1
    return res_dct


def get_changes(dold, dnew):
    changes = {}
    for k in dnew.keys():
        if not k in dold:
            changes[k]=dnew[k]
        elif dnew[k] != dold[k]:
            changes[k] = dnew[k]
        else:
            pass
    return changes


def set_sleep_register(client,userdata,message):
    payload = str(message.payload)
    try:
        seconds = int(payload)
        pi.set_sleep_timer_reg(seconds)
    except:
        print("error sleeping for %s seconds" % payload)

def activate_sleep(client,userdata,message):
    payload = str(message.payload)
    if payload == '1':
        pi.sendCommand(offgridpi.CMD_WAIT_TIMER)


def reset_on_problems():
    if not pi.detect_sleepy():
        print("not getting response from sleepy pi, resetting")
        pi.safe_reset_arduino()
        import sys
        sys.exit(1)


def reset_arduino(client,userdata,message):
    pi.safe_reset_arduino()


def set_min_run_voltage(client,userdata,message):
    payload = str(message.payload)
    try:
        volts = float(payload)
        pi.set_minimum_run_voltage(volts)
    except:
        print("error set  minimum run voltage to %s" % payload)


def set_resume_voltage(client,userdata,message):
    payload = str(message.payload)
    try:
        volts = float(payload)
        pi.set_resume_voltage(volts)
    except:
        print("error set  minimum run voltage to %s" % payload)



def s_on_connect(client,userdata,flags,rc):
    state['connected'] = 1
    print("connected!")

def s_on_log(client,userdata,level,buf):
    if (level == mqtt.MQTT_LOG_WARNING) or (level == mqtt.MQTT_LOG_ERR):
        print("MQTT:"+buf)

def set_sleep_regime(client,userdata,message):
    state['regime'] = wake_sleep_sheduler.parse_definition(str(message.payload))
    print("updated sleep regime to " + str(state['regime']))

def tkphoto(client,userdata,message):
    print("taking photo!")
    cam = VideoCapture(0)
    s, img = cam.read()
    for x in range(7):
        sleep(0.1)
        s, img = cam.read()
    sleep(0.1)
    s, img = cam.read()
    if s:
        #gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
        res = cv2.resize(img,dsize=(640,480))
        photo = bytearray(cv2.imencode('.jpg', res)[1].tostring())
        print(type(photo))
        cv2.imwrite('/tmp/photo.jpg',res)
        client.publish(rootkey+'/photo',photo)

def storephoto(client,userdata,message):
    import os
    os.system('mkdir -p /home/pi/photos')
    now = datetime.datetime.now()
    fname = 'still-' + now.strftime('%y%m%d-%H%M%S') + '.jpg'
    os.system('raspistill -o /home/pi/photos/%s' % fname)


parser = argparse.ArgumentParser(description='Process some integers.')

parser.add_argument('--mqtt-host', type=str, help='MQTT host', metavar='N',default='mqtt.dioty.co')
parser.add_argument('--mqtt-port', type=int, help='MQTT host', metavar='N',default=1883)
parser.add_argument('--mqtt-user', type=str, help='MQTT username', metavar='N')
parser.add_argument('--mqtt-password', type=str, help='MQTT password', metavar='N')
parser.add_argument('--allow-shell-commands',action='store_true',help='allow shell commands')
parser.add_argument('--mqtt-root-topic', type=str, help='MQTT root topic', metavar='N')
parser.add_argument('--simulate',action='store_true',help='simulate (do not try to connect to sleepy pi)')
parser.add_argument('--sleep-alarm', type=str, help='sleep until timestamp X (format: HH:MM)', metavar='X')
parser.add_argument('--always-run-voltage', type=float, help='voltage above which we dont go to sleep anymore', default=13.3)
parser.add_argument('--minimum-run-voltage',type=float,help='if voltage drops below X, go into deep sleep mode (do not overwrite settings if already present in arduino)', default=10.5)
parser.add_argument('--resume-voltage', type=float, help='if voltage gets above Y, wake up from deep sleep mode (do not overwrite settings if already present in arduino)', default=11)
parser.add_argument('--regime', type=str, help='regime eg C:600:3600 cyclic wake 10 minutes sleep 1 hour')
parser.add_argument('--reset-on-fail',action='store_true',help='try to reset the arduino when we cannot get response, then exit')

args = parser.parse_args()

state['regime'] = wake_sleep_sheduler.parse_definition(args.regime)


mqttc = mqtt.Client('python_pub')
print(mqttc)
mqttc.on_log = s_on_log
mqttc.username_pw_set(args.mqtt_user,args.mqtt_password)



rootkey = args.mqtt_root_topic
mqttc.will_set(rootkey+'/online',0,retain=True)

mqttc.on_connect = s_on_connect
x = mqttc.connect(args.mqtt_host, args.mqtt_port)
print(x)



def subscribe_with_callback(subtopic, callbackfunc):
    mqttc.subscribe(rootkey+subtopic)
    mqttc.message_callback_add(rootkey+subtopic, callbackfunc)




old_c = {}
pi = offgridpi.SimulatedPi()
if not args.simulate:
    pi = offgridpi.SleepyPi()

while state['connected'] == 0:
    mqttc.loop(10)

if (pi.get_minimum_run_voltage() == 0):
    pi.set_resume_voltage(args.resume_voltage)
    pi.set_minimum_run_voltage(args.minimum_run_voltage)

subscribe_with_callback('/takephoto',tkphoto)
subscribe_with_callback('/storephoto',storephoto)
subscribe_with_callback('/v_minimum/setpoint',set_min_run_voltage)
subscribe_with_callback('/v_resume/setpoint',set_resume_voltage)
subscribe_with_callback('/sleeptimer/setpoint',set_sleep_register)
subscribe_with_callback('/sleeptimer/activate',activate_sleep)
subscribe_with_callback('/sleeptimer/regime/setpoint',set_sleep_regime)
subscribe_with_callback('/sleepypi/reset',reset_arduino)

if args.allow_shell_commands:
    subscribe_with_callback('/shell/cmd',shell_command)


state['startup_time'] = datetime.datetime.now()

while True:
    regime = state['regime']
    state['rootkey'] = rootkey
    if regime.getRemainingRunTimeSeconds(state['startup_time']) == 0:
        if not (pi.get_supply_voltage() > args.always_run_voltage):
            pi.sleepTimer(regime.getNextSleepTimeSeconds(state['startup_time']))
            if (args.simulate):
                # reset regime
                state['startup_time'] = datetime.datetime.now()
    now = datetime.datetime.now()
    new_c = build_ifaces_topics()
    new_c['/online'] = 1
    new_c['/current'] = pi.get_rpi_current()
    new_c['/diskusage'] = disk_usage('/')
    new_c['/scheduler/remaining_time'] = regime.getRemainingRunTimeSeconds(state['startup_time'])
    new_c['/scheduler/next_sleep_time'] = regime.getNextSleepTimeSeconds(state['startup_time'])
    new_c['/voltage'] = pi.get_supply_voltage()
    new_c['/sleeptimer/regime/value']= regime.definition
    diff = get_changes(old_c, new_c)
    old_c = new_c
    for k in diff.keys():
        print("publishing %s to %s" % (rootkey + k, diff[k]))
        mqttc.publish(rootkey + k, diff[k],retain=True)
    start_loop = datetime.datetime.now()
    while now - start_loop < datetime.timedelta(seconds=30):
        x = mqttc.loop(20)
        if x > 0:
            raise Exception(x)
        now = datetime.datetime.now()
    reset_on_problems()


