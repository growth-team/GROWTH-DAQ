#!/usr/bin/env bash

set -ex

gem install pi_piper git pry god serialport m2x

# Uninstall rbczmq if already installed.
gem uninstall rbczmq || true

# Uninstall apt-installed libzmq3-dev if already installed.
apt remove -y libzmq3-dev || true

pushd /tmp

  git clone https://github.com/zeromq/zeromq4-x
  git clone https://github.com/growth-team/czmq

  # Build & install zeromq
  install -d zeromq4-x/build-ubuntu
  pushd zeromq4-x/build-ubuntu
  git checkout v4.0.7
  cmake ..
  make -j2 install
  popd

  # Build & install czmq
  install -d czmq/build-ubuntu
  pushd czmq/build-ubuntu
  git checkout raspbian-buster-support
  cmake ..
  make -j2 install
  popd

  # Install rbczmq
  gem install rbczmq -- --with-system-libs

  # Test "require 'rbczmq'
  export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
  ruby -e "require 'rbczmq'"

popd

# Install libbcm2835 that supports Raspberry Pi 4
pushd /tmp
  rm -rf bcm2835-1.64.tar.gz bcm2835-1.64
  wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.64.tar.gz
  tar zxf bcm2835-1.64.tar.gz
  cd bcm2835-1.64
    ./configure && make
    sudo make check
    sudo make install
    cd src && cc -shared bcm2835.o -o libbcm2835.so
      sudo cp libbcm2835.so /var/lib/gems/2.5.0/gems/pi_piper-2.0.0/lib/pi_piper/
    cd ..
  cd ..
popd

#
# If LD_LIBRARY_PATH is not updated, the following error occurs:
#
# root@c534e1b43ce5:/data/czmq/build-ubuntu# ruby -e "require 'rbczmq'"
# Traceback (most recent call last):
# 	10: from -e:1:in `<main>'
# 	 9: from /usr/lib/ruby/2.5.0/rubygems/core_ext/kernel_require.rb:39:in `require'
# 	 8: from /usr/lib/ruby/2.5.0/rubygems/core_ext/kernel_require.rb:135:in `rescue in require'
# 	 7: from /usr/lib/ruby/2.5.0/rubygems/core_ext/kernel_require.rb:135:in `require'
# 	 6: from /var/lib/gems/2.5.0/gems/rbczmq-1.7.9/lib/rbczmq.rb:3:in `<top (required)>'
# 	 5: from /usr/lib/ruby/2.5.0/rubygems/core_ext/kernel_require.rb:59:in `require'
# 	 4: from /usr/lib/ruby/2.5.0/rubygems/core_ext/kernel_require.rb:59:in `require'
# 	 3: from /var/lib/gems/2.5.0/gems/rbczmq-1.7.9/lib/zmq.rb:6:in `<top (required)>'
# 	 2: from /var/lib/gems/2.5.0/gems/rbczmq-1.7.9/lib/zmq.rb:9:in `rescue in <top (required)>'
# 	 1: from /usr/lib/ruby/2.5.0/rubygems/core_ext/kernel_require.rb:59:in `require'
# /usr/lib/ruby/2.5.0/rubygems/core_ext/kernel_require.rb:59:in `require': libczmq.so:
# cannot open shared object file: No such file or directory - /var/lib/gems/2.5.0/gems/rbczmq-1.7.9/lib/rbczmq_ext.so (LoadError)
#
