#
# GROWTH-FY2016 FPGA/ADC Board - Clock constraints
#

create_clock -period 16.667 -name ACBUS5_CLKOUT -waveform {0.000 10.000} [get_ports ACBUS5_CLKOUT]

create_clock -period 20.000 -name AD1_DCOA -waveform {0.000 10.000} [get_ports AD1_DCOA]
create_clock -period 20.000 -name AD1_DCOB -waveform {0.000 10.000} [get_ports AD1_DCOB]
create_clock -period 20.000 -name AD2_DCOA -waveform {0.000 10.000} [get_ports AD2_DCOA]
create_clock -period 20.000 -name AD2_DCOB -waveform {0.000 10.000} [get_ports AD2_DCOB]

set_clock_groups -asynchronous -group [get_clocks {AD1_DCOA AD1_DCOB AD2_DCOA AD2_DCOB}] -group [get_clocks {CLKIN clk_out1_clk_wiz_0}] -group [get_clocks ACBUS5_CLKOUT]





