require "rbczmq"
require "json"
require "growth_controller/logger"

DETECTOR_CONTROLLER_ZMQ_PORT_NUMBER   = 10000

module GROWTH

#---------------------------------------------
# ConsoleModules
#---------------------------------------------
class ConsoleModule < LoggingInterface
	@@context = ZMQ::Context.new

	def initialize(name, logger: nil)
		set_logger(logger, module_name: name)
		@requester = nil
		@name = name
	end

	def connect()
		# Open ZeroMQ client connection
		log_info("Connecting to Controller...")
		@requester = @@context.socket(ZMQ::REQ)
		begin
			@requester.connect("tcp://localhost:#{DETECTOR_CONTROLLER_ZMQ_PORT_NUMBER}")
			@requester.rcvtimeo = 1000
			@requester.sndtimeo = 1000
		rescue
			log_fatal("Connection failed. It seems Controller is not running")
			raise "Connection failed"
		end
		log_info("Connected to Controller")
	end

	def send_command(command, option_hash={})
		json_command = {command: name+"."+command, option: option_hash}.to_json
		log_debug("Sending command: #{json_command}")
		begin
			if(@requester==nil)then
				connect()
			end
			@requester.send(json_command.to_s)
		rescue => e
			log_error("ZeroMQ send failed (#{e})")
			@requester.close
			@requester = nil
			return {status: "error", message: "ZeroMQ send failed (#{e})"}
		end
		return receive_reply()
	end

	def receive_reply()
		begin
			if(@requester==nil)then
				connect()
			end
			reply_message = @requester.recv()
			return JSON.parse(reply_message)
		rescue => e
			log_error("ZeroMQ receive failed (#{e})")
			@requester.close
			@requester = nil
			return {status: "error", message: "ZeroMQ receive failed (#{e})"}
		end
	end

	attr_accessor :name
end

class ConsoleModuleDetector < ConsoleModule
	def initialize(name, logger: nil)
		super(name, logger: logger)
	end

	def id()
		return send_command("id")
	end

	def ip()
		return send_command("ip")
	end

	def hash()
		return send_command("hash")
	end

	def ping()
		return send_command("ping")
	end
end

class ConsoleModuleHV < ConsoleModule
	def initialize(name, logger: nil)
		super(name, logger: logger)
	end

	def status()
		return send_command("status")
	end

	def set(ch, value_in_mV)
		return send_command("set", {ch: ch, value_in_mV:value_in_mV})
	end
	
	def on(ch)
		return send_command("on", {ch: ch})
	end

	def off(ch)
		return send_command("off", {ch: ch})
	end

	def off_all()
		return send_command("off_all")
	end
end

class ConsoleModuleDisplay < ConsoleModule
	def initialize(name, logger: nil)
		super(name, logger: logger)
	end

	# Stops the display server
	def stop()
		return send_command("stop")
	end

	# Clears the display output
	def clear()
		return send_command("clear")
	end

	# Displays message on the screen
	def display(str)
		return send_command("display", {message: str})
	end

	# Stops display server (for test)
	def stop()
		return send_command("stop")
	end
end

class ConsoleModuleHK < ConsoleModule
	def initialize(name, logger: nil)
		super(name, logger: logger)
	end

	# Clears the display output
	def read()
		return send_command("read")
	end
end

class ConsoleModuleDAQ < ConsoleModule
	def initialize(name, logger: nil)
		super(name, logger: logger)
	end

	def ping()
		return send_command("ping")
	end

	def stop()
		return send_command("stop")
	end

	def pause()
		return send_command("pause")
	end

	def resume()
		return send_command("resume")
	end

	def status()
		return send_command("status")
	end

	def switch_output()
		return send_command("switch_output")
	end
end

end
