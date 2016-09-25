require "pi_piper"
require "json"
require "logger"

module GROWTH
	class GPIO
		# Construct Pin instances
		@@hv  = [PiPiper::Pin.new(:pin => 27, :direction => :out), # Rev1.1 HV0
		            PiPiper::Pin.new(:pin => 22, :direction => :out)] # Rev1.1 HV1
		@@slide_switch = PiPiper::Pin.new(:pin => 21, :direction => :in)

			#~ LED = [PiPiper::Pin.new(:pin => 26, :direction => :out),
						#~ PiPiper::Pin.new(:pin => 20, :direction => :out)]

		# Logger instance
		@@logger = Logger.new(STDOUT)

		def self.set_logger(logger)
			if(logger!=nil)then
				@@logger = logger
			end
		end

		#~ # Sets LED status.
		#~ # @param ch 0 or 1
		#~ # @param status :on or :off
		#~ # @return true if successfully switched, false if error
		#~ def self.set_led(ch, status=:on)
#~ 
			#~ # Check channel
			#~ if(ch<0 or ch>1)then
				#~ @@logger.error("[GPIO] invalid LED channel (0 or 1; #{ch} provided)")
				#~ return false
			#~ end
#~ 
			#~ # Set LED output status
			#~ if(status==:on)then
				#~ @@logger.debug("[GPIO] Turne on LED #{ch}")
				#~ LED[ch].on
			#~ elsif(status==:off)then
				#~ @@logger.debug("[GPIO] Turne off LED #{ch}")
				#~ LED[ch].off
			#~ else
				#~ @@logger.error("[GPIO] invalid LED status (:on or :off; #{status} provided)")
				#~ return false
			#~ end
#~ 
			#~ return true
		#~ end

		# Sets HV output status.
		# @param ch 0 or 1
		# @param status :on or :off
		# @return true if successfully switched, false if error
		def self.set_hv(ch, status=:on)
			# Check channel
			if(ch<0 or ch>1)then
				@@logger.error("[GPIO] invalid HV channel (0 or 1; #{ch} provided)")
				return false
			end

			# Set HV output status
			if(status==:on)then
				@@logger.warn("[GPIO] HK Ch.#{ch} ON")
				@@hv[ch].on
			elsif(status==:off)then
				@@logger.warn("[GPIO] HK Ch.#{ch} OFF")
				@@hv[ch].off
			else
				@@logger.error("[GPIO] invalid HV status (:on or :off; #{status} provided)")
				return false
			end

			return true
		end

		# Checks the HV output status
		# @param ch HV channel 0 or 1
		# @return true if on, falsei if off
		def self.is_hv_on?(ch=0)
			# Check channel
			if(ch<0 or ch>1)then
				@@logger.error("[GPIO] invalid HV channel (0 or 1; #{ch} provided)")
				return false
			end
			return @@hv[ch].on?
		end

		# Checks the slide switch status
		# @return true if on, falsei if off
		def self.is_slide_switch_on?()
			@@slide_switch.read()
			return @@slide_switch.on?()
		end
	end
end
