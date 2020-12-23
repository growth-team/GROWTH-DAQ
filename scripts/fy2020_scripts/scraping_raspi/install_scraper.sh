#!/bin/sh

HOMEDIR=$HOME

cd $HOMEDIR/git/
git clone git://github.com/n1k0/casperjs.git
sudo ln -sf `pwd`/casperjs/bin/casperjs /usr/local/bin/casperjs
git clone https://github.com/piksel/phantomjs-raspberrypi.git
sudo chmod +x phantomjs-raspberrypi/bin/phantomjs
sudo ln -s `pwd`/phantomjs-raspberrypi/bin/phantomjs /usr/local/bin/phantomjs

