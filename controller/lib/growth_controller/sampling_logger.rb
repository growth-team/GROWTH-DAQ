require "growth_controller/logger"

module GROWTH

class SamplingLogger

  FILE_SWITCHING_PERIOD_SEC = 86400
  
  def initialize(logger_name, sampling_period_sec, file_name_prefix)
    @logger = GROWTH.logger(ARGV, logger_name)
    @sampling_period_sec = sampling_period_sec
    @file_name_prefix = file_name_prefix
    @output_file_name = ""
    @output_file = nil
    @file_creation_time_unix_time = 0
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
    return now.strftime("#{@file_name_prefix}_%Y%m%d_%H%M%S.text")
  end

end

end
