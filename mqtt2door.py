import logging
from systemd.journal import JournalHandler
import sys
import threading
import time
import serial

import paho.mqtt.client as mqtt
import json

import door_lib

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

logger.addHandler(JournalHandler())


def on_close_request(client, userdata, msg):
    if msg.topic == "actor/door/close" or \
       msg.payload.startswith('{"esp_id":"2c:3a:e8:27:44:01:","switch_closed":true'):
        logger.info(msg.topic+" "+str(msg.payload))
        door_lib.close()

def on_disconnect(client, userdata, rc):
    if rc != 0:
        logger.info("unexpected disconnect")
        sys.exit(1)

def updater():
    logger.info("updater started")
    while True:
        time.sleep(60)
        try:
            logger.debug("updater loop")
            door_lib.update_api()
        except Exception as e:
            logger.exception("updater")
            pass
    logger.info("updater died")


door_state_updater = threading.Thread(target=updater)
door_state_updater.daemon = True
door_state_updater.start()

client = mqtt.Client()
client.on_message = on_close_request
client.on_disconnect = on_disconnect
client.connect("power.flipdot.space", 1883, 60)
client.subscribe("actor/door/close")
client.subscribe("sensors/all")
client.loop_start()


rfid_serial = serial.Serial('/dev/ttyUSB0', baudrate=9600, timeout=2)
valid_tags = json.load(open('rfid_tags.json'))

logger.info("read {} tags".format(len(valid_tags)))

while True:
    id_tag = rfid_serial.read(13)
    if id_tag:
        id_tag = id_tag.decode('utf-8').strip(" \t\n\2")
	logger.info("read tag {}".format(id_tag))
        if id_tag in valid_tags:
            door_lib.open()
            logger.info("open door")
        else:
            logger.info("invalid RFID tag")
