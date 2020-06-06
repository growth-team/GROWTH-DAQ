#!/usr/bin/env bash

set -ex

pushd /tmp
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.64.tar.gz
tar zxf bcm2835-1.64.tar.gz
cd bcm2835-1.64

./configure && make
sudo make check
sudo make install
cd src && cc -shared bcm2835.o -o libbcm2835.so
cp libbcm2835.so /var/lib/gems/2.5.0/gems/pi_piper-2.0.0/lib/pi_piper/

cd ..
popd


