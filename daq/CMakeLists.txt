#=============================================
# Definition of project
#=============================================
project( GROWTH_DAQ CXX)
cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)
if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    add_definitions(-DRASPBERRY_PI)
endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )

if(USE_ROOT)
    find_package(ROOT REQUIRED)
    add_definitions(-DUSE_ROOT)
endif(USE_ROOT)

find_package(Boost)

#=============================================
# Initial definition of cmake variables
#=============================================
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/work/install CACHE PATH "install prefix")
set(CMAKE_BUILD_TYPE Release CACHE STRING "build type")

#=============================================
# Source file list
#=============================================
file(GLOB_RECURSE HEADER_FILES src/*/*.hh)
file(GLOB_RECURSE SOURCE_FILES src/*/*.cc)

#=============================================
# Include path
#=============================================
include_directories(
  src
  include/picojson
  include/SpaceWireRMAPLibrary/includes/SpaceWireRMAPLibrary
  include/XMLUtilities/include
  include/CxxUtilities/includes
  ${ROOT_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

#=============================================
# Link directories
#=============================================
link_directories(
  ${ROOT_LIBRARY_DIR}
  ${Boost_LIBRARIES}
  /usr/local/lib
)

#=============================================
# Build external libraries
#=============================================
add_subdirectory(external)

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
  boost_thread
  ${ROOT_LIBRARIES}
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

#=============================================
# Custom target
#=============================================
add_custom_target(
        clangformat
        COMMAND clang-format
        -i
        ${SOURCE_FILES} ${HEADER_FILES}
)
