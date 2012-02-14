#!/bin/bash

rm -rf result.valgrind*
valgrind -v --tool=memcheck --leak-check=yes  --show-reachable=yes  --log-file=result.valgrind $*
