require "pi_piper"
require "json"
require "logger"

#           Connected to   Pin# Pin name            ___    Pin name           Pin#   Connected to
#                           01  3.3V               |o o|   5V                  02
#          RasPi I2C SDA <= 03  GPIO 2 (SDA1, I2C) |o o|   5V                  04
#          RasPi I2C SCL <= 05  GPIO 3 (SCL1, I2C) |o o|   GND                 06
#   RasPi-Daughter GPIO0 <= 07  GPIO 4 (GPIO_GCLK) |o o|   GPIO14 (TXD0)       08 => RasPi Tx
#                           09  GND                |o o|   GPIO15 (RXD0)       10 => RasPi Rx
#   RasPi-Daughter GPIO1 <= 11  GPIO17             |o o|   GPIO18              12 => RasPi-FPGA GPIO0
#   RasPi-Daughter GPIO2 <= 13  GPIO27             |o o|   GND                 14
#   RasPi-Daughter GPIO3 <= 15  GPIO22             |o o|   GPIO23              16 => RasPi-FPGA GPIO1
#                           17  3.3V               |o o|   GPIO24              18 => RasPi-FPGA GPIO2
#         RasPi SPI MOSI <= 19  GPIO10 (SPI_MOSI)  |o o|   GND                 20
#         RasPi SPI MISO <= 21  GPIO09 (SPI_MISO)  |o o|   GPIO25              22 => RasPi-FPGA GPIO3
#         RasPi SPI CLK  <= 23  GPIO11 (SPI_CLK)   |o o|   GPIO 8 (SPI_CE0_N)  24
#                           25  GND                |o o|   GPIO 7 (SIP_CE1_N)  26
#                           27  ID_SD              |o o|   ID_SC               28
#        RasPi-CN6 GPIO0 <= 29  GPIO 5             |o o|   GND                 30
#        RasPi-CN6 GPIO1 <= 31  GPIO 6             |o o|   GPIO12              32 => RasPi-CN6 GPIO2
#        RasPi-CN6 GPIO3 <= 33  GPIO13             |o o|   GND                 34
#        RasPi-CN6 GPIO4 <= 35  GPIO19             |o o|   GPIO16              36 => RasPi-CN6 GPIO5
#             RasPi LED0 <= 37  GPIO26             |o o|   GPIO20              38 => RasPi LED1
#                           39  GND                |o o|   GPIO21              40 => Slide Switch
#                                                   ---

module GROWTH
	class GPIO
		# Construct Pin instances
		@@hv  = [PiPiper::Pin.new(:pin => 27, :direction => :out), # Rev1.1 HV0
		         PiPiper::Pin.new(:pin => 22, :direction => :out)] # Rev1.1 HV1
		@@slide_switch = PiPiper::Pin.new(:pin => 21, :direction => :in)
		# Heart beat signal connected to PDU's microcontroller
		# GPIO 4 = RasPi-Daughter GPIO0
		# GPIO17 = RasPi-Daughter GPIO1
		@@heartbeat_signals = [PiPiper::Pin.new(:pin =>  4, :direction => :out),
			                     PiPiper::Pin.new(:pin => 17, :direction => :out)]
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

		# Sets 2-bit heartbeat signal value
		# @param value 0/1/2/3
		def self.set_heartbeat(value)
			if value < 0 or value > 3 then
				@@logger.error("[GPIO] value must be 0, 1, 2, or 3. #{value} was given.")	
				return
			end
			bit0 = value % 2
			bit1 = (value % 4) / 2
			@@logger.debug("[GPIO] setting heartbeat signal (bit1, bit0) = (#{bit1}, #{bit0})")
			if bit0 == 0 then
				@@heartbeat_signals[0].off
			else
				@@heartbeat_signals[0].on
			end
			if bit1 == 0 then
				@@heartbeat_signals[1].off
			else
				@@heartbeat_signals[1].on
			end
		end
	end
end
