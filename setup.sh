#!/bin/bash

sudo apt-get install make g++ cmake libglfw3-dev nlohmann-json-dev

git submodule update --init
mkdir build && cd build
cmake ../ 
make -j6
