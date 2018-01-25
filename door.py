#!/usr/bin/python2 -u
import serial
from time import sleep
import logging
import logging.handlers



logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
handler = logging.handlers.SysLogHandler(address = '/dev/log')
handler.setLevel(logging.INFO)

formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)

logger.addHandler(handler)


try:
        s = serial.Serial('/dev/ttyAMA0', baudrate=9600, timeout=10)

        line = s.readline()
        logger.debug("line: {}".format(line))

	adc = int(line.split(" ")[0])
        logger.debug("adc: {}".format(adc))
	if adc < 500:
		s.write('1')
		print("hallo {}".format(adc))
	else:
		s.write('0')
		print("ade {}".format(adc))
except Exception as e:
	print(e)
        logger.info(e)
        sleep(10)
        pass
s.close()
