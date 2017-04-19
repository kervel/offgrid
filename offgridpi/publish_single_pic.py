import datetime
import os, os.path
import offgridpi
import paho.mqtt.client as mqtt
from cv2 import *
import cv2
from time import sleep
import argparse

state = {
    'connected' : 0
}


parser = argparse.ArgumentParser(description='Process some integers.')

parser.add_argument('--mqtt-host', type=str, help='MQTT host', metavar='N',default='mqtt.dioty.co')
parser.add_argument('--mqtt-port', type=int, help='MQTT host', metavar='N',default=1883)
parser.add_argument('--mqtt-user', type=str, help='MQTT username', metavar='N')
parser.add_argument('--mqtt-password', type=str, help='MQTT password', metavar='N')
parser.add_argument('--mqtt-root-topic', type=str, help='MQTT root topic', metavar='N')
args = parser.parse_args()


def take_photo(client):
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
        #print(type(photo))
        #cv2.imwrite('/tmp/photo.jpg',res)
        client.publish(state['rootkey']+'/auto_photo',photo,retain=True)
        client.publish(state['rootkey']+'/auto_phototaken',1)


def s_on_log(client,userdata,level,buf):
    if (level == mqtt.MQTT_LOG_WARNING) or (level == mqtt.MQTT_LOG_ERR):
        print("MQTT:"+buf)

state['rootkey'] = args.mqtt_root_topic

mqttc = mqtt.Client('python_pub')
print(mqttc)
mqttc.on_log = s_on_log
mqttc.username_pw_set(args.mqtt_user,args.mqtt_password)
mqttc.connect(args.mqtt_host, args.mqtt_port)
mqttc.loop(2)
take_photo(mqttc)
while mqttc.want_write():
    mqttc.loop(1)

mqttc.disconnect()
