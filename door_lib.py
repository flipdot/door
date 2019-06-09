import contextlib
import logging
from systemd.journal import JournalHandler
import requests
from time import sleep
from smbus2 import SMBus
import config

DOOR_CMD_OPEN = 0x23
DOOR_CMD_CLOSE = 0x42
DOOR_CMD_STATUS = 0xBB


base_url = "https://api.flipdot.org/sensors/door/locked/"
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
logger.addHandler(JournalHandler())

addr = 0x42

def close():
    with get_serial() as bus:
        try:
            if is_open(bus):
                bus.write_word_data(addr, DOOR_CMD_CLOSE, 0xCAFE)
                return True
            else:
                return False
        except Exception as e:
            logger.exception("close")
            pass

def open():
    with get_serial() as bus:
        try:
            if not is_open(bus):
                bus.write_word_data(addr, DOOR_CMD_OPEN, 0xCAFE)
                return True
            else:
                return False
        except Exception as e:
            logger.exception("open")
            pass

def toggle():
    with get_serial() as bus:
        try:
            if not is_open(bus):
                bus.write_word_data(addr, DOOR_CMD_OPEN, 0xCAFE)
                return True
            else:
                print("already open")
                bus.write_word_data(addr, DOOR_CMD_CLOSE, 0xCAFE)
                return False
        except Exception as e:
            logger.exception("toggle")
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
    bus = SMBus(1)
    return contextlib.closing(bus)

def get_state(bus):

    bus.write_word_data(addr, DOOR_CMD_STATUS, 0xCAFE)
    sleep(0.5)
    #state = bus.read_word_data(addr, 0)
    state = bus.read_byte(addr)
    foo = bus.read_byte(addr)
    print("state2 %02X %c" % (state,foo))
    if state == 0x01:
        print("is open")
        return True, 0
    else:
        print("is closed")
        return False, 800


def is_open(s):
    door_is_open, _ = get_state(s)
    return door_is_open
