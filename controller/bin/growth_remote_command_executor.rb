#!/usr/bin/env ruby

require "growth_controller/logger"
require "growth_controller/config"
require "growth_controller/keys"
require "json"
require "uri"
require "net/http"
require "date"
require "open3"

# Transmit telemtry data to M2X and/or other cloud-base storage.

class RemoteCommandExecutor

  COMMAND_FETCH_PERIOD_SEC = 600
  COMMAND_TIMESTAMP_LOG_FILE_PATH = "/var/log/growth/command.timestamp.log"
  COMMAND_ORIGIN = "https://thdr.info"
  UPLOAD_COMMAND = "rsync -auv "
  UPLOAD_DESTINATION = "galileo:/work-galileo/home/growth-data/upload"
  #UPLOAD_DESTINATION = "thdr:growth-data/upload"
  UPLOAD_DIRECTORY="/media/pi/GROWTH-DATA/data"
  
  def initialize()
    @logger = GROWTH.logger(ARGV, "growth_remote_command_executor")
    @growth_config = GROWTH::Config.new(logger: @logger)

    # Timestamp of the last executed command
    @last_command_timestamp = get_last_command_timestamp()

    # Command download URI
    @uri = URI.parse("#{COMMAND_ORIGIN}/command/#{@growth_config.detector_id}/command.json")
    @logger.info "Command JSON file fetched from #{@uri}"
  end
  
  def run()
    while(true)
      commands = nil
      # Fetch command JSON file
      begin
        json = Net::HTTP.get(@uri)
        @logger.info "Command file fetched successfully"
        begin
          commands = JSON.parse(json)
        rescue => e
          @logger.error "Failed to parse command file (#{e})"
        end
      rescue => e
        @logger.error "Failed to fetch command file (#{e})"
      end

      # Parse/execute command
      parse_commands(commands) if commands!=nil

      # Wait until next fetch
      sleep(COMMAND_FETCH_PERIOD_SEC)
    end
  end

  private
  def parse_commands(commands)
    commands.each(){|command_entry|
      
      # Check the command id and the creation date/time to avoid double execution
      created_at = command_entry["created_at"]
      begin
        command_timestamp = DateTime.parse(created_at)
        if(command_timestamp <= @last_command_timestamp)then
          @logger.info "Skipping old command timestamped at #{command_timestamp}"
          next
        end
      rescue => e
        @logger.error "Could not parse #{command_entry.to_s} (#{e})"
        next
      end

      # Handle command if defined
      command = command_entry["command"]
      option = command_entry["option"]
      if(command!=nil)then
        command.strip!()
        if(command=="upload")then
          command_upload(option)
        end
        # Update the last command timestamp regardless of the execution
        # result (success/fail)
        update_last_command_timestamp(command_timestamp)
      else
        @logger.error "Undefined command #{command}"
      end
    }
  end

  private
  def get_last_command_timestamp()
    default_timestamp = DateTime.now
    if(File.exist?(COMMAND_TIMESTAMP_LOG_FILE_PATH))then
      begin
        timestamp = DateTime.parse(File.read(COMMAND_TIMESTAMP_LOG_FILE_PATH).strip)
        @logger.info "Cached last command timestamp = #{timestamp}"
        return timestamp
      rescue => e
        @logger.error "Invalid cache file at #{COMMAND_TIMESTAMP_LOG_FILE_PATH}. Deleting..."
        begin
          FileUtils.rm(COMMAND_TIMESTAMP_LOG_FILE_PATH)
          @logger.warn "Cache file #{COMMAND_TIMESTAMP_LOG_FILE_PATH} deleted"
        rescue => e
          @logger.error "Failed to delete the invalid cache file"
        end
        @logger.info "Using default last command timestamp #{default_timestamp}"
        return default_timestamp
      end
    else
      # Otherwise, return a default timestamp
      @logger.info "Using default last command timestamp #{default_timestamp}"
      return default_timestamp
    end
  end

  private
  def update_last_command_timestamp(command_timestamp)
    @logger.info "Updating the last command timestamp cache"
    @last_command_timestamp = command_timestamp
    begin
      file = open(COMMAND_TIMESTAMP_LOG_FILE_PATH, "w")
      file.puts command_timestamp
      file.close
      @logger.info "Updated to #{command_timestamp}"
    rescue => e
      @logger.fatal "Failed to write to #{COMMAND_TIMESTAMP_LOG_FILE_PATH} (#{e})"
      exit 1
    end
  end

  private
  def command_upload(option)
    @logger.info "Executing 'upload' command (option = #{option})"
    if(option["from"]==nil or option["to"]==nil)then
      @logger.error "'from' and 'to' are expected in the 'option' map"
      return
    end

    begin
      from_datetime = DateTime.parse(option["from"])
      to_datetime = DateTime.parse(option["to"])
      from_yyyymm = from_datetime.year * 100 + from_datetime.month
      to_yyyymm = to_datetime.year * 100 + to_datetime.month
      from_yyyymmddhhmmss = from_yyyymm * 1e8 + from_datetime.day * 1e6 + from_datetime.hour * 1e4 + from_datetime.minute * 1e2 + from_datetime.second
      to_yyyymmddhhmmss = to_yyyymm * 1e8 + to_datetime.day * 1e6 + to_datetime.hour * 1e4 + to_datetime.minute * 1e2 + to_datetime.second

      # Process YYYYMM directories
      #Dir.glob("#{@growth_config.daq_run_dir}/20[0-9]*/").sort().each(){|dir|
      Dir.glob("#{UPLOAD_DIRECTORY}/#{@growth_config.detector_id}/20[0-9]*/").sort().each(){|dir|
        yyyymm = dir.split("/")[-1].to_i
        if(from_yyyymm<=yyyymm and yyyymm<=to_yyyymm)then
          @logger.info "Processing #{dir}"
          # Enter if this is a directory within the specified range
          Dir.glob("#{dir}/*20*.gz").sort().each(){|gz_file_path|
            @logger.debug "Processing #{gz_file_path}"
            timestamp = extract_yyyymmdd_hhmmss_as_integer(gz_file_path)
            @logger.debug "Timestamp = #{timestamp}"
            if(from_yyyymmddhhmmss<=timestamp and timestamp<=to_yyyymmddhhmmss)then
              @logger.info "Upload #{gz_file_path}"
              upload_path = UPLOAD_DESTINATION + "/" + gz_file_path.split("/data/")[-1].gsub("//","/").gsub("/","_-_")
              command = "#{UPLOAD_COMMAND} #{gz_file_path} #{upload_path}"
              @logger.info "Executing shell command '#{"#{command}"}'"
              begin
                stdout, stderr, status = Open3.capture3(command)
                @logger.info "Status = #{status}"
                @logger.info "STDOUT = #{stdout}"
                @logger.info "STDERR = #{stderr}"
              rescue => e
                @logger.error "Error occurred during shell command execution (#{e}). This shell command is skipped."
              end
            end
          }
        end
      }
    rescue => e
      @logger.error e.to_s
      return
    end
  end

  private
  def extract_yyyymmdd_hhmmss_as_integer(str)
    if(str.match(/20[0-9]{6}_[0-9]{6}/))then
      return Regexp.last_match(0).to_s.gsub("_","").to_i
    else
      return 0
    end
  end
end

remote_command_executor = RemoteCommandExecutor.new()
remote_command_executor.run()
