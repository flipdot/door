#!/bin/bash
stty -F /dev/ttyAMA0 9600 cs8 cread clocal
read ADC < /dev/ttyAMA0

ADC=$(echo $ADC | awk -F " " '{ print $1 }')

#read ADC < /dev/ttyAMA0

if [ "625" -lt "$ADC" ]; then
	OPEN=1
else
	OPEN=0
fi
echo $OPEN
#wget -q -O /dev/null "http://flipdot.vega.uberspace.de/spacestatus/door.php?state=$OPEN"
