require "json"
require "ffi-rzmq"

context = ZMQ::Context.new
socket  = context.socket(ZMQ::REQ)
socket.connect("tcp://localhost:5555")

str=<<EOS
{
	"command": "stop"
}
EOS

puts socket.send_string(str)
