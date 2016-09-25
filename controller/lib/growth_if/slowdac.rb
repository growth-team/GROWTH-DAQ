#!/usr/bin/env ruby

require "json"
require "pi_piper"
require "logger"

module GROWTH

  # Sets Slow DAC output values.
  class SlowDAC

    # Gain setting: 0=gain 2x, 1=gain 1x
    NGAIN = 0

    # Shutdown setting: 0=shutdown 1=output enabled
    NSHDN = 1

    # Logger instance used by this class
    @@logger = Logger.new(STDOUT)

    def self.set_logger(logger)
      if(logger!=nil)then
        @@logger = logger
      end
    end

    # Sets output value in mV. DAC will output specified voltage soon
    # after this method is invoked. When one or more of specified
    # parameters are invalid, an error message will be output to
    # STDOUT via logger.
    #
    # @param ch output channel (0 or 1)
    # @param voltage_in_mV output voltage in mV (0-3300 mV)
    # @return [Boolean] true if successfully set, false if not
    def self.set_output(ch, voltage_in_mV)
      # Check parameters
      if(ch<0 or ch>1)then
        @@logger.error("[SlowDAC] Output channel should 0 or 1 (#{ch} provided)")
        return false
      end

      if(voltage_in_mV<0 or voltage_in_mV>3300)then
        @@logger.error("[SlowDAC] Output voltage should be >=0 and <=3300 (#{voltage_in_mV} provided)")
        return false
      end

      # Construct SPI instance
      begin
        PiPiper::Spi.begin(PiPiper::Spi::CHIP_SELECT_0) do |spi|
          header = "0b#{ch}0#{NGAIN}#{NSHDN}".to_i(2) << 12
          register_value = header + voltage_in_mV
          @@logger.info("[SlowDAC] Setting Ch. #{ch} voltage at #{"%.3f"%(voltage_in_mV/1000.0)} V")
          spi.write [ register_value/0x100, register_value%0x100 ]
        end
        return true
      rescue => e
        @@logger.error("[SlowDAC] Failed to set DAC output voltage (#{e})")
        return false
      end
    end
  end
end

