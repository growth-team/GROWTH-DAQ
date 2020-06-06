#!/usr/bin/env bash

set -o errexit
source ./setenv.sh

sudo apt-get install -y build-essential python-dev python-pip
sudo pip install RPi.GPIO

if [ ! -d ${git_dir}/${repo} ]; then
  echo "Error: git repo does not exist (expected at ${git_dir}/${repo})."
  echo "Please run 00_setup_all.sh"
  exit 1
fi

pushd ${git_dir}/${repo}
  git submodule init
  git submodule update

  if [ ! -d lib/Adafruit_Python_SSD1306 ]; then
    echo "Error: failed to clone Adafruit_Python_SSD1306"
    exit 1
  fi

  pushd lib/Adafruit_Python_SSD1306
    sudo python setup.py install
    # Check build
    gpio -v
  popd
popd
