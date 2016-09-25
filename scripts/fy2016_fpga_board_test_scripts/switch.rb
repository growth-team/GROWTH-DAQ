#!/usr/bin/env ruby

require "pi_piper"

# Instantiate Pin
slide_switch = PiPiper::Pin.new(:pin => 21, :direction => :in)

# Check slide sitch status 10 times
for i in 0..10
    slide_switch.read()
    if(slide_switch.on?())then
        puts "Slide Switch == ON"
    else
        puts "Slie Switch == OFF"
    end
    sleep 1
end
    
