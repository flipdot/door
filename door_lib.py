import contextlib
import logging
from systemd.journal import JournalHandler

import requests
import serial
from time import sleep

import config

base_url = "https://api.flipdot.org/sensors/door/locked/"
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
logger.addHandler(JournalHandler())

def close():
    with get_serial() as s:
        try:
            if is_open(s):
                s.write('0')
                return True
            else:
                return False
        except Exception as e:
            logger.exception("close")
            pass

def open():
    with get_serial() as s:
        try:
            if not is_open(s):
                s.write('1')
                return True
            else:
                return False
        except Exception as e:
            logger.exception("open")
            pass


def update_api(locked=None):
    logger.debug("update api")
    if locked is None:
        with get_serial() as s:
            locked = not is_open(s)
            logger.debug("door is {}".format(locked))

    value = 1 if locked else 0
    logger.debug("update api to {}".format(value))
    try:
        logger.debug("posted api: {}".format(requests.get(base_url + str(value), timeout=3).content))
    except Exception as e:
        logger.exception("error posting api status")


def get_serial():
    s = serial.Serial(config.SERIAL, baudrate=9600, timeout=10)
    return contextlib.closing(s)

def get_state(s):
    for i in range(5):
        line = s.readline()
        logger.debug("line: {}".format(line))
        if not line or len(line) < 3:
            continue
        first = line.split(" ")[0]
        try:
            adc = int(first)
        except ValueError:
            msg = "ValueError: '{}' is not a valid door state.".format(first)
            logger.error(msg)
            sleep(1)
            continue
        logger.debug("adc: {}".format(adc))
        return adc > 500, adc
    return None, "unk"

def is_open(s):
    door_is_open, _ = get_state(s)
    return door_is_open
