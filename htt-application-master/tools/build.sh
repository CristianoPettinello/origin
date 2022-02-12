#!/bin/bash

cd /source/ext-dep/jsoncpp
make clean; make -j4

# cd /source/libraries/UsbDriver
# make clean; make -j4

cd /source/libraries/Utils
make clean; make -j4

cd /source/solution/makefile
make clean; make -j4
