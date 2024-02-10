#!/bin/bash

if [ "$#" -ne 1 ]; then
	echo "Usage: $0 <target device>"
	exit 1
fi 

target="$1"
sz=$(blockdev --getsz $target)

proxydev=$(ls /dev/mapper | grep proxy-device | wc -l);
if [ "$proxydev" -gt 0 ]; then
	dmsetup remove proxy-device
fi 

mod=$(lsmod | grep dm_linear_stats | wc -l)
if [ "$mod" -gt 0 ]; then
	rmmod dm_linear_stats
fi 

insmod dm-linear-stats.ko

dmsetup create proxy-device --table "0 $sz linear-stats $1"
