#!/usr/bin/env ruby

require "pi_piper"

if(ARGV.length<2)then
	STDERR.puts "Provide ch (0/1) and output voltage in mV"
	exit -1
end

# 0=Ch A, 1=Ch B
ch=ARGV[0].to_i
value = ARGV[1].to_i

# Gain setting: 0=gain 2x, 1=gain 1x
ngain = 0 

# Shutdown setting: 0=shutdown 1=output enabled
nshdn = 1

# Check parameters
if(ch<0 or ch>1)then
    STDERR.puts "Error: Channel should be either of 0 or 1."
    exit -1
end

if(value<0 or value>3300)then
    STDERR.puts "Error: output voltage should be >=0 and <=3300"
    exit -1
end

# Construct SPI instance
PiPiper::Spi.begin(PiPiper::Spi::CHIP_SELECT_0) do |spi|
    header = "0b#{ch}0#{ngain}#{nshdn}".to_i(2) << 12
    register_value = header + value
    puts "Setting Ch. #{ch} voltage at #{"%.3f"%(value/1000.0)} V"
    spi.write [ register_value/0x100, register_value%0x100 ]
end
