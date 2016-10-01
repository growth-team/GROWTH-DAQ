#!/usr/bin/env ruby

require "growth_controller/logger"
require "growth_controller/config"
require "growth_controller/console_modules"
require "fileutils"

# Relocates DAQ/HK Logger output files into
# appropriate subfolders (e.g. 201610/ or 201610/hk for
# DAQ output FITS files and HK Logger output HK files).

class FileRelocator

  RELOCATION_PERIOD = 600
  GROWTH_DATA_DIR = "/home/pi/work/growth/data"

  def initialize()
    @logger = GROWTH.logger(ARGV, "growth_file_relocator")
    @growth_config = GROWTH::Config.new(logger: @logger)
    # Instantiate subsystems
    @daq = GROWTH::ConsoleModuleDAQ.new("daq", logger: @logger)
    @hk = GROWTH::ConsoleModuleHK.new("hk", logger: @logger)
    # Construct root dir to work in
    @root_dir = GROWTH_DATA_DIR + @growth_config.detector_id
    @logger.info("Started")
    @logger.info("Root dir: #{@root_dir}")
    @logger.info("Relocation period: #{RELOCATION_PERIOD} sec")
    if !Dir.exist?(@root_dir) then
      @logger.fatal("Rood dir not found. Exiting...")
      exit 1
    end
    Dir.chdir(@root_dir)
  end
  
  def run()
    while(true)
      # Check the current DAQ output file
      daq_status = @daq.status()
      daq_current_output_file_name = ""
      if(daq_status["status"]=="ok" and daq_status["outputFileName"]!=nil)then
        daq_current_output_file_name = daq_status["outputFileName"]
      end

      @logger.debug("Checking files need to be relocated")
      daq_output_files = Dir.glob("2*.fits")
      hk_logger_output_files = Dir.glob("hk_2*").sort().pop
      files = []
      files.concat(daq_output_files)
      files.concat(hk_logger_output_files)
      files.each(){ |file_name|
        if daq_current_output_file_name.include?(file_name.strip) then
          @logger.info("Skipping #{file_name} (current output file of the DAQ program)")
          continue
        end
        yyyymm = extract_yyyymm(file_name)
        destination_dir = yyyymm
        if file_name.include?("hk_") then
          destination_dir += "/hk"
        end
        @logger.debug("Processing #{file_name} (destination #{destination_dir}")
        # Create destination folder if it does not exist
        if !Dir.exist?(destination_dir) then
          @logger.info("Creating #{destination_dir}")
          FileUtils.mkdir_p(destination_dir)
          if !Dir.exist?(destination_dir) then
            @logger.fatal("Failed to create destination directory #{destination_dir}")
            @logger.fatal("Exiting...")
            exit 1
          end
        end
        # Move to the destination directory
        begin
          FileUtils.mv(file_name, destination_dir)
        rescue => e
          @logger.warn("Failed to move #{file_name} to #{destination_dir} (#{e}). Continuing...")
        end
      }
      # Wait for a while
      sleep(RELOCATION_PERIOD)
    end
  end

  private
  def extract_yyyymm(file_name)
    file_name = File.basename(file_name)
    # Cut hk_ prefix (if exists)
    file_name.gsub("hk_","")
    # Check if starts with "20"
    if(file_name[0..1]!="20")then
      return "999999"
    end
    # Extract the first 6 chars
    return file_name[0...6]
  end

end

file_relocator = FileRelocator.new()
file_relocator.run()
