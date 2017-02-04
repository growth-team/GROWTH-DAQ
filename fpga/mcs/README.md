# GROWTH FPGA/ADC Board - FPGA images

This folder contains pre-built FPGA images that can be used with
the GROWTH FPGA/ADC board.

- growth_fy2016_fpga_20160831_2144.mcs
    - All 4 ADC channels are enabled
    - ADC clock 50MHz
    - FT2232H Bank B: Pulse event measurement
    - Use Bank B (2) serial device when running `growth_daq`.
E.g. 
```
$ growth_daq /dev/tty.usbserial--FT0K8WCSB configuration.yaml 100
$ growth_daq /dev/ttyUSB2 configuration.yaml 100
```

- growth_fy2016_fpga_20161010_1152.mcs
    - Low-power-consumption version
    - Ch.0 and Ch.1 are only enabled
    - ADC clock 25MHz
    - ADC clock is output only when a measurement is started by software
    - FT2232H Bank B: Pulse event measurement
    - Use Bank B (2) serial device when running `growth_daq`.
E.g. 
```
$ growth_daq /dev/tty.usbserial--FT0K8WCSB configuration.yaml 100
$ growth_daq /dev/ttyUSB2 configuration.yaml 100
```

- growth_fy2016_fpga_20170129_1411_with_GPS_serial_data_ADC2ch.mcs.zip
    - Ch.0 and Ch.1 are only enabled
    - ADC clock 25MHz
    - ADC clock is output only when a measurement is started by software
    - FT2232H Bank A: Pulse event measurement
    - FT2232H Bank B: GPS serial output (through connection from daughter card)
    - Use Bank A (1) serial device when running `growth_daq`.
E.g. 
```
$ growth_daq /dev/tty.usbserial--FT0K8WCSA configuration.yaml 100
$ growth_daq /dev/ttyUSB1 configuration.yaml 100
```
