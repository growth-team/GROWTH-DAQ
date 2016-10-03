require "json"
require "growth_controller/constans"
require "growth_controller/logger"
require "growth_controller/config"

module GROWTH

  class Keys < LoggingInterface
  	def initialize(logger: nil)
      # Construct a logger instance
      set_logger(logger, module_name: "config")

      # Load configuration
      @growth_config = GROWTH::Config.new(logger: logger)
      @detector_id = @growth_config.detector_id

      # Initialize instance variables
      @device_id = nil
      @primary_endpoint = nil
      @api_key = nil

      # Load key file
      load_m2x_keys()
  	end

    attr_accessor :device_id, :primary_endpoint, :api_key

    def m2x_key_set()
      if(@device_id!=nil and @api_key!=nil)then
        return true
      else
        return false
      end
    end

    # Load M2X keys
    private
    def load_m2x_keys()      
      # Get M2X keys
      if(GROWTH_KEYS_FILE=="" or !File.exist?(GROWTH_KEYS_FILE))then
        log_warn "GROWTH-Keys file not found. Check if GROWTH_KEYS_FILE environment variable is set."
        log_warn "Telemetry will not be sent to M2X."
        return
      end

      # Load the file
      @key_json = JSON.load(GROWTH_KEYS_FILE)
      if(@key_json[@detector_id]==nil)then
        log_fatal "M2X device ID and API key for #{@detector_id} is not defined in #{GROWTH_KEYS_FILE}"
        exit(1)
      end

      @device_id = @key_json[@detector_id]["m2x"]["device-id"]
      @primary_endpoint = "/devices/#{@device_id}"
      @api_key = @key_json[@detector_id]["m2x"]["primary-api-key"]
    end



  end

end