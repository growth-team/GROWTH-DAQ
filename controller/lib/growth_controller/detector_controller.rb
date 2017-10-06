require "rbczmq"
require "json"
require "yaml"
require "socket"
require "logger"

require "growth_controller/constants"
require "growth_controller/logger"
require "growth_controller/config"
require "growth_controller/controller_module"
require "growth_controller/detector"
require "growth_controller/hv"
require "growth_controller/display"
require "growth_controller/hk"
require "growth_controller/daq"
require "growth_controller/heartbeat"

module GROWTH

  class DetectorController < LoggingInterface

    DETECTOR_CONTROLLER_ZMQ_PORT_NUMBER   = 10000
    LOG_MESSAGE_CONTROLLER_STARTED = "controller daemon started"
    LOG_MESSAGE_CONTROLLER_STOPPED = "controller daemon stopped"

    # Constructs an instance, and then start ZeroMQ server
    def initialize(logger: nil)
      set_logger(logger, module_name:"controller")

      # Load configuration
      @growth_config = GROWTH::Config.new(logger: logger)
      @detector_id = @growth_config.detector_id

      # Check constant definitions
      check_constants()

      # Initialize instance variables
      @stopped = false
      @controller_modules = {}

      # Start ZeroMQ server
      @context = ZMQ::Context.new
      @socket  = @context.socket(ZMQ::REP)
      @socket.bind("tcp://*:#{DETECTOR_CONTROLLER_ZMQ_PORT_NUMBER}")
      log_info("Controller started")

      # Add controller modules
      add_controller_module(ControllerModuleDetector.new("det", logger: logger))
      add_controller_module(ControllerModuleHV.new("hv", logger: logger))
      add_controller_module(ControllerModuleDisplay.new("disp", @context, logger: logger))
      add_controller_module(ControllerModuleHK.new("hk", logger: logger))
      add_controller_module(ControllerModuleDAQ.new("daq", @context, logger: logger))
      add_controller_module(ControllerModuleHeartBeat.new("heartbeat", @context, logger: logger))

      # Send a log message to M2X
      send_log_to_m2x(LOG_MESSAGE_CONTROLLER_STARTED)
    end
    
    def config()
        return @growth_config
    end

    private
    def check_constants()
      {
       GROWTH_REPOSITORY: GROWTH_REPOSITORY
      }.each(){|label,file|
        if(file==nil or file=="")then
          log_fatal("#{label} environment variable not set")
          exit(-1)
        end
        if(!File.exists?(file))then
          log_fatal("#{label} #{file} not found")
          exit(-1)
        end
      }
    end

    # Process received JSON command
    def process_json_command(json)
      # Parse message
      subsystem="controller"
      command=json["command"].strip
      option={}
      if(json["option"]!=nil)then
        option=json["option"]
      end
      if(command.include?("."))then
        subsystem = command.split(".")[0]
        command = command.split(".")[1]
      end
      log_info "Subsystem: #{subsystem} Command: #{command} Option: #{option}"

      # Controller commands
      if(subsystem=="controller" and command=="stop")then
        @stopped = true
        return {status: "ok", messaeg: "Controller has been stopped"}.to_json
      end

      # Subsystem commands
      if(@controller_modules[subsystem]!=nil)then
        controller_module = @controller_modules[subsystem]
        if(controller_module.has_command(command))then
          reply = controller_module.send(command, option)
          if (reply==nil or !reply.instance_of?(Hash)) then
			log_warn("Subsystem '#{subsystem}' returned invalid return value (not Hash)")
			log_warn("Returned value = #{reply}")
			reply = {}
          end
          reply["subsystem"] = subsystem
          return reply.to_json
        end
      else
        subsystem_not_found_message = {sender: "detector_controller", status: "error", message: "Subsystem '#{subsystem}' not found"}.to_json
        return subsystem_not_found_message
      end

      # If command not found, return error message
      command_not_found_message = {status: "error", message: "Command '#{subsystem}.#{command}' not found"}.to_json
      return command_not_found_message
    end

    # Utility function to convert sting to JSON
    def to_json_object(str)
      return JSON.parse(str)
    end

    # Main loop
    public
    def run()
      # Start heartbeat thread
      hearbeat_module = @controller_modules["heartbeat"]
      thread_heartbeat = Thread.new do
        log_info("Starting heartbeat thread")
        hearbeat_module.start_increment()
      end
      while(!@stopped)
        # Wait for a JSON message from a client
        message = @socket.recv()
        log_info "Receive status: message: #{message.inspect}"

        # Process JSON commands
        replyMessage = "{}"
        if(message!="")then
          replyMessage = process_json_command(to_json_object(message))
        end
        @socket.send(replyMessage)
      end
      # Stop heartbeat thread
      log_info("Stopping heartbeat thread")
      hearbeat_module.stop()
      thread_heartbeat.join()
      log_info("Heartbeat thread stopped")
      # Finalize
      log_info "Controller stopped"
    end

    # Send log to M2X
    def send_log_to_m2x(str)
      if(!@use_m2x)then
        return
      end

      stream_id = "detector-status"
      url = "http://api-m2x.att.com/v2/devices/#{@device_id}/streams/#{stream_id}/value"
      json = { value: str }.to_json
      log_debug "URL = #{url}"
      log_debug "json = #{json}"
      log_debug `curl -i -X PUT #{url} -H "X-M2X-KEY: #{@primary_api_key}" -H "Content-Type: application/json" -d "#{json.gsub('"','\"')}"`
    end

    # Add ControllerModule instance
    def add_controller_module(controller_module)
      controller_module.controller = self
      @controller_modules[controller_module.name] = controller_module
      log_info "ControllerModule #{controller_module.name} added"
    end

    # Getter/setter
    attr_accessor :detector_id
  end
end
