#!/usr/bin/env ruby

# if this script doesn't work because of no installation of "pi_piper", execute
# sudo apt-get install ruby-ffi ruby-dev libssl-dev
# sudo gem install pi_piper

require "pi_piper"

RESOLUTION_AT_12BIT = 4095.0

# Reference voltage of MCP3208
V_REF = 3.3

# LM60 Temperature sensor
LM60_OFFSET_V = 0.424
LM60_COEFFICIENT_V_PER_DEG = 0.00625

# LT6106 Current sense amplifier
LT6106_ROUT = 10e3   #  10k Ohm
LT6106_RIN  = 100    #  100 Ohm
LT6106_RSENSE = 0.02 # 0.02 Ohm

# Labels
TEMPERATURE_LABEL = {0=>"FPGA", 1=>"DCDC"}
CURRENT_VOLTAGE_RAIL = {2=>"12V", 3=> "5V", 4=>"3.3V"}
SLOWADC_LABEL = {5=>"Slow ADC 5", 6=>"Slow ADC 6", 7=>"Slow ADC 7"}

# Read ADC values
PiPiper::Spi.begin(PiPiper::Spi::CHIP_SELECT_1) do |spi|
    for channel in 0...8 do
      ch_selection = channel.to_s(2).rjust(3,"0")
      _, center, last = spi.write(["0b0000011#{ch_selection[0]}".to_i(2), "0b#{ch_selection[1]}#{ch_selection[2]}000000".to_i(2), 0b00000000])
      center = center & 0b00001111
      center = center << 8
      adc_value =  center + last
      voltage = adc_value / RESOLUTION_AT_12BIT * V_REF
      converted_string = ""
      case channel
      when 0, 1 then
          temp = (voltage-LM60_OFFSET_V)/LM60_COEFFICIENT_V_PER_DEG
          converted_string = "- #{TEMPERATURE_LABEL[channel]} Temperature #{"%.2f"%temp} degC"
      when 2, 3, 4 then
          case channel
          when 2 then
              current_mA = voltage * 1000
          when 3 then
              current_mA = voltage * 500
          when 4 then 
              current_mA = voltage * 1000
          end
          converted_string = "- #{"%4s"%CURRENT_VOLTAGE_RAIL[channel]} Current #{"%.1f"%current_mA} mA"
      when 5, 6, 7 then
          converted_string = "- #{SLOWADC_LABEL[channel]}"
      end
      puts "#{channel}: Raw #{"%4d"%adc_value} ch #{"%.3f"%voltage} V #{converted_string}"
    end
end
