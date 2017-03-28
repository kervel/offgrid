import paho.mqtt.client as mqtt
import argparse
import datetime
import sys

parser = argparse.ArgumentParser(description='Process some integers.')

parser.add_argument('--mqtt-host', type=str, help='MQTT host', metavar='N',default='mqtt.dioty.co')
parser.add_argument('--mqtt-port', type=int, help='MQTT host', metavar='N',default=1883)
parser.add_argument('--mqtt-user', type=str, help='MQTT username', metavar='N')
parser.add_argument('--mqtt-password', type=str, help='MQTT password', metavar='N')
parser.add_argument('--topic', type=str, help='topic', metavar='N')
parser.add_argument('--payload', type=str, help='topic', metavar='N')

args = parser.parse_args()

if args.topic == '' or args.topic is None:
    parser.print_usage()
    sys.exit(1)


mqttc = mqtt.Client('python_setter')
print(mqttc)
mqttc.username_pw_set(args.mqtt_user,args.mqtt_password)

mqttc.connect(args.mqtt_host, args.mqtt_port)


state = {
    'connected' : 0
}


def s_on_connect(client,userdata,flags,rc):
    state['connected'] = 1
    print("connected!")


success_c = False
mqttc.on_connect = s_on_connect
print("trying to connect to %s:%i" % (args.mqtt_host, args.mqtt_port))
x = mqttc.connect(args.mqtt_host, args.mqtt_port)

while state['connected'] == 0:
    mqttc.loop(10)
    print("waiting for connection ...")



mqttc.publish(args.topic, args.payload, retain=True)
mqttc.loop(1)

