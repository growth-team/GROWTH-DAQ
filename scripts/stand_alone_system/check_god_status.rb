#!/usr/bin/env ruby

sleep(15)

message=`sudo god status`.split("\n")
if message.length<1 then
  `sudo bash /home/pi/git/GROWTH-DAQ/controller/god/growth_god start`
else
  parse=message[1].split("\s")
  if parse[1]=="up" then
    puts "God status normal"
  elsif parse[1]=="unmonitored" then
    puts "God status stopped"
    `sudo bash /home/pi/git/GROWTH-DAQ/controller/god/growth_god restart`
  else
    puts "God status not started"
    `sudo bash /home/pi/git/GROWTH-DAQ/controller/god/growth_god start`
  end
end
