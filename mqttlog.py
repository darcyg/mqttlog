import socket
import time
import paho.mqtt.client as mqtt
from time import gmtime, strftime

# global stuff
logfilename = strftime("/home/pi/heizungslog_%Y-%m-%d_%H-%M-%S.log", gmtime())
with open(logfilename,'a') as myFile:
    myFile.write("# " + logfilename + " created " + strftime("%Y-%m-%d %H:%M:%S", gmtime())+"\n")

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, rc):
    log("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("#")

# fix numberism
def num(s):
    try:
        return float(s)
    except ValueError:
        return 0

def log(s):
    actime = strftime("%Y-%m-%d %H:%M:%S", gmtime())
    with open('samplelogt.log','a') as myFile:
        myFile.write("# "+actime+" "+s+"\n")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    # separate the topic by / and aggregate it with . again
    actime = strftime("%Y-%m-%d %H:%M:%S", gmtime())
    msg.topic = msg.topic.replace("/",".")
    if msg.topic[0] is not '$':
        with open(logfilename,'a') as myFile:
            myFile.write(actime+" "+msg.topic+" "+str(msg.payload)+"\n")
    print(msg.topic+"  "+ msg.payload)
    #send_metric(msg.topic, msg.payload)

def send_metric(name, value):
    sock = socket.socket()
    if sock is None:
        log("Socket open error")
    else:
        log("opening socket")
        sock.connect(('localhost', 2003))
        sock.send("%s %d %d\n" % (name, num(value), int(time.time())))
        sock.close()


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()




