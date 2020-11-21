import paho.mqtt.client as mqtt
import subprocess
import time

MQTT_ADDRESS = '192.168.1.102'
MQTT_USER = 'engin'
MQTT_PASSWORD = '****'
MQTT_TOPIC = 'doorstatus'


def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('Connected with result code ' + str(rc))
    client.subscribe(MQTT_TOPIC)


def on_message(client, userdata, msg):
    """The callback for when a PUBLISH message is received from the server."""
    print(msg.topic + ' ' + str(msg.payload))
    if str(msg.payload) == "OPEN":
        subprocess.call(['sh', '/home/pi/./takephoto.sh'])
        subprocess.call(['sh', '/home/pi/./takephoto.sh'])
        time.sleep(2)
        subprocess.call(['sh', '/home/pi/./takephoto.sh'])


def main():
    mqtt_client = mqtt.Client()
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(MQTT_ADDRESS, 1883)
    mqtt_client.loop_forever()


if __name__ == '__main__':
    print('MQTT islemleri')
    main()

