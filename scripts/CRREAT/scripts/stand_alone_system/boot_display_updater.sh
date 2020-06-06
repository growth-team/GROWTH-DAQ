#!/bin/sh

sleep 10s
export RUBYLIB=$HOME/git/GROWTH-DAQ/controller/lib:$RUBYLIB
$HOME/git/GROWTH-DAQ/controller/bin/growth_display_updater.rb
