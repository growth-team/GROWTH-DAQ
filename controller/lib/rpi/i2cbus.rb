=begin
Copyright (c) 2016, Javier Valencia. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=end

require "rpi/validations"

module RPi
	class I2CBus
		include Validations
		
		IOCTL_I2C_SLAVE = 0x0703
		
		def initialize(id)
			validate_type(id, Fixnum)
			validate_range(id, 0..4)
			validate_file("/dev/i2c-#{id}")
			
			@device = "/dev/i2c-#{id}"
		end
		
		def read(address:, register:, length:)
			validate_type(address, Fixnum)
			validate_range(address, 0..127)
			validate_type(register, Fixnum)
			validate_range(register, 0..Infinity)
			validate_type(length, Fixnum)
			validate_range(length, 1..Infinity)
			
			File.open(@device, "r+") do |f|
				f.flock(File::LOCK_EX)
				f.ioctl(IOCTL_I2C_SLAVE, address)
				
				f.syswrite([register].pack("C"))
				str = f.sysread(length)
				
				length == 1 ? str.bytes[0] : str
			end
		end
		
		def write(address:, register:, bytes:)
			validate_type(address, Fixnum)
			validate_range(address, 0..127)
			validate_type(register, Fixnum)
			validate_range(register, 0..Infinity)
			validate_type(bytes, [String, Array, Fixnum])
			
			File.open(@device, "r+") do |f|
				f.flock(File::LOCK_EX)
				f.ioctl(IOCTL_I2C_SLAVE, address)
				
				case bytes
					when String
						f.syswrite([register].pack("C") + bytes)
					when Array
						f.syswrite([register].pack("C") + bytes.pack("C*"))
					when Fixnum
						f.syswrite([register].pack("C") + [bytes].pack("C"))
				end
			end
		end
	end
end