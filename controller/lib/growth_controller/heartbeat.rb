require "growth_controller/config"
require "growth_if/gpio"

module GROWTH

class ControllerModuleHeartBeat < ControllerModule
	
	DEFAULT_WAIT_DURATION_SEC = 10

	def initialize(name, logger: nil)
		super(name, logger: logger)
		GPIO.set_logger(logger)

		@growth_config = GROWTH::Config.new(logger: logger)
		define_command("get_heartbeat_value")
		define_command("get_wait_duration_sec")
		define_command("set_wait_duration_sec")

		set_heartbeat_value(0)
		@wait_duration_sec = DEFAULT_WAIT_DURATION_SEC
		@stopped = true
	end

	attr_accessor :wait_duration_sec

	def get_heartbeat_value(option_json)
		return {
			status: "ok", heartbeat_value: @heartbeat_value
		}
	end

	def get_wait_duration_sec(option_json)
		return {
			status: "ok", wait_duration_sec: @wait_duration_sec
		}
	end

	def set_wait_duration_sec(option_json)
		# Check option
		if(option_json["wait_duration_sec"]==nil)then
			return {status: "error", message: "heartbeat.set_wait_duration_sec command requires 'wait_duration_sec' option"}
		end
		# Parse channel option and value_in_mV option
		wait_duration_sec = option_json["wait_duration_sec"].to_i
		if wait_duration_sec < 0 or 1800 < wait_duration_sec then
			return {status: "error", message: "Invalid wait duration (#{wait_duration_sec} sec; should be 0 < duration_sec < 1800)"}
		end
		@wait_duration_sec = wait_duration_sec
		return {
			status: "ok"
		}
	end

	def set_heartbeat_value(value)
		@heartbeat_value = value
		@heartbeat_value %= 4
		GPIO.set_heartbeat(@heartbeat_value)
		log_debug("Setting heartbeat value #{@heartbeat_value}")
	end

	def increment()
		@heartbeat_value += 1
		set_heartbeat_value(@heartbeat_value)
	end

	def start_increment()
		@stopped = false
		log_info("Heartbeat auto increment started")
		while(!@stopped)
			sleep(@wait_duration_sec)
			increment()
		end
		log_info("Heartbeat auto increment stopped")
	end
	
	def stop()
		@stopped = true
	end
end

end
