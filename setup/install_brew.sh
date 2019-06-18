#!/usr/bin/env bash

set -e

brew install \
cmake \
yaml-cpp \
cfitsio \
boost@1.60 \
xerces-c \
zmq \
git-lfs

brew tap IceCube-SPNO/icecube
brew install cppzmq

# Run git-lfs install command
git lfs install
