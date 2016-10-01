#!/usr/bin/env ruby

require "growth_controller/logger"
require "growth_controller/config"
require "growth_controller/console_modules"
require "fileutils"

# Relocates DAQ/HK Logger output files into
# appropriate subfolders (e.g. 201610/ or 201610/hk for
# DAQ output FITS files and HK Logger output HK files).
# If a file is not gzip-compressed, this program automatically
# compresses it when moving to the subfolder.

class FileRelocator

  RELOCATION_PERIOD = 600
  GROWTH_DATA_DIR = "/home/pi/work/growth/data"

  def initialize()
    @logger = GROWTH.logger(ARGV, "growth_file_relocator")
    @logger.level = Logger::DEBUG
    @growth_config = GROWTH::Config.new(logger: @logger)
    # Instantiate subsystems
    @daq = GROWTH::ConsoleModuleDAQ.new("daq", logger: @logger)
    @hk = GROWTH::ConsoleModuleHK.new("hk", logger: @logger)
    # Construct root dir to work in
    @root_dir = GROWTH_DATA_DIR + "/"+ @growth_config.detector_id
    @logger.info("Started")
    @logger.info("Root dir: #{@root_dir}")
    @logger.info("Relocation period: #{RELOCATION_PERIOD} sec")
    if !Dir.exist?(@root_dir) then
      @logger.fatal("Root dir not found. Exiting...")
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

      @logger.info("Checking files need to be relocated")
      daq_output_files = Dir.glob("2*.fits").sort()
      hk_logger_output_files = Dir.glob("hk_2*").sort()
      @logger.info("#{daq_output_files.length} DAQ output file(s) and #{hk_logger_output_files.length} HK file(s) detected")
      # Remove the last DAQ file if communication with DAQ program was not successful
      # because the failure may be due to non-existent growth_controller and DAQ program
      # may be writing to this file.
      if(daq_status["status"]!="ok")then
        last_daq_output_file = daq_output_files.pop
        @logger.warn("Communication with DAQ program failed.")
      	@logger.warn("Skipping the latest DAQ output file #{last_daq_output_file} (this may be the current output of the DAQ program")
      end
      # Remove the last hk file because this may be the current output file
      # of HK Logger
      last_hk_logger_output_file = hk_logger_output_files.pop
      @logger.info("Skipping the latest HK output file #{last_hk_logger_output_file} (may be current output of HK Logger)")
      files = []
      files.concat(daq_output_files)
      files.concat(hk_logger_output_files)
      files.each(){ |file_name|
        if daq_current_output_file_name.include?(file_name.strip) then
          @logger.info("Skipping #{file_name} (current output file of the DAQ program)")
          next
        end
        yyyymm = extract_yyyymm(file_name)
        destination_dir = yyyymm
        if file_name.include?("hk_") then
          destination_dir += "/hk"
        end
        @logger.info("Processing #{file_name} (destination #{destination_dir})")
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
          if(!file_name.include?(".gz"))then
            gz_file_name = file_name+".gz"
            @logger.info("Compressing #{file_name}")
            original_file_size_kb = File.size(file_name) / 1024.0
            `gzip #{file_name}`
            if(File.exist?(gz_file_name))then
              gz_file_size_kb = File.size(gz_file_name) / 1024.0
              ratio_percent = gz_file_size_kb / original_file_size_kb * 100
              @logger.info(
                "Successfully compressed from %.1fkB to %.1fkB (%.1f%%)" % [
                 original_file_size_kb, gz_file_size_kb, ratio_percent
              ])
              # Now, moved file should end ".gz" file.
              file_name = gz_file_name
            else
              @logger.warn("Compression failed for #{file_name}")
            end
            FileUtils.mv(file_name, destination_dir)
          end
        rescue => e
          @logger.warn("Failed to move #{file_name} to #{destination_dir} (#{e}). Continuing...")
        end
      }
      # Wait for a while
      @logger.info("Sleeping #{RELOCATION_PERIOD} sec")
      sleep(RELOCATION_PERIOD)
    end
  end

  private
  def extract_yyyymm(file_name)
    file_name = File.basename(file_name)
    # Cut hk_ prefix (if exists)
    file_name.gsub!("hk_","")
    # Check if starts with "20"
    if(file_name[0..1]!="20")then
      @logger.warn("extract_yyyymm() returning 999999 for #{file_name}")
      return "999999"
    end
    # Extract the first 6 chars
    return file_name[0...6]
  end

end

file_relocator = FileRelocator.new()
file_relocator.run()
