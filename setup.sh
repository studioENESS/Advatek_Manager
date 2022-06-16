#!/bin/bash

sudo apt-get install make cmake libglfw3-dev

git submodule update --init
cd External/boost
git submodule update --init
./bootstrap.sh or bootstrap.bat
./b2
cd ../../
mkdir build && cd build
cmake ../ 
make -j6
