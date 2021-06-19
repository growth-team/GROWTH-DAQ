#=============================================
# Definition of project
#=============================================
project( GROWTH_DAQ CXX)
cmake_minimum_required(VERSION 3.10)
set (CMAKE_CXX_STANDARD 17)
if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    add_definitions(-DRASPBERRY_PI)
endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )

if(USE_ROOT)
    find_package(ROOT REQUIRED)
    add_definitions(-DUSE_ROOT)
endif(USE_ROOT)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASIO_STANDALONE")

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics -g")
endif(APPLE)

#=============================================
# Initial definition of cmake variables
#=============================================
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/work/install CACHE PATH "install prefix")
set(CMAKE_BUILD_TYPE Release CACHE STRING "build type")

#=============================================
# Source file list
#=============================================
file(GLOB_RECURSE HEADER_FILES src/*.hh)
file(GLOB_RECURSE SOURCE_FILES src/*.cc)

# file(GLOB_RECURSE SPW_SOURCE_FILES src/spacewire/*.cc)
set(SPW_SOURCE_FILES
  src/spacewire/rmapengine.cc
  src/spacewire/rmapinitiator.cc
  src/spacewire/rmappacket.cc
  src/spacewire/rmapreplystatus.cc
  src/spacewire/spacewireifoveruart.cc
  src/spacewire/spacewiressdtpmodule.cc
  src/spacewire/spacewiressdtpmoduleuart.cc
  src/spacewire/spacewireutil.cc
  src/spacewire/tcpsocket.cc
)

set(GROWTH_FPGA_SOURCE_FILES
  src/growth-fpga/channelmanager.cc
  src/growth-fpga/channelmodule.cc
  src/growth-fpga/consumermanagereventfifo.cc
  src/growth-fpga/eventdecoder.cc
  src/growth-fpga/hitpatternmodule.cc
  src/growth-fpga/registeraccessinterface.cc
  src/growth-fpga/semaphoreregister.cc
  src/growth-fpga/slowadcdac.cc
)

SET(ZeroMQ_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/libzmq)
SET(ZMQ_BUILD_TESTS off)
add_subdirectory(external/libzmq)
add_subdirectory(external/yaml-cpp)
add_subdirectory(external) # For cfitsio and serial

#=============================================
# Include path
#=============================================
include_directories(SYSTEM
  include
  include/picojson
  external/serial/include
  external/libzmq/include
  external/asio/asio/include
  external/cppzmq
  external/cfitsio
  external/spdlog/include
  external/yaml-cpp/include
  ${ROOT_INCLUDE_DIRS}
  src
)

#=============================================
# Link directories
#=============================================
link_directories(
  ${ROOT_LIBRARY_DIR}
  /usr/local/lib
)

#=============================================
# Executable definitions
#=============================================
#---------------------------------------------
# Executable source
#---------------------------------------------
add_executable(growth_daq
  src/adcboard.cc
  src/eventlistfilefits.cc
  src/growth_daq.cc
  src/mainthread.cc
  src/messageserver.cc
  src/serialport.cc
  src/stringutil.cc
  src/timeutil.cc
  ${SPW_SOURCE_FILES}
  ${GROWTH_FPGA_SOURCE_FILES}
)

add_executable(growth_reg
  cli/growth_reg.cc
  src/serialport.cc
  src/stringutil.cc
  src/timeutil.cc
  src/registeraccessautomator.cc
  ${SPW_SOURCE_FILES}
  ${GROWTH_FPGA_SOURCE_FILES}
)

#---------------------------------------------
# Linked libraries
#---------------------------------------------
target_link_libraries(growth_daq
  cfitsio
  yaml-cpp
  libzmq
  serial
  ${ROOT_LIBRARIES}
  pthread
  z
)

target_link_libraries(growth_reg
  yaml-cpp
  serial
)

#=============================================
# Installs
#=============================================
install_targets(/bin growth_daq growth_reg)

#=============================================
# Custom target
#=============================================
add_custom_target(
  clangformat
  COMMAND clang-format
  -i
  ${SOURCE_FILES} ${HEADER_FILES}
)
