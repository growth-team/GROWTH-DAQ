#!/usr/bin/env bash

set -e

brew install \
cmake \
yaml-cpp \
cfitsio \
boost@1.60 \
xerces-c \
git-lfs

# Run git-lfs install command
git lfs install
