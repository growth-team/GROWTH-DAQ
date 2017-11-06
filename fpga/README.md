FPGA image of GROWTH-FY2016 FPGA/ADC Board

## Overview

This folder contains VHDL source files of the GROWTH-FY2016 FPGA/ADC board.
The FPGA image generated based on these files provide the following functions.

1. Pulse signal measurement with self trigger
2. Event-data buffering using BlockRAM
3. Data readout and register read/write  via USB-Serial converter (FT2232)
4. GPS-based time tagging (if GPS module is connected)

Triggered event readout and register control can be done using `growth_daq`
application in `GROWTH-DAQ/daq`.

Pre-built images are also available in `GROWTH-DAQ/fpga/mcs`, and the
images should cover most of use cases of this board. Building an FPGA image
from source is only recommended for those who would like to make changes to
the FPGA functionality.

## Pre-requisites

1. CentOS-based workstation or virtual machine
2. `make` command
3. Xilinx Vivado 2016.2 WebPack
   - Download from https://www.xilinx.com/support/download.html
   - Note that newer versions can be used, but IP cores may need to be upgraded.

## Build

1. Make sure that the `vivado` command is in the command search path.
2. Execute the following commands:
   ```
   cd GROWTH-DAQ/fpga
   make
   ```
3. Resulting FPGA image (bit file) will be saved under `bitstream/`.
4. Convert the generated bitstream file to an MCS file when programming to EEPROM.
   See this Google Document for details of this procedure: http://bit.ly/2xUvnmn

## Bug report

Please report any bug to [the GitHub Issue page](https://github.com/growth-team/GROWTH-DAQ/issues).

Contributions are highly welcomed. Send your changes as Pull request.
