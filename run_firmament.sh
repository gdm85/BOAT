#!/bin/bash

set -e

mkdir -p build
cd build
cmake ../src
make firmament -j
cd firmament
./firmament
