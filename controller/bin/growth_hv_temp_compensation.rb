#!/usr/bin/env ruby

require "/home/pi/git/GROWTH-DAQ/controller/lib/growth_controller/logger"
require "/home/pi/git/GROWTH-DAQ/controller/lib/growth_controller/config"
require "/home/pi/git/GROWTH-DAQ/controller/lib/growth_controller/console_modules"

# This script control HV outputs for temperature compensation of SiPM sensors.

class HVTempControl
  
  WAIT_DURATION_SEC = 60
  
  def initialize()
    @logger = GROWTH.logger(ARGV, "growth_hv_temp_compensation")
    puts "initialize"
    @growth_config = GROWTH::Config.new(logger: @logger)
    @hv = GROWTH::ConsoleModuleHV.new("hv", logger: @logger)
    @hk = GROWTH::ConsoleModuleHK.new("hk", logger: @logger)
  end
  
  attr_accessor :stopped
  
  def run()
    while(true)
      @logger.debug("Autorun check...")
      hk = @hk.read()
      temp=hk["hk"]["bme280"]["temperature"]["value"]
      if (temp>=-10.0)&&(temp<50.0) then
        for ch in [0,1]
          hv_value=@growth_config.get_controlled_hv(ch, temp)
          @hv.set(ch, hv_value)
        end
      end
      sleep(WAIT_DURATION_SEC)
    end
  end
end

hv_temp_control = HVTempControl.new()
hv_temp_control.run()
