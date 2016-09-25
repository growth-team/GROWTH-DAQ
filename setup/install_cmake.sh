#!/usr/bin/env bash

install -d $HOME/work/install/cmake

pushd $HOME/work/install/cmake

# Try downloading a compiled binary from thdr.info
wget http://thdr.info/raspi/cmake 2> /dev/null
if [ -f cmake ]; then
  mv cmake $HOME/work/install/bin/
  exit 0
fi

# Build from source
if [ ! -f cmake-3.6.2.tar.gz ]; then
	wget https://cmake.org/files/v3.6/cmake-3.6.2.tar.gz
fi
tar zxf cmake-3.6.2.tar.gz
pushd cmake-3.6.2
./configure --prefix=$HOME/work/install
make -j4 install
popd

popd
