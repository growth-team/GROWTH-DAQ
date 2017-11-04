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

## Source code
### Auto format

Install `clang-format` to use auto-formating of the C++ source/header files.

```
brew install clang-format
```

`CMakeLists.txt` defines `clangformat`, and this can be used by running `make clangformat` in the build directory.
