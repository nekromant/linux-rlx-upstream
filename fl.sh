#!/bin/bash

PORT=/dev/ttyUSB0

#echo "LOADADDR 80000000" > $PORT
sleep 1
sudo ifconfig eth0 up 192.168.1.1
echo -en "rexmt 1\ntrace\nbinary\nput $1\nquit\n" | tftp 192.168.1.6
sleep 1
echo "J 80500000" > $PORT
