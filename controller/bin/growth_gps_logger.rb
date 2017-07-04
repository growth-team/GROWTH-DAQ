#!/usr/bin/env ruby

# Loggs GPS serial output.
# This script uses FT2232 Bank B (FT232 mode) to receive
# serial output data from GPS module on the daughter board
# connected to the GROWTH FPGA/ADC board.

require "serialport"
require "time"
require "json"

require "growth_controller/config"
require "growth_controller/sampling_logger"


class GPSLogger < GROWTH::SamplingLogger

  # Constants
  FILE_SWITCHING_PERIOD_SEC = 86400
  SAMPLING_PERIOD_SEC = 120
  NUM_LINES_TO_LOG_PER_SAMPLING = 15
  UART_RECEIVE_TIMEOUT_SEC = 5
  UART_READ_WAIT_DURATION_SEC = 0.1

  def initialize(uart_device)
    super("growth_gps_logger", SAMPLING_PERIOD_SEC, "gps")
    @uart_device = uart_device

    # Construct a serial port instance
    @serial_port = nil
    begin
      @serial_port = SerialPort.new(@uart_device, 9600, 8, 1, 0)
      @serial_port.read_timeout = 200
    rescue => e
      @logger.fatal("Failed to open serial port (#{e}})")
      exit(1)
    end

  end

  def read_line()
    elapsed_time = 0
    while true do
      line = @serial_port.gets
      if line != nil then
        return line
      else
        sleep UART_READ_WAIT_DURATION_SEC
        elapsed_time += UART_READ_WAIT_DURATION_SEC
        if elapsed_time > UART_RECEIVE_TIMEOUT_SEC then
          @logger.fatal("GPS does not seem to be connected")
          exit(1)
        end
      end
    end
  end
  
  def skip_until_catch_up()
    # Read serial input until it reaches the latest data
    time_sec = [0, 0]
    unixtime = [0, 0]
    while true do
      for i in 0...2 do
        while true do
          line = read_line()
          
          if line[0...6] == "$GPRMC" then
            time_str = line.split(",")[1]
            time_sec[i] = time_str[0...2].to_i * 3600 + time_str[2...4].to_i * 60 + time_str[4..-1].to_f
            unixtime[i] = Time.now.to_i
            break
          end
        end
      end

      delta_gps_time = time_sec[1] - time_sec[0]
      delta_unixtime = unixtime[1] - unixtime[0]

      if delta_gps_time == 1 and delta_unixtime == 1 then
        # Caught up to (almost) real time
        return
      end

    end
    
  end

  def run()
    while(true)
      switch_output_file()
      # Time stamp
      unix_time = Time.now().to_i()
      # Read HK/status from subsystems
      begin
        data_to_log = []
    
        # Discard first NUM_LINES_TO_LOG_PER_SAMPLING lines
        skip_until_catch_up()

        # Receive NUM_LINES_TO_LOG_PER_SAMPLING lines and tag time stamps
        for i in 0...NUM_LINES_TO_LOG_PER_SAMPLING do
          message = read_line()
          if message == nil then
            sleep 0.1
            next
          end
          if message.length != 0 and message[0] == "$" then
            datetime = Time.now
            hash = {unixtime: datetime.to_i, datetime: datetime.iso8601, message: message}
            data_to_log << hash
          end
        end

      rescue => e
        @logger.fatal("Failed to receive GPS data (#{e})")
        @logger.fatal(e.backtrace.join("\n"))
        exit(1)
      end

      # Write to file if output file is opened
      if(@output_file!=nil)then
        begin
          data_to_log.each(){|data|
            @output_file.puts data.to_json
          }
          @output_file.flush
        rescue => e
          @logger.fatal("Failed to write to #{@output_file_name} (#{e})")
          @logger.fatal(e.backtrace.join("\n"))
          exit(1)
        end
      else
        @logger.warn("Output file is nil (should not reach here)")
      end
      # Wait until next sampling
      sleep(SAMPLING_PERIOD_SEC)
    end
  end

end


# Constants
# FT2232 Bank B (FT232 mode)
DEFAULT_UART_DEVICE = "/dev/ttyUSB1"

# Main
gps_logger = GPSLogger.new(DEFAULT_UART_DEVICE)
gps_logger.run()
