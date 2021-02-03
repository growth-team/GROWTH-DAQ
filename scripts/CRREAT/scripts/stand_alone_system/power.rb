require "pi_piper"

switch = PiPiper::Pin.new(:pin => 17, :direction => :in)

loop do
  switch.read()
  if switch.off?() then
    `sudo shutdown -h now`
  else
    sleep 5
  end
end
