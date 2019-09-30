#!/usr/bin/env bash

set -o errexit

mkdir -p "${HOME}"/work/install
pushd "${HOME}"/work/install
  git clone https://github.com/WiringPi/wiringpi
pushd wiringpi
./build

#check build
gpio -v

popd
popd
