#!/usr/bin/env ruby
require "yaml"

inputFile="/home/pi/growth_config.yaml"
data=YAML.load_file(inputFile)
puts data["detectorID"]
