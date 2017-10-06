#
# GROWTH-FY2016 FPGA/ADC Board - Bitstream constraints
#

set_property BITSTREAM.CONFIG.CONFIGRATE 33 [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]



connect_debug_port u_ila_1/probe0 [get_nets [list {ssdtp2RMAP/ssdtp2rmapFIFODataIn[0]} {ssdtp2RMAP/ssdtp2rmapFIFODataIn[1]} {ssdtp2RMAP/ssdtp2rmapFIFODataIn[2]} {ssdtp2RMAP/ssdtp2rmapFIFODataIn[3]} {ssdtp2RMAP/ssdtp2rmapFIFODataIn[4]} {ssdtp2RMAP/ssdtp2rmapFIFODataIn[5]} {ssdtp2RMAP/ssdtp2rmapFIFODataIn[6]} {ssdtp2RMAP/ssdtp2rmapFIFODataIn[7]} {ssdtp2RMAP/ssdtp2rmapFIFODataIn[8]}]]
connect_debug_port u_ila_0/probe1 [get_nets [list ft2232_inRead]]
connect_debug_port u_ila_0/probe3 [get_nets [list ft2232_outValid]]
connect_debug_port u_ila_1/probe2 [get_nets [list {ssdtp2RMAP/ssdtp2rmapFIFODataCount[0]} {ssdtp2RMAP/ssdtp2rmapFIFODataCount[1]} {ssdtp2RMAP/ssdtp2rmapFIFODataCount[2]} {ssdtp2RMAP/ssdtp2rmapFIFODataCount[3]} {ssdtp2RMAP/ssdtp2rmapFIFODataCount[4]} {ssdtp2RMAP/ssdtp2rmapFIFODataCount[5]}]]
connect_debug_port u_ila_1/probe5 [get_nets [list ft232SendFIFOReadEnable]]

connect_debug_port u_ila_0/probe12 [get_nets [list ssdtp2RMAP/ssdtp2rmapFIFOReadEnable]]





connect_debug_port u_ila_0/probe0 [get_nets [list {ssdtp2RMAP/commandType[0]} {ssdtp2RMAP/commandType[1]} {ssdtp2RMAP/commandType[2]} {ssdtp2RMAP/commandType[3]} {ssdtp2RMAP/commandType[4]} {ssdtp2RMAP/commandType[5]} {ssdtp2RMAP/commandType[6]} {ssdtp2RMAP/commandType[7]}]]
connect_debug_port u_ila_0/probe1 [get_nets [list {ssdtp2RMAP/dataLength[0]} {ssdtp2RMAP/dataLength[1]} {ssdtp2RMAP/dataLength[2]} {ssdtp2RMAP/dataLength[3]} {ssdtp2RMAP/dataLength[4]} {ssdtp2RMAP/dataLength[5]} {ssdtp2RMAP/dataLength[6]} {ssdtp2RMAP/dataLength[7]} {ssdtp2RMAP/dataLength[8]} {ssdtp2RMAP/dataLength[9]} {ssdtp2RMAP/dataLength[10]} {ssdtp2RMAP/dataLength[11]} {ssdtp2RMAP/dataLength[12]} {ssdtp2RMAP/dataLength[13]} {ssdtp2RMAP/dataLength[14]} {ssdtp2RMAP/dataLength[15]} {ssdtp2RMAP/dataLength[16]} {ssdtp2RMAP/dataLength[17]} {ssdtp2RMAP/dataLength[18]} {ssdtp2RMAP/dataLength[19]} {ssdtp2RMAP/dataLength[20]} {ssdtp2RMAP/dataLength[21]} {ssdtp2RMAP/dataLength[22]} {ssdtp2RMAP/dataLength[23]}]]
connect_debug_port u_ila_0/probe2 [get_nets [list {ssdtp2RMAP/ustate[0]} {ssdtp2RMAP/ustate[1]} {ssdtp2RMAP/ustate[2]} {ssdtp2RMAP/ustate[3]} {ssdtp2RMAP/ustate[4]}]]
connect_debug_port u_ila_0/probe3 [get_nets [list {ssdtp2RMAP/startAddress[0]} {ssdtp2RMAP/startAddress[1]} {ssdtp2RMAP/startAddress[2]} {ssdtp2RMAP/startAddress[3]} {ssdtp2RMAP/startAddress[4]} {ssdtp2RMAP/startAddress[5]} {ssdtp2RMAP/startAddress[6]} {ssdtp2RMAP/startAddress[7]} {ssdtp2RMAP/startAddress[8]} {ssdtp2RMAP/startAddress[9]} {ssdtp2RMAP/startAddress[10]} {ssdtp2RMAP/startAddress[11]} {ssdtp2RMAP/startAddress[12]} {ssdtp2RMAP/startAddress[13]} {ssdtp2RMAP/startAddress[14]} {ssdtp2RMAP/startAddress[15]} {ssdtp2RMAP/startAddress[16]} {ssdtp2RMAP/startAddress[17]} {ssdtp2RMAP/startAddress[18]} {ssdtp2RMAP/startAddress[19]} {ssdtp2RMAP/startAddress[20]} {ssdtp2RMAP/startAddress[21]} {ssdtp2RMAP/startAddress[22]} {ssdtp2RMAP/startAddress[23]} {ssdtp2RMAP/startAddress[24]} {ssdtp2RMAP/startAddress[25]} {ssdtp2RMAP/startAddress[26]} {ssdtp2RMAP/startAddress[27]} {ssdtp2RMAP/startAddress[28]} {ssdtp2RMAP/startAddress[29]} {ssdtp2RMAP/startAddress[30]} {ssdtp2RMAP/startAddress[31]}]]


connect_debug_port u_ila_0/probe1 [get_nets [list {ssdtp2RMAP/rmapErrorCode[0]} {ssdtp2RMAP/rmapErrorCode[1]} {ssdtp2RMAP/rmapErrorCode[2]} {ssdtp2RMAP/rmapErrorCode[3]} {ssdtp2RMAP/rmapErrorCode[4]} {ssdtp2RMAP/rmapErrorCode[5]} {ssdtp2RMAP/rmapErrorCode[6]} {ssdtp2RMAP/rmapErrorCode[7]}]]
connect_debug_port u_ila_0/probe2 [get_nets [list uartRMAPBusMasterReadEnable]]

connect_debug_port u_ila_0/probe0 [get_nets [list {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[0]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[1]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[2]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[3]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[4]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[5]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[6]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[7]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[8]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[9]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[10]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[11]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[12]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[13]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[14]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[15]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[16]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[17]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[18]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[19]} {ssdtp2RMAP/ssdtp2_tcp2spw/packetSizeInByte[20]}]]

