#/usr/bin/env bash

set -e

# Build external libraries and install to the local directory

mkdir build_libzmq
cd build_libzmq
cmake ../libzmq -DCMAKE_INSTALL_PREFIX=../..
make -j2 install
rm -rf build_libzmq
cd ..

mkdir build_cppzmq
cd build_cppzmq
cmake ../cppzmq -DCMAKE_INSTALL_PREFIX=../.. -DCMAKE_MODULE_PATH=../..
make -j2 install
rm -rf build_cppzmq
cd ..
