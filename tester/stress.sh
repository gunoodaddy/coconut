#!/bin/bash

rm -rf core*
rm -rf *.log

while :
do
	./protocoltest
	if [ $? != 0 ]; then
		echo "Your program has problems..."
		exit 0
	fi
done
