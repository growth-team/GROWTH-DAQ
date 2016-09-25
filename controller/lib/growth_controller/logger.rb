require "logger"
require "optparse"
require "fileutils"

module GROWTH

	# Constructs a logger instance by parsing command line options.
	#   --log-level : either of [debug, info, warn, error, fatal] (default = info)
	#   --log-device: stdout or file
	#   --log-dir   : log save destination (used when log device is "file")
	# Log file name will be "program_name.log".
	# Example usage:
	# sample_program --log-level warn --log-device file --log-dir /var/log/growth/
	#
	def GROWTH.logger(argv, program_name)
		option_hash = nil
		begin
			option_hash = ARGV.getopts("","log-level:info", "log-device:stdout", "log-dir:/var/log/growth")
		rescue OptionParser::InvalidOption => e
			STDERR.puts "Error: #{e}"
			exit(-1)
		end
		
		# Set log level
		levels = {
		    "debug" => Logger::DEBUG, 
		    "info" => Logger::INFO,
		    "warn" => Logger::WARN,
		    "error" => Logger::ERROR,
		    "fatal" => Logger::FATAL
		    }
		if levels[option_hash["log-level"]]==nil then
			STDERR.puts "Error: Invalid --log-level #{log_level} (should be either of debug, info, warn, error, fatal)"
			exit(-1)
		end
		log_level = levels[option_hash["log-level"]]

		# Set log destination (stdout or file)
		log_destinaiton=STDOUT
		if(option_hash["log-device"].downcase=="stdout")then
			log_destinaiton=STDOUT
		elsif(option_hash["log-device"].downcase=="file")then
			log_dir = option_hash["log-dir"]
			if(!File.exist?(log_dir))then
				begin
					FileUtils.mkdir_p(log_dir)
				rescue => e
					STDERR.puts "Error: Could not create log dir #{log_dir} (#{e})"
					exit(-1)
				end
			end
			log_file_name = "#{log_dir}/#{program_name}.log"
			log_destinaiton = log_file_name
		else
			STDERR.puts "Error: --log-device should be either of stdout or file"
			exit(-1)
		end

		# Construct logger object
		logger = nil
		if(log_destinaiton==STDOUT)then
			logger = Logger.new(STDOUT)
		else
			logger = Logger.new(log_destinaiton, "daily")
		end
		logger.level = log_level
		logger.progname = program_name
		
		return logger
	end

	# Provides logging capabilities for submodules that comprises
	# a bigger program. Logged string will have module name if
	# module_name is specified in the set_logger() method.
	class LoggingInterface
		
		def initialize()
			@logger = nil
			@module_name = ""
		end

		def set_logger(logger, module_name: "")
			if(logger==nil)then
				@logger = Logger.new(STDOUT)
			else
				@logger = logger
			end
			if(module_name!="")then
				@module_name = module_name
			end
		end

		def log_debug(str)
			if(@module_name!="")then
				@logger.debug("[#{@module_name}] "+str)
			else
				@logger.debug(str)
			end
		end

		def log_info(str)
			if(@module_name!="")then
				@logger.info("[#{@module_name}] "+str)
			else
				@logger.info(str)
			end
		end

		def log_warn(str)
			if(@module_name!="")then
				@logger.warn("[#{@module_name}] "+str)
			else
				@logger.warn(str)
			end
		end

		def log_error(str)
			if(@module_name!="")then
				@logger.error("[#{@module_name}] "+str)
			else
				@logger.error(str)
			end
		end

		def log_fatal(str)
			if(@module_name!="")then
				@logger.fatal("[#{@module_name}] "+str)
			else
				@logger.fatal(str)
			end
		end

	end
end
