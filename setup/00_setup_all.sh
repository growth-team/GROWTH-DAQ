#!/usr/bin/env bash

set -o nounset
set -o errexit

source ./setenv.sh

install -d ${git_dir}
install -d ${log_dir}

#---------------------------------------------
# Define log file names
#---------------------------------------------
date=`date +'%Y%m%d_%H%M'`
log_git=${log_dir}/log_setup_all_git_clone_${date}.text
log_apt=${log_dir}/log_setup_all_apt_${date}.text
log_python=${log_dir}/log_setup_all_python_${date}.text
log_adafruit=${log_dir}/log_setup_all_adafruit_${date}.text
log_ruby_gem=${log_dir}/log_setup_all_ruby_gem_${date}.text
log_wiringPi=${log_dir}/log_setup_all_wiringPi_${date}.text

#---------------------------------------------
# Clone GROWTH-FY2015-Software repo if not present
#---------------------------------------------
pushd ${git_dir}
  if [ ! -f ${repo} ]; then
  	git clone https://github.com/growth-team/${repo} > ${log_git}
  	if [ ! -f ${repo} ]; then
  		echo "Error: git repository could not be cloned."
  		exit -1
  	fi
  fi
  pushd ${git_dir}/${repo}
    git submodule init
    git submodule update
  popd
popd


#---------------------------------------------
# Copy (link) zsh profile
#---------------------------------------------
pushd $HOME
  if [ -L $HOME/.zshrc ]; then
    rm -f $HOME/.zshrc
  fi
  if [ -f $HOME/.zshrc ]; then
    # Backup the exisiting .zshrc
    mv $HOME/.zshrc $HOME/.zshrc_${date}
  fi
  ln -s ${git_dir}/${repo}/raspi_setup/zshrc .zshrc
popd


#---------------------------------------------
# Install required libraries/software
#---------------------------------------------
sudo ./install_apt-get.sh > ${log_apt}
sudo ./install_python.sh > ${log_python}
./install_adafruit_ssd1306.sh > ${log_adafruit}
sudo ./install_ruby_gem.sh > ${log_ruby_gem}
sudo ./install_wiringPi.sh > ${log_wiringPi}


#---------------------------------------------
# Prepare .ssh directory and authorized_keys
#---------------------------------------------
./setup_ssh.sh
