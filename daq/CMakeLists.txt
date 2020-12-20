#=============================================
# Definition of project
#=============================================
project( GROWTH_DAQ CXX)
cmake_minimum_required(VERSION 3.12)
set (CMAKE_CXX_STANDARD 17)
if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    add_definitions(-DRASPBERRY_PI)
endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )

if(USE_ROOT)
    find_package(ROOT REQUIRED)
    add_definitions(-DUSE_ROOT)
endif(USE_ROOT)

set(CMAKE_CXXFLAGS "-fcolor-diagnostics")

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

#=============================================
# Include path
#=============================================
include_directories(SYSTEM
  include
  include/picojson
  include/CxxUtilities/includes
  external/serial/include
  external/libzmq/include
  external/cppzmq
  external/cfitsio
  external/yaml-cpp/include
  ${ROOT_INCLUDE_DIRS}
)

include_directories(
  src
)

SET(ZeroMQ_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/libzmq)

add_subdirectory(external/libzmq)
add_subdirectory(external/yaml-cpp)
add_subdirectory(external) # For cfitsio and serial


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
  src/spacewire/rmapengine.cc
  src/spacewire/spacewireutil.cc
  src/growth_daq.cc
  src/EventListFileFITS.cc
  src/MessageServer.cc
  src/SerialPort.cc
  src/adcboard.cc
  src/timeutil.cc
  src/stringutil.cc
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

#=============================================
# Installs
#=============================================
install_targets(/bin growth_daq)

#=============================================
# Custom target
#=============================================
add_custom_target(
        clangformat
        COMMAND clang-format
        -i
        ${SOURCE_FILES} ${HEADER_FILES}
)
