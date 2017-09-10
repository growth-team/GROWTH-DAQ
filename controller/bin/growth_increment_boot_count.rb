#!/usr/bin/env ruby

BOOT_COUNT_FILE = "/home/pi/bootcount.text"

if File.exist?(BOOT_COUNT_FILE) then
  str = open(BOOT_COUNT_FILE).read()
  count = str.to_i
  output_file = open(BOOT_COUNT_FILE, "w")
  output_file.puts count+1
  output_file.close
else
  output_file = open(BOOT_COUNT_FILE, "w")
  output_file.puts "1"
  output_file.close
end
