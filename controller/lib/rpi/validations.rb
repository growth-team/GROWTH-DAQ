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

module RPi
	module Validations
		Infinity = Float::INFINITY
		
		def validate(condition, fail_message)
			abort(fail_message) unless condition
		end
		
		def validate_range(variable, range)
			validate(range.member?(variable), "Value '#{variable}' not in range '#{range}'.")
		end
		
		def validate_type(variable, type)
			if type.is_a?(Array)
				validate(type.member?(variable.class), "Invalid type '#{variable.class}'. Expected: '#{type}'.")
			else
				validate(variable.is_a?(type), "Invalid type '#{variable.class}'. Expected: '#{type}'.")
			end
		end
		
		def validate_file(variable)
			validate(File.exists?(variable), "File '#{variable}' not found.")
		end
	end
end