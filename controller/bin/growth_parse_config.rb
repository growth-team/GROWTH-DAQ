#!/usr/bin/env ruby

require "growth_controller/config"

# This script parses growth_config.yaml and dumps parse result.

@logger = GROWTH.logger(ARGV, "growth_parse_config")
growth_config = GROWTH::Config.new(logger: @logger)
