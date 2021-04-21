#!/usr/bin/env bash

set -e

brew install \
cmake \
yaml-cpp \
cfitsio \
xerces-c \
git-lfs

brew tap growth-team/homebrew-growth
brew install growth-team/growth/boost@1.60

# Run git-lfs install command
git lfs install
