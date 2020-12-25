# growth_daq

## 1. Overview

A data-acquisition (DAQ) program that 

- configures the GROWTH FPGA/ADC board (threshold, number of samples, etc)
- collects the maximum PHA (pulse height amplitude) and waveform of
  triggered pulse events, and
- saves the collected events to a FITS or a ROOT file.

## 2. Folder structure

- `src`: contains source of the DAQ program
- `configuration_files`: contains example configuration files
- `cli`: source of other command-line tools

## 3. Build and install

```
mkdir build
cmake /.../GROWTH-DAQ/daq -DCMAKE_INSTALL_PREFIX=$HOME
make -j2
make install
```

You will find compiled binaries (`growth_daq` and other command-line tools in `$CMAKE_INSTALL_PREFIX/bin`.

## 4. Execution

```
Usage:
$ growth_daq (serial device) (configuration file) (exposure in sec)

Example:
$ growth_daq /dev/ttyUSB1 configuration.yaml 100
```

## 5. Coding style

### 5.1. Use clang-format

There is a CMake target `clangformat` which reformats all header and source files, and can be executed within the build directory.

```
mkdir build
cmake ../growth-daq/daq
make clangformat
```

If clang-format is not installed, run the following commands:

```
# Mac
brew install clang-format

# Ubuntu
sudo apt install clang-format
```

### 5.2. Use CppStyle on Eclipse CDT

When using Eclipse CDT as an IDE, you can install [the CppStyle plugin](https://marketplace.eclipse.org/content/cppstyle) to format the header/source files within Eclipse.

How to setup:

1. Drag-and-drop "Install" button in [this page](https://marketplace.eclipse.org/content/cppstyle) to an Eclipse window to start installation.
2. After install, open "Preference" window (from "Eclipse" menu item on Mac), and navigate to "CppStyle" setting tab. Fill in the path to clang-format properly (if you installed clang-format using Homebrew, it should be `/usr/local/bin/clang-format`).
3. Open "Properties" of your GROWTH-DAQ project, and navigate to "Formatter" setting. Select "CppStyle" from the second dropdown list, and click the "Apply and close" button.

### 5.3. Naming convention

C++

- `filename.cc` (lower case, no underscore or hyphen when appropriate)
- `UpperCamelCase` for class name
- `lowerCamelCase` for method name, function name, local variables
- `lowerCamelCase_` (with underscore) for member variable
- `UPPER_SNAKE_CASE` for static constant variables
- `snake_case` for namespace

FPGA

- `filename.vhd` for file names (lowercase, no underscore or hyphen)
- `UpperCamelCase` for module name
- `snake_case` for interface signals, module-local signals, instance name, process name
- `UPPER_SNAKE_CASE` for constant variables
- `sna
