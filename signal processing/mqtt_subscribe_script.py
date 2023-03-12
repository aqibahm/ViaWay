from paho.mqtt import client as mqtt_client
import time, sys, random, json
from pprint import pprint
import pandas as pd
from pathlib import Path

# broker="raspberrypi.home"
broker = "10.18.239.147"
#broker="sas"
#broker="ws4"
#broker="sas"
#broker="192.168.1.1"
#broker="test.mosquitto.org"
# port = 8883
#port=9001
port =1883
username = "aqib"
password = "aqib@123"
conn_flag = False
topic = "module/wireless/acceleration_z"

# generate client ID with pub prefix randomly
client_id = f'python-mqtt-{random.randint(0, 100)}'

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

# empty list to store the data
data = {}

def subscribe(client: mqtt_client):
    # Run code for 30 s:
    def on_message(client, userdata, msg):
        data[time.time()] = str(msg.payload.decode())
        # data.append(str(msg.payload.decode()))
        # print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    client.subscribe(topic)
    client.on_message = on_message

def run():
    start_time = time.time()
    # Read and record the data
    record_time = 5.0
    client = connect_mqtt()
    subscribe(client)   
    client.loop_start()

    while time.time() - start_time < record_time:
        time_cur = time.time() - start_time
        print('Time: ' + str(time_cur))

    pprint(dict(data), indent = 4, width = 1)
    print(len(data))

    #  Convert dict to DataFrame
    data_ = {"time": list(data.keys()), "accel_z": list(data.values())}
    filepath = Path("./ViaWay/data/" + str(client_id) + str(time.time()) + ".csv")
    filepath.parent.mkdir(parents=True, exist_ok=True)
    data_df = pd.DataFrame.from_dict(data_)
    data_df.to_csv(filepath, index = False)

if __name__ == '__main__':
    run()
