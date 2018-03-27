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
                send_api(True)
                return True
            else:
                send_api(True)
                return False
        except Exception as e:
            print(e)
            pass

def open():
    with get_serial() as s:
        try:
            if not is_open(s):
                s.write('1')
                send_api(False)
                return True
            else:
                send_api(False)
                return False
        except Exception as e:
            print(e)
            pass


def send_api(value):
    try:
        requests.get(base_url + str(value), timeout=3)
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
