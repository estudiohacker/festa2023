# python3.6

import random
import vlc
import time
from paho.mqtt import client as mqtt_client
# create a new instance of the media player
player = vlc.MediaPlayer()
player.set_fullscreen(True)

loop = True
broker = '127.0.0.1'
port = 1883
topic = "painel/chaves"
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
        id = int(msg.payload.decode())%15
        print(id)
        media = vlc.Media(str(id)+".mp4")
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