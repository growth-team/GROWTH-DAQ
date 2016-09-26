require "rbczmq"
require "json"
require "yaml"
require "socket"
require "growth_controller/controller_module"

module GROWTH

class ControllerModuleDisplay < ControllerModule
	# TCP port number of display server
	DisplayServerPortNumber = 10010

	def initialize(name, zmq_context, logger: nil)
		super(name, logger: logger)
		define_command("stop")
		define_command("clear")
		define_command("display")
		define_command("connected")

		@context = zmq_context
		connect()
	end

	private
	def connect()
		log_info("Connecting to display server...")
		@requester = @context.socket(ZMQ::REQ)
		begin
			@requester.connect("tcp://localhost:#{DisplayServerPortNumber}")
			@requester.rcvtimeo = 1000
			@requester.sndtimeo = 1000
			log_info("Connected to display server")
		rescue
			log_error("Connection failed. It seems Controller is not running")
			@requester = nil
		end
	end

	private
	def send_command(hash)
		# Connect if necessary
		if(@requester==nil)then
			connect()
		end
		if(@requester==nil)then
			log_warn("Continue with being disconnected from display server")
			return {status: "error", message: "Could not connect to display server"}
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
			log_debug(reply_message)
			return JSON.parse(reply_message)
		rescue => e
			@requester.close
			@requester = nil
			log_warn("receive_reply() returning error (#{e})")
			return {status: "error", message: "ZeroMQ receive failed (#{e})"}
		end		
	end

	#---------------------------------------------
	# Implemented commands
	#---------------------------------------------
	# Stops the display server
	def stop(option_json)
		pid = 0
		message = "failed to stop display server"
		str = `pgrep -f growth_display_server.py`.strip()
		if (str.length!=0) then
			pid = str.to_i
			if(pid!=0)then
				`kill #{pid}`
				message = "Stopped display server #{pid}"
				log_warn("Display server stopped (#{pid})")
			end
		end
		return {status: "ok", message: message, pid: pid}
	end
	
	# Clears display
	def clear(option_json)
		log_debug("clear command invoked")
		return send_command({command: "clear"})
	end

	# Displays string
	# option_json should contain "message" entry.
	def display(option_json)
		log_debug("display command invoked")
		return send_command({command: "display", option:option_json})
	end

	# Returns the status of connection to the
	# display server.
	def connected(option_json)
		if(@requester==nil)then
			connect()
		end
		if(@requester!=nil)then
			return {status: "ok", message: "true"}
		else
			return {status: "ok", message: "false"}
		end
	end

	# Ping the server
	def ping(option_json)
		return send_command({command: "ping"})
	end
end

end
