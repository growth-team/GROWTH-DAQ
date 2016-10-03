#!/usr/bin/env ruby

require "growth_controller/logger"
require "growth_controller/config"
require "growth_controller/keys"
require "growth_controller/console_modules"
require "json"

# Transmit telemtry data to M2X and/or other cloud-base storage.

class TelemetryTransmitter

  SAMPLING_PERIOD_SEC = 300
  
  def initialize()
    @logger = GROWTH.logger(ARGV, "growth_hk_logger")
    # @growth_config = GROWTH::Config.new(logger: @logger)
    @growth_keys = GROWTH::Keys.new(logger: @logger)

    @hv = GROWTH::ConsoleModuleHV.new("hv", logger: @logger)  
    @hk = GROWTH::ConsoleModuleHK.new("hk", logger: @logger)
    @daq = GROWTH::ConsoleModuleDAQ.new("daq", logger: @logger)

    # M2X-related instances
    @client = nil
    @device = nil
  end
  
  def run()
    while(true)
      @logger.info("Sending telemetry to M2X")
      # Read status then send to M2X
      begin
        hk_data = @hk.read()
        send_to_m2x_hk(hk_data)
      rescue => e
        @logger.error("HK send failed")
      end
      begin
        daq_status = @daq.status()
        send_to_m2x_daq(daq_status)
      rescue => e
        @logger.error("DAQ status send failed")
      end
      begin
        hv_status = @hv.status()
        send_to_m2x_hv(hv_status)
      rescue => e
        @logger.error("HV status send failed")
      end
      # Wait until next sampling
      sleep(SAMPLING_PERIOD_SEC)
    end
  end

  private
  def send_to_m2x_hk(hk)
    @logger.info("Sending HK telemetry to M2X")
    begin
      # temperature-fpga
      if(hk["status"]=="ok" and hk["hk"]["slow_adc"]!=nil)then
        send_to_m2x_single("temperature-fpga", hk["hk"]["slow_adc"]["0"]["converted_value"])
      else
        @logger.warn("Skipping temperature-fpga because HK data contains error")
      end

      # temperature-dcdc
      if(hk["status"]=="ok" and hk["hk"]["slow_adc"]!=nil)then
        send_to_m2x_single("temperature-dcdc", hk["hk"]["slow_adc"]["1"]["converted_value"])
      else
        @logger.warn("Skipping temperature-dcdc because HK data contains error")
      end

      # temperature-daughter
      if(hk["status"]=="ok" and hk["hk"]["bme280"]["temperature"]!=nil)then
        temperature = hk["hk"]["bme280"]["temperature"]["value"]
        send_to_m2x_single("temperature-daughter", temperature)
      else
        @logger.warn("Skipping temperature-daughter because BME280 data is empty")
      end

      # pressure
      if(hk["status"]=="ok" and hk["hk"]["bme280"]["pressure"]!=nil)then
        pressure = hk["hk"]["bme280"]["pressure"]["value"]
        send_to_m2x_single("pressure", pressure)
      else
        @logger.warn("Skipping pressure because BME280 data is empty")
      end

      # humidity
      if(hk["status"]=="ok" and hk["hk"]["bme280"]["humidity"]!=nil)then
        humidity = hk["hk"]["bme280"]["humidity"]["value"]
        send_to_m2x_single("humidity", humidity)
      else
        @logger.warn("Skipping humidity because BME280 data is empty")
      end

    rescue => e
      @logger.error("Failed to send HK telemetry (#{e})")
      raise e
    end
  end

  private
  def send_to_m2x_daq(daq_status)
    @logger.info("Sending DAQ telemetry to M2X")
    begin
      # temperature-fpga
      if(daq_status["status"]=="ok" and daq_status["daqStatus"]!=nil)then
        if(daq_status["daqStatus"]=="Running")then
          send_to_m2x_single("daq-status", 1)
        else
          send_to_m2x_single("daq-status", 0)
        end
      else
        @logger.warn("Skipping DAQ status because HK data contains error")
      end
    rescue => e
      @logger.error("Failed to send DAQ telemetry (#{e})")
      raise e
    end
  end

  private
  def send_to_m2x_hv(hv)
    @logger.info("Sending HV telemetry to M2X")
    begin
      # hv-status-2bit
      if(hv["status"]=="ok")then
        status_ch0 = 0
        status_ch1 = 0
        if(hv["0"]["status"]=="on")then
          status_ch0 = 1
        end
        if(hv["1"]["status"]=="on")then
          status_ch1 = 1
        end
        status = status_ch1 * 2 + status_ch0
        send_to_m2x_single("hv-status-2bit", status)
      else
        @logger.warn("Skipping HV status because HK data contains error")
      end
    rescue => e
      @logger.error("Failed to send HV telemetry (#{e})")
      raise e
    end
  end

  private
  def send_to_m2x_single(stream_id, value)
    begin
      if(@client==nil or @device==nil)then
        connect_to_m2x()
      end
      @device.
    rescue => e
      @logger.error("Stream #{stream_id} value #{value} could not be sent")
      reset_m2x()
    end
  end

  STREAM_DEFINITIONS = [
    "temperature-fpga" => {unit: { label: "celsius", symbol: "degC" } },
    "temperature-dcdc" => {unit: { label: "celsius", symbol: "degC" } },
    "temperature-daughter" => {unit: { label: "celsius", symbol: "degC" } },
    "pressure"         => {unit: { label: "pressure", symbol: "hPa" } },
    "humidity"         => {unit: { label: "humidity", symbol: "%" } },
    "daq-status"       => {unit: { label: "Paused/Running", symbol: "" } },
    "hv-status-2bit"    => {unit: { label: "Off/On", symbol: "" } },
  ]

  private
  def reset_m2x()
    @client = nil
    @device = nil
  end

  def connect_to_m2x()
    begin
      @logger.info("Connecting to M2X")
      @client = M2X::Client.new(@growth_keys.primary_api_key)
      @logger.info("M2X client instantiated")
      @device = client.device(@growth_keys.device_id)
      @logger.info("M2X device #{@device.name} instantiated")
      check_stream_existence()
    rescue => e
      @logger.error("Could not connect to M2X (#{e})")
      reset_m2x()
      raise e
    end
  end

  def check_stream_existence()
    begin
      STREAM_DEFINITIONS.each(){|stream_id, map|
        if(@device.streams[stream_id]==nil)then
          create_stream(stream_id, map[:units])
        else
          @logger.info("Stream #{stream_id} already exists")
        end
      }
    rescue => e
      @logger.error("Could not check existence of stream '#{stream_id}' (#{e})")
      raise e
    end
  end

  def create_stream(stream_id, units_definition)
    begin
      @logger.info("Creating stream #{stream_id}")
      @device.create_stream(stream_id, unit: units_definition)
      @logger.info("Stream #{stream_id} successfully created")
    rescue => e
      @logger.error("Stream creation failed")
      raise e
    end
  end

end

telemetry_transmitter = TelemetryTransmitter.new()
telemetry_transmitter.run()
