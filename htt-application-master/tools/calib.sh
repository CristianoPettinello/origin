#!/bin/bash

if [ ! -L liblog4cpp.so.5 ]; then
    ln -s liblog4cpp.so.5.0.6 liblog4cpp.so.5
fi

# LD_LIBRARY_PATH=. ./build/main -c -s ./build/settings.json -l ./build/log4cpp.properties -f ./sample_data/
LD_LIBRARY_PATH=. ./build/main -c
