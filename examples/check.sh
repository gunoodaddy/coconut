#!/bin/bash

valgrind --tool=memcheck --leak-check=yes $1
