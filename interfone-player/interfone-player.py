# python3.6

import random
import vlc
import time
from paho.mqtt import client as mqtt_client
# create a new instance of the media player
player = vlc.MediaPlayer()
player.set_fullscreen(False)

loop = True
broker = '172.17.77.172'
port = 1883
topic = "interfone/+/sensor/chave/+"
# Generate a Client ID with the subscribe prefix.
client_id = f'subscribe-{random.randint(0, 100)}'
username = 'quilombo'
password = 'sesc'
def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")
        chave = msg.topic.split('/')[-1]
        valor = int(msg.payload.decode())
        media = vlc.Media(str(chave) + "." + str(valor) + ".jpg")
        player.set_media(media)
        player.play()

    client.subscribe(topic)
    client.on_message = on_message


def run():
    client = connect_mqtt()
    subscribe(client)
    client.loop_forever()
    
if __name__ == '__main__':
    run()