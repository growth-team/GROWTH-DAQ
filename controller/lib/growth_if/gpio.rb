require "rpi_gpio"
require "json"
require "logger"

module GROWTH
	class GPIO
		# PiPiper::Pin 互換のラッパークラス (rpi_gpio ベース)
		class Pin
			attr_reader :pin, :direction

			def initialize(options = {})
				@pin = options[:pin]
				@direction = options[:direction]

				RPi::GPIO.set_numbering :bcm
				RPi::GPIO.set_warnings(false)

				case @direction
				when :out
					RPi::GPIO.setup(@pin, as: :output)
				when :in
					RPi::GPIO.setup(@pin, as: :input)
				end
			end

			def on
				RPi::GPIO.set_high(@pin)
			end

			def off
				RPi::GPIO.set_low(@pin)
			end

			def on?
				RPi::GPIO.high?(@pin)
			end

			def read
                                RPi::GPIO.high?(@pin) ? 1 : 0
                        end
                        #def read
                        #        RPi::GPIO.get_value(@pin)
			#end
		end

		# Construct Pin instances
		@@hv  = [Pin.new(:pin => 27, :direction => :out), # Rev1.1 HV0
		         Pin.new(:pin => 22, :direction => :out)] # Rev1.1 HV1
		@@slide_switch = Pin.new(:pin => 21, :direction => :in)

			#~ LED = [Pin.new(:pin => 26, :direction => :out),
						#~ Pin.new(:pin => 20, :direction => :out)]

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
