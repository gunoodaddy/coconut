#!/bin/bash

#valgrind --tool=memcheck --leak-check=yes $1
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes $1
