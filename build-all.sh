#!/bin/bash

# github.com/sjoon-oh/hartebeest
# Author: Sukjoon Oh, sjoon@kaist.ac.kr
# build-all.cpp

# rm ./*.json
rm ./build/*.json

# make clean
# rm -rf build/*

# rm -rf CMakeCache.txt
# rm -rf CMakeFile

cmake .
make