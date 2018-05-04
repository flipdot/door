import contextlib
import logging

import requests
import serial

import config

base_url = "https://api.flipdot.org/sensors/door/locked/"

def close():
    with get_serial() as s:
        try:
            if is_open(s):
                s.write('0')
                update_api(locked=True)
                return True
            else:
                update_api(locked=True)
                return False
        except Exception as e:
            print(e)
            pass

def open():
    with get_serial() as s:
        try:
            if not is_open(s):
                s.write('1')
                update_api(locked=False)
                return True
            else:
                update_api(locked=False)
                return False
        except Exception as e:
            print(e)
            pass


def update_api(locked=None):
    if locked is None:
        with get_serial() as s:
            locked = not is_open(s)

    value = 1 if locked else 0
    try:
        print("posted api: ", requests.get(base_url + str(value), timeout=3).content)
    except Exception as e:
        print("error posting status", repr(e))


def get_serial():
    s = serial.Serial(config.SERIAL, baudrate=9600, timeout=10)
    return contextlib.closing(s)

def get_state(s):
    line = s.readline()
    logging.debug("line: {}".format(line))
    first = line.split(" ")[0]
    try:
        adc = int(first)
    except ValueError:
        msg = "ValueError: '%s' is not a valid door state" % first
        logging.error(msg)
        print(msg)
        return False, first
    logging.debug("adc: {}".format(adc))
    return adc > 500, adc

def is_open(s):
    door_is_open, _ = get_state(s)
    return door_is_open
