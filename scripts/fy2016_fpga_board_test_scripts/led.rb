#!/usr/bin/env ruby

require "pi_piper"

# Construct Pin instances
led0 = PiPiper::Pin.new(:pin => 26, :direction => :out)
led1 = PiPiper::Pin.new(:pin => 20, :direction => :out)

# Turn on LED0, turn off LED1
led0.on
led1.off

# Wait 5 sec before exiting
sleep 5
