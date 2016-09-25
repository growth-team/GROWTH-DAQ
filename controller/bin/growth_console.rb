#!/usr/bin/env ruby

require "pry"
require "growth_controller/logger"
require "growth_controller/console_modules"

@logger = Logger.new(STDOUT)

#---------------------------------------------
# Instantiate ConsoleModules
#---------------------------------------------
det  = GROWTH::ConsoleModuleDetector.new("det", logger: @logger)
hv   = GROWTH::ConsoleModuleHV.new("hv", logger: @logger)
disp = GROWTH::ConsoleModuleDisplay.new("disp", logger: @logger)
hk   = GROWTH::ConsoleModuleHK.new("hk", logger: @logger)
daq  = GROWTH::ConsoleModuleDAQ.new("daq", logger: @logger)

#---------------------------------------------
# Start console
#---------------------------------------------
binding.pry
