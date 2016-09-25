#=============================================
# Definition of project
#=============================================
project( GROWTH_DAQ CXX)
cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)
if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    add_definitions(-DRASPBERRY_PI)
endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    
#=============================================
# Initial definition of cmake variables
#=============================================
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/work/install CACHE PATH "install prefix")
set(CMAKE_BUILD_TYPE Release CACHE STRING "build type")

#=============================================
# Include path
#=============================================
include_directories(
  src
  include/picojson
  include/SpaceWireRMAPLibrary/includes/SpaceWireRMAPLibrary
  include/XMLUtilities/include
  include/CxxUtilities/includes
)

#=============================================
# Link directories
#=============================================
link_directories(
)

#=============================================
# Executable definitions
#=============================================
#---------------------------------------------
# Executable source
#---------------------------------------------
add_executable(growth_daq
  src/growth_daq.cc
)

#---------------------------------------------
# Linked libraries
#---------------------------------------------
target_link_libraries(growth_daq
  cfitsio
  yaml-cpp
  zmq
  xerces-c
  boost_system
  boost_thread-mt
)
if (__APPLE__)
else(__APPLE__)
  target_link_libraries(growth_daq
    pthread
  )
endif (__APPLE__)

#=============================================
# Installs
#=============================================
install_targets(/bin growth_daq)

