import paho.mqtt.client as mqtt
import netifaces
from time import sleep
from cv2 import *
import cv2
import offgridpi
import argparse
import datetime




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


def tkphoto(client,userdata,message):
    cam = VideoCapture(0)
    s, img = cam.read()
    sleep(0.05)
    s, img = cam.read()
    sleep(0.05)
    s, img = cam.read()
    if s:
        #gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
        res = cv2.resize(img,dsize=(640,480))
        photo = cv2.imencode('.jpg', res)[1].tobytes()
        cv2.imwrite('/tmp/photo.jpg',res)
        client.publish(rootkey+'/photo',photo)


parser = argparse.ArgumentParser(description='Process some integers.')

parser.add_argument('--mqtt-host', type=str, help='MQTT host', metavar='N',default='mqtt.dioty.co')
parser.add_argument('--mqtt-port', type=int, help='MQTT host', metavar='N',default=1883)
parser.add_argument('--mqtt-user', type=str, help='MQTT username', metavar='N')
parser.add_argument('--mqtt-password', type=str, help='MQTT password', metavar='N')
parser.add_argument('--mqtt-root-topic', type=str, help='MQTT root topic', metavar='N')

parser.add_argument('--sleep-alarm', type=str, help='sleep until timestamp X (format: HH:MM)', metavar='X')
args = parser.parse_args()



mqttc = mqtt.Client('python_pub')
mqttc.username_pw_set(args.mqtt_user,args.mqtt_password)
rootkey = args.mqtt_root_topic
mqttc.connect(args.mqtt_host, args.mqtt_port)
mqttc.subscribe(rootkey+'/takephoto')
mqttc.message_callback_add(rootkey+'/takephoto',tkphoto)
old_c = {}
mqttc.will_set(rootkey+'/online',0,retain=True)
pi = offgridpi.SleepyPi()

while True:
    new_c = build_ifaces_topics()
    new_c['/online'] = 1
    new_c['/current'] = pi.get_rpi_current()
    new_c['/voltage'] = pi.get_supply_voltage()
    diff = get_changes(old_c, new_c)
    old_c = new_c
    for k in diff.keys():
        print("publishing %s to %s" % (rootkey + k, diff[k]))
        mqttc.publish(rootkey + k, diff[k],retain=True)
    start_loop = datetime.datetime.now()
    while datetime.datetime.now() - start_loop < datetime.timedelta(seconds=30):
        x = mqttc.loop(20)
        if x > 0:
            raise Exception(x)


