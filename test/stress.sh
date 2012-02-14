#!/bin/bash

killall -9 unittest

rm -rf core*
rm -rf *.log
rm -rf *.log.*

while :
do
	./unittest $1
	if [ $? != 0 ]; then
		echo "Your program has problems..."
		exit 0
	fi
done
