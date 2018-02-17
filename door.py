#!/usr/bin/python2 -u
import serial
from time import sleep
import logging
import logging.handlers

import door_lib

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
handler = logging.handlers.SysLogHandler(address='/dev/log')
handler.setLevel(logging.INFO)

formatter = logging.Formatter(
    '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)

logger.addHandler(handler)

with door_lib.get_serial() as s:
    try:
        door_is_open, adc = door_lib.get_state(s)
        if door_is_open:
            s.write('0')
            print("ade", adc)
        else:
            s.write('1')
            print("hallo", adc)
    except Exception as e:
        print(e)
        logger.info(e)
        sleep(10)
        pass
