require "rbczmq"
require "json"
require "yaml"
require "socket"
require "growth_controller/logger"
require "growth_controller/controller_module"

module GROWTH

class ControllerModuleDAQ < ControllerModule
	# TCP port number of growth_daq ZeroMQ server
	DAQ_ZMQ_PORT_NUMBER = 10020

	def initialize(name, zmq_context, logger: nil)
		super(name, logger: logger)
		define_command("ping")
		define_command("stop")
		define_command("pause")
		define_command("resume")
		define_command("status")
		define_command("switch_output")

		@context = zmq_context
		connect()
	end

	private
	def connect()
		log_info("Connecting to DAQ ZeroMQ server...")
		@requester = @context.socket(ZMQ::REQ)
		begin
			@requester.connect("tcp://localhost:#{DAQ_ZMQ_PORT_NUMBER}")
			@requester.rcvtimeo = 1000
			@requester.sndtimeo = 1000
			log_info("Connected to DAQ ZeroMQ server")
		rescue
			log_error("Connection failed. It seems the DAQ program is not running")
			@requester = nil
		end
	end

	private
	def send_command(hash)
		# Connect if necessary
		if(@requester == nil)then
			connect()
		end
		if(@requester == nil)then
			return {status: "error", message: "Could not connect to DAQ ZeroMQ server"}
		end
		begin
			@requester.send(hash.to_json.to_s)
		rescue => e
			@requester.close
			@requester = nil
			log_warn("send_command() returning error (#{e})")
			return {status: "error", message: "ZeroMQ send failed (#{e})"}
		end
		return receive_reply()
	end

	private
	def receive_reply()
		begin
			reply_message = @requester.recv()
			return JSON.parse(reply_message)
		rescue => e
			@requester.close
			@requester = nil
			log_warn("receive_reply() returning error (#{e})")
			return {status: "error", message: "ZeroMQ communication failed (#{e}"}
		end		
	end

	#---------------------------------------------
	# Implemented commands
	#---------------------------------------------
	def ping(option_json)
		log_debug("ping command invoked")
		return send_command({command: "ping"})
	end

	def stop(option_json)
		log_debug("stop command invoked")
		return send_command({command: "stop"})
	end

	def pause(option_json)
		log_debug("pause command invoked")
		return send_command({command: "pause"})
	end

	def resume(option_json)
		log_debug("resume command invoked")
		return send_command({command: "resume"})
	end

	def status(optionJSON)
		log_debug("status command invoked")
		return send_command({command: "getStatus"})
	end

	def switch_output(option_json)
		log_debug("switch_output command invoked")
		return send_command({command: "startNewOutputFile"})
	end

end

end
