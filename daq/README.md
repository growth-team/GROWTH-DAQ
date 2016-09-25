# growth_daq

A DAQ program that 

- configures the GROWTH FPGA/ADC board (threshold, number of samples, etc)
- collects the maximum PHA (pulse height amplitude) and waveform of
  triggered pulse events, and
- saves the collected events to a FITS or a ROOT file.

## Folder structure

- src: contains source of the DAQ program
- configuration_files: contains example configuration files

## Install

```
mkdir build
cmake /.../repo/daq -DCMAKE_INSTALL_PREFIX=$HOME
make -j2
```

## Execution

```
Usage:
$ growth_daq (serial device) (configuration file) (exposure in sec)

Example:
$ growth_daq /dev/ttyUSB1 configuration.yaml 100
```

