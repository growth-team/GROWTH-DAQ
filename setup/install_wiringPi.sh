#!/usr/bin/env bash

mkdir -p $HOME/work/install
pushd $HOME/work/install
git clone git://git.drogon.net/wiringPi
pushd wiringPi
./build

#check build
gpio -v

popd
popd
