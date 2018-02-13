import paho.mqtt.client as mqtt
import serial
import logging
import logging.handlers
import sys

import door_lib

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
handler = logging.handlers.SysLogHandler(address = '/dev/log')
handler.setLevel(logging.INFO)

formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)

logger.addHandler(handler)


def on_close_request(client, userdata, msg):
    if msg.topic == "actor/door/close" or \
       msg.payload.startswith('{"esp_id":"2c:3a:e8:27:44:01:","switch_closed":true'):
        logger.info(msg.topic+" "+str(msg.payload))
        door_lib.close()

def on_disconnect(client, userdata, rc):
    if rc != 0:
        logger.info("unexpected disconnect")
        sys.exit(1)
   
client = mqtt.Client()
client.on_message = on_close_request
client.on_disconnect = on_disconnect
client.connect("power-pi.fd", 1883, 60)
client.subscribe("actor/door/close")
client.subscribe("sensors/all")
client.loop_forever()

