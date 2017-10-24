#!/usr/bin/env bash

set -e

brew install cmake
brew install yaml-cpp
brew install cfitsio
brew install boost
brew install xerces-c
brew install zmq
brew tap IceCube-SPNO/icecube
brew install cppzmq
brew install git-lfs

# Run git-lfs install command
git lfs install
