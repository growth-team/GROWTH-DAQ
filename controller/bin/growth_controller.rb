#!/usr/bin/env ruby
require "growth_controller/detector_controller"

# 
# This program runs as a daemon on Raspberry Pi of the detector.
# The daemon acts as a ZeroMQ server and accepts ZeroMQ client connection.
# Clients send JSON commands to the server to controll the detector, 
# to start/stop data acquisition, and to retrieve internal status of the detector.
# 

@logger = GROWTH.logger(ARGV, "growth_controller")
controller = GROWTH::DetectorController.new(logger: @logger)
controller.run()
