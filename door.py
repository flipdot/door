#!/usr/bin/python2 -u
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

try:
    door_lib.toggle()
except Exception as e:
    print(e)
    logger.info(e)
    sleep(10)
    pass
