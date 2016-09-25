require "growth_controller/config"
require "growth_if/slowdac"
require "growth_if/gpio"

module GROWTH

class ControllerModuleHV < ControllerModule
	HV_CHANNEL_LOWER = 0
	HV_CHANNEL_UPPER = 1

	HV_VALUE_IN_MILLI_VOLT_LOWER = 0
	HV_VALUE_IN_MILLI_VOLT_UPPER = 3300

	def initialize(name, logger: nil)
		super(name, logger: logger)
		SlowDAC.set_logger(logger)
		GPIO.set_logger(logger)

		@growth_config = GROWTH::Config.new(logger: logger)
		define_command("status")
		define_command("set")
		define_command("on")
		define_command("off")
		define_command("off_all")
		@on_off_status = []
		@hv_value = []
		# Initialize
		for i in HV_CHANNEL_LOWER..HV_CHANNEL_UPPER
			@on_off_status << "off"
			@hv_value << 0
			off({"ch"=>i})
		end
	end

	def is_fy2015()
		if(@growth_config.detector_id.include?("growth-fy2015"))then
			return true
		else
			return false
		end
	end
	
	def status(option_json)
		reply = {status: "ok"}
		for ch in HV_CHANNEL_LOWER..HV_CHANNEL_UPPER
			reply[ch.to_s] = {status: @on_off_status[ch], value_in_mV: @hv_value[ch]}
		end
		return reply
	end
	
	def set(option_json)
		# Check option
		if(option_json["ch"]==nil)then
			return {status: "error", message: "hv.set command requires channel option"}
		end
		# Parse channel option and value_in_mV option
		ch = option_json["ch"].to_i
		value_in_mV = 0
		value_in_HV_V = 0
		if(ch<HV_CHANNEL_LOWER or ch>HV_CHANNEL_UPPER)then
			return {status: "error", message: "Invalid channel index #{ch}"}
		end
		# Set HV DAC value
		if(is_fy2015())then
			# FY2015 does not support HV DAC
		else
			# FY2016 onwards HV on command
			if(option_json["value_in_mV"]==nil)then
				return {status: "error", message: "hv.set command requires DAC output voltage in mV"}
			end
			# Check value range
			value_in_mV = option_json["value_in_mV"]
			if(value_in_mV<HV_VALUE_IN_MILLI_VOLT_LOWER or value_in_mV>HV_VALUE_IN_MILLI_VOLT_UPPER)then
				return {status: "error", message: "hv.set command received invalid 'voltage in mV' of #{value_in_mV}"}
			end
			if(@growth_config.has_hv_limit and @growth_config.has_hv_conversion)then
				# Check HV limit in HV Volt
				value_in_HV_V = @growth_config.to_hv_voltage(ch, value_in_mV)
				hv_limit_hv_V = @growth_config.get_hv_limit(ch)
				if (value_in_HV_V==-999 or value_in_HV_V>hv_limit_hv_V) then
					return {status: "error", message: "hv.set command failed exceeding the limit (#{hv_limit_hv_V})"}
				end
			end
			# Set HV value
			if(!GROWTH::SlowDAC.set_output(ch, value_in_mV))then
				return {status: "error", message: "hv.set command failed to set DAC output voltage (SPI error?)"}
			end
			# Update internal state
			@hv_value[ch] = value_in_mV.to_f
		end
		# Return message
		return { #
			status: "ok", message:"hv.set executed", ch:option_json["ch"].to_i, #
			value_in_mV:@hv_value[ch], value_in_HV_V: value_in_HV_V}
	end

	def on(option_json)
		# Check option
		if(option_json["ch"]==nil)then
			return {status: "error", message: "hv.on command requires channel option"}
		end
		# Parse channel option and value_in_mV option
		ch = option_json["ch"].to_i
		if(ch<HV_CHANNEL_LOWER or ch>HV_CHANNEL_UPPER)then
			return {status: "error", message: "Invalid channel index #{ch}"}
		end
		# Turn on HV
		if(is_fy2015())then
			# FY2015 HV on command
			`hv_on`
		else
			# FY2016 onwards HV on command
			# Turn on HV output
			GROWTH::GPIO.set_hv(ch,:on)
			# Update internal state
			@on_off_status[ch] = "on"
		end
		# Return message
		return { #
			status: "ok", message:"hv.on executed", ch:option_json["ch"].to_i, #
			value_in_mV:@hv_value[ch]}
	end

	def off(option_json)
		# Check option
		if(option_json["ch"]==nil)then
			return {status: "error", message: "hv.off command requires channel option"}
		end
		# Parse channel option
		ch = option_json["ch"].to_i
		if(ch<HV_CHANNEL_LOWER or ch>HV_CHANNEL_UPPER)then
			return {status: "error", message: "Invalid channel index #{ch}"}
		end
		# Turn off HV
		if(is_fy2015())then
			# FY2015 HV off command
			`hv_off`
		else
			# FY2016 onwards HV off command
			# Set HV value (0 mV)
			GROWTH::SlowDAC.set_output(ch, 0)
			# Turn off HV output
			GROWTH::GPIO.set_hv(ch,:off)
			# Update internal state
			@on_off_status[ch] = "off"
			@hv_value[ch] = 0
		end
		# Return message
		return {status: "ok", message:"hv.off executed", ch:option_json["ch"].to_i}
	end

	def off_all(option_json)
		for ch in HV_CHANNEL_LOWER..HV_CHANNEL_UPPER
			off({"ch"=>ch})
		end
		# Return message
		return {status: "ok", message:"hv.off_all executed"}
	end
end

end
