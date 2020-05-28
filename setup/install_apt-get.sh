#!/usr/bin/env bash

set -o errexit

sudo apt-get install -y software-properties-common
sudo apt-get update
sudo apt-get install -y \
  automake \
  binutils \
  build-essential \
  chromium \
  cmake \
  curl \
  dpkg-dev \
  fswebcam \
  git \
  i2c-tools \
  imagemagick \
  libboost-all-dev \
  libbz2-dev \
  libcfitsio-dev \
  libcurl4-openssl-dev \
  libexpat-dev \
  libjpeg8-dev \
  libncurses-dev \
  libssl-dev \
  libtool-bin \
  libx11-dev \
  libxerces-c-dev \
  libxext-dev \
  libxft-dev \
  libxpm-dev \
  libyaml-cpp-dev \
  make \
  python-dev \
  python-smbus \
  python-zmq \
  rails \
  ruby \
  ruby-dev \
  ruby-ffi \
  subversion \
  supervisor \
  swig \
  texinfo \
  wget \
  wicd-curses \
  zlib1g-dev \
  zsh
