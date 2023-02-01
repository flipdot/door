#!/usr/bin/python3 -u
from time import sleep
import logging
import logging.handlers
import sys
import door_lib
from systemd.journal import JournalHandler

logger = logging.getLogger()
logger.setLevel(logging.INFO)
logger.handlers = []
logging.getLogger("door_lib").handlers = []
handler = JournalHandler()
handler.setLevel(logging.INFO)

formatter = logging.Formatter('%(levelname)s - %(message)s')
handler.setFormatter(formatter)

logger.addHandler(handler)

user = ""
if len(sys.argv) > 1:
    user = str(sys.argv[-1].split(' ')[-1])

try:
    door_lib.summer()
    print("welcome")
    logger.warning("DoorSSH - %s - summer", user)
except Exception as e:
    print(e)
    logger.info(e)
    sleep(10)
    pass
