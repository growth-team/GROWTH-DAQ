require "yaml"
require "logger"
require "growth_controller/logger"

module GROWTH
  class Config < LoggingInterface

    DEFAULT_DAQ_EXPOSURE_SEC = 1800
    DEFAULT_HK_SAMPLING_PERIOD = 120

    GROWTH_DAQ_RUN_ROOT_DIR = "/home/pi/work/growth/data"

    def initialize(logger: nil)
      @growth_config_file = ""
      # Set config file path
      if(ENV["GROWTH_CONFIG_FILE"]!=nil and File.exists?(ENV["GROWTH_CONFIG_FILE"]))then
        @growth_config_file = File.expand_path(ENV["GROWTH_CONFIG_FILE"])
      elsif(File.exists?("#{ENV["HOME"]}/growth_config.yaml"))then
        @growth_config_file = File.expand_path("#{ENV["HOME"]}/growth_config.yaml")
      elsif(File.exists?("/etc/growth/growth_config.yaml"))then
        @growth_config_file = File.expand_path("/etc/growth/growth_config.yaml")
      end

      # Construct a logger instance
      set_logger(logger, module_name: "config")
      
      log_info("growth_config = #{@growth_config_file}")

      # Set default values
      @autorun_daq_exposure_sec = DEFAULT_DAQ_EXPOSURE_SEC
      @autorun_hk_sampling_period_sec = DEFAULT_HK_SAMPLING_PERIOD

      # Load configuration file
      load_config_file()

      # Set misc parameters
      @daq_run_dir = "#{GROWTH_DAQ_RUN_ROOT_DIR}/#{@detector_id}"
    end

    attr_accessor :detector_id
    attr_accessor :growth_config_file
    attr_accessor :has_hv_conversion
    attr_accessor :has_hv_default
    attr_accessor :has_temperature_limit
    attr_accessor :has_hv_limit
    attr_accessor :autorun_daq_exposure_sec
    attr_accessor :autorun_hk_sampling_period_sec
    attr_accessor :daq_run_dir
    
    def growth_config_file_path()
      return @growth_config_file
    end

    def to_hv_voltage(ch, dac_voltage_mV)
      if @has_hv_conversion then
        x = dac_voltage_mV.to_f
        if(@hv_conversion[ch]==nil)then
          log_error("growth_config does not define HV conversion equations for channel #{ch}.")
          return -999
        else
          return eval(@hv_conversion[ch])
        end
      else
        log_error("growth_config does not define conversion equations for HV DAC.")
        return 0
      end
    end

    def get_detault_hv_DAC_mV(ch)
      if @has_hv_default then
        if(@hv_default_DAC_mV[ch]==nil)then
          log_error("growth_config does not define deault HV DAC value for #{ch}.")
          return 0
        else
          return @hv_default_DAC_mV[ch]
        end
      else
        log_error("growth_config does not define default HV DAC values.")
        return 0
      end
    end

    def inside_temperature_limit(temperature_degC)
      if(@has_temperature_limit)then
        return (@limits_temperature["lower"]<=temperature_degC and temperature_degC<=@limits_temperature["upper"])
      else
        log_warn("Temperature limit not defined.")
        return true
      end
    end

    def get_temperature_limit_source()
      return @limits_temperature["source"]
    end

    def get_hv_limit(ch)
      if(@has_hv_limit)then
        if(@limit_hv[ch]==nil)then
          log_warn("HV limit not defined for Ch.#{ch}.")
          return 0
        end
        return @limit_hv[ch]
      else
        return 0
      end
    end

    private
    def load_config_file()
      # Check file presence
      if(!File.exist?(@growth_config_file))then
        log_error "#{@growth_config_file} not found"
        exit(-1)
      end

      # Load YAML configuration file
      log_info "Loading #{@growth_config_file}"
      yaml = YAML.load_file(@growth_config_file)

      # Detector ID
      if(yaml["detectorID"]==nil or yaml["detectorID"]=="")then
        log_error "detectorID not found in #{@growth_config_file}"
        exit(-1)
      else
        @detector_id = yaml["detectorID"]
        log_info "detectorID: #{@detector_id}"
      end

      # HV-related configuration
      load_hv(yaml)

      # Limits
      load_limits(yaml)
    end

    private
    def load_hv(yaml)
      @hv_conversion = {}
      @has_hv_conversion = false
      if(yaml["hv"]!=nil)then
        # DAC-HV conversion equation
        if(yaml["hv"]["conversion"]!=nil)then
          @has_hv_conversion = true
          for ch,equation in yaml["hv"]["conversion"]
            # equation should use x or DAC_mV as variable that represents DAC output
            # voltage in mV, for example "x/3300 * 1000" or "DAC_mV/3300 * 1000".
            @hv_conversion[ch.to_i] = equation
            log_info("HV Ch.#{ch} HV_V = #{equation.downcase.gsub("dac_mv","x")}")
          end
        else
          log_warn("No HV conversion equation defined")
          @has_hv_conversion = false
        end

        # Nominal HV values
        @hv_default_DAC_mV = {}
        @has_hv_default = false
        if(yaml["hv"]["default"]!=nil)then
          @has_hv_default = true
          for ch,default_DAC_mV in yaml["hv"]["default"]
            @hv_default_DAC_mV[ch.to_i] = default_DAC_mV.to_i
            log_info("HV Ch.#{ch} DAC_mV = #{default_DAC_mV.to_i}")
          end
        else
          log_warn("No HV default value defined")
          @has_hv_default = false
        end

      else
        log_error("No HV configuration")
        exit(-1)
      end
    end

    private
    def load_limits(yaml)
      @has_temperature_limit = false
      @limits_temperature = { "source" => nil, "lower" => 0, "upper" => 60 }
      @has_hv_limit = false
      @limit_hv = {0 => 700, 1 => 700}

      if(yaml["limits"]!=nil)then
        # Temperature limits
        if(yaml["limits"]["temperature"]!=nil)then
          for threshold_type,value in yaml["limits"]["temperature"]
            if(threshold_type=="upper" or threshold_type=="lower")then
              temperature_degC = value
              @limits_temperature[threshold_type] = temperature_degC
              log_info("Temprature limit #{threshold_type} = #{temperature_degC} degC")
              @has_temperature_limit = true
            elsif(threshold_type=="source")then
              # Select temperature limit source. Possible sources are:
              # - temperature-pcb (i.e. one close to DC/DC)
              # - bme280
              if(value=="temperature-pcb" or value=="bme280")then
                @limits_temperature["source"] = value
              else
                log_error("Invalid temperature source #{threshold_type} was specified.")
              end
            else
              log_error("Invalid limit key #{threshold_type} was specified (supported = upper/lower/source)")
              @has_temperature_limit = false
            end
          end
        end

        # HV limits
        if(yaml["limits"]["hv"]!=nil)then
          @has_hv_limit = true
          for ch, value_V in yaml["limits"]["hv"]
            @limit_hv[ch.to_i] = value_V
            log_info("HV limit Ch.#{ch} = #{value_V} V")
          end
        end
      else
        log_warn("No limit defined in growth_config.yaml")
      end
    end

    private
    def load_autorun_config(yaml)
      if(yaml["autorun"]!=nil)then
        # DAQ-related configuration
        if(yaml["autorun"]["daq"]!=nil)then
          @autorun_daq_exposure_sec = yaml["autorun"]["daq"]["exposure_sec"]
          log_info("DAQ autorun observation exposure #{@autorun_daq_exposure_sec} sec.")
        end

        # HK-related configuration
        if(yaml["autorun"]["hk"]!=nil)then
          @autorun_hk_sampling_period_sec = yaml["autorun"]["hk"]["sampling_period_sec"]
          log_info("HK autorun observation sampling perioid #{@autorun_hk_sampling_period_sec} sec.")
        end

      else
        log_warn("No autorun configuration defined.")
      end
    end
  end
end
