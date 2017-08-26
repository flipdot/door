#!/bin/bash

stty -F /dev/ttyAMA0 9600 cs8 cread clocal

read ADC < /dev/ttyAMA0


ADC=$(echo $ADC | awk -F " " '{ print $1 }')
echo $ADC

if [ "$ADC" -lt "500" ]; then
	echo -n "1" > /dev/ttyAMA0
	echo "hallo"
else
	echo -n "0" > /dev/ttyAMA0
	echo "ade"
fi
