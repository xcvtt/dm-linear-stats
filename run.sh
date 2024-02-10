#!/bin/bash

if [ "$#" -ne 1 ]; then
	echo "Usage: $0 <target device>"
	exit 1
fi 

target="$1"
sz=$(blockdev --getsz $target)

dmsetup create test-device --table "0 $sz linear-stats $1"
