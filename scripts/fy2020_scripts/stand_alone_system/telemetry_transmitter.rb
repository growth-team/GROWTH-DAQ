#!/usr/bin/env ruby
require "yaml"
require "json"

waiting=rand(1..600)
sleep waiting

inputFile="/home/pi/growth_config.yaml"
data=YAML.load_file(inputFile)
id=data["detectorID"]

date=`date "+%Y-%m-%d %H:%M:%S %Z"`.chomp!
hk_file=`find /home/pi/work/growth/data/#{id}/ -name "hk*.text" -type f -print0 | xargs -0 ls -at1`.chomp!
File.open(hk_file, "r") do |hk_data|
  line=hk_data.readlines()
  hk_hash=JSON.parse(line[line.length-1])
  time=hk_hash["hk"]["unixtime"]
  temp=hk_hash["hk"]["hk"]["bme280"]["temperature"]["value"].to_f.round(2)
  pres=hk_hash["hk"]["hk"]["bme280"]["pressure"]["value"].to_f.round(2)
  humi=hk_hash["hk"]["hk"]["bme280"]["humidity"]["value"].to_f.round(2)
  sdaq=hk_hash["daq"]["daqStatus"]
  shv0=hk_hash["hv"]["0"]["status"]
  shv1=hk_hash["hv"]["1"]["status"]
  string="date: #{date}, unixtime: #{time}, temp: #{temp}, pressure: #{pres}, humidity: #{humi}, DAQ status: #{sdaq}, HV0 status: #{shv0}, HV1 status: #{shv1}"
  `ssh wada@thdr.info 'echo #{string} >> /home/wada/telemetry/#{id}.csv'`
end
