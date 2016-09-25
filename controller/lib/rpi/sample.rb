require "rpi"

i2cbus = RPi::I2CBus.new(1)
bme = RPi::BME280.new(i2cbus)

bme.update

puts "Temperature: #{bme.temperature} ºC"
puts "Pressure: #{bme.pressure} mb"
puts "Humidity: #{bme.humidity} %"