#!/usr/bin/env ruby

require "growth_controller/logger"
require "growth_controller/config"
require "growth_controller/console_modules"
require "json"

# Loggs HK data.

class HKLogger

  SAMPLING_PERIOD_SEC = 120
  FILE_SWITCHING_PERIOD_SEC = 86400
  
  def initialize()
    @logger = GROWTH.logger(ARGV, "growth_hk_logger")
    @growth_config = GROWTH::Config.new(logger: @logger)

    @hv = GROWTH::ConsoleModuleHV.new("hv", logger: @logger)  
    @hk = GROWTH::ConsoleModuleHK.new("hk", logger: @logger)
    @daq = GROWTH::ConsoleModuleDAQ.new("daq", logger: @logger)
    
    @output_file_name = ""
    @output_file = nil
    @file_creation_time_unix_time = 0
  end
  
  def run()
    while(true)
      switch_output_file()
      # Time stamp
      unix_time = Time.now().to_i()
      # Read HK/status from subsystems
      begin
        hk_data = @hk.read()
        daq_status = @daq.status()
        hv_status = @hv.status()
      rescue => e
        @logger.warn("Subsystem communication error (#{e})")
      end
      # Write to file if output file is opened
      if(@output_file!=nil)then
        line = { timestamp: unix_time, hk: hk_data, daq: daq_status, hv: hv_status}
        begin
          @output_file.puts line.to_json
          @output_file.flush
        rescue => e
          @logger.fatal("Failed to write to #{@output_file_name} (#{e})")
          @logger.fatal("Exiting...")
          exit 1
        end
      else
        @logger.warn("Output file is nil (should not reach here)")
      end
      # Wait until next sampling
      sleep(SAMPLING_PERIOD_SEC)
    end
  end

  def switch_output_file()
    if(@output_file==nil or should_switch_output_file())then
      if(@output_file!=nil)then
        @logger.info("Closing #{@output_file_name}")
        @output_file.close()
        try_compress()
      end
      @output_file_name = get_new_output_file_name()
      begin
        @logger.info("Current directory = #{Dir.pwd}")
        @output_file = open(@output_file_name, "w")
        @logger.info("New log file #{@output_file_name} was created")
      rescue => e
        @logger.fatal("Failed to create #{@output_file_name} (#{e})")
        exit 1
      end
    end
  end

  private
  def should_switch_output_file()
    now_unix_time = Time.now().to_i()
    if (now_unix_time - @file_creation_time_unix_time) >= FILE_SWITCHING_PERIOD_SEC then
      return true
    else
      return false
    end
  end

  def try_compress()
    if(File.exist?(@output_file_name))then
      original_size_kb = File.size(@output_file_name)/1024.0
      @logger.info("Compressing #{@output_file_name}")
      `gzip #{@output_file_name}`
      if(File.exist?(@output_file_name+".gz"))then
        compressed_size_kb = File.size(@output_file_name+".gz")/1024.0
        ratio_percent = compressed_size_kb / original_size_kb * 100
        @logger.info(
          "Successfully compressed from %.1fkB to %.1fkB (%.1f%%)" % [
           original_size_kb, compressed_size_kb, ratio_percent]
        )
      else
        @logger.warn("Failed to compress")
      end
    end
  end

  def get_new_output_file_name()
    now = Time.now()
    @file_creation_time_unix_time = now.to_i()
    return now.strftime("hk_%Y%m%d_%H%M%S.text")
  end

end

hk_logger = HKLogger.new()
hk_logger.run()
