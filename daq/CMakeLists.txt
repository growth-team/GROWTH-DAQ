#=============================================
# Definition of project
#=============================================
project( GROWTH_DAQ CXX)
cmake_minimum_required(VERSION 3.12)
set (CMAKE_CXX_STANDARD 14)
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
  include/XMLUtilities/include
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

add_subdirectory(external/serial)
add_subdirectory(external/libzmq EXCLUDE_FROM_ALL)
add_subdirectory(external/yaml-cpp)


#=============================================
# Link directories
#=============================================
link_directories(
  ${ROOT_LIBRARY_DIR}
  /usr/local/lib
)

set(cfitsio_SRCS
    buffers.c cfileio.c checksum.c
    drvrfile.c drvrmem.c drvrnet.c
    editcol.c edithdu.c eval_f.c eval_l.c eval_y.c
    f77_wrap1.c f77_wrap2.c f77_wrap3.c f77_wrap4.c
    fits_hcompress.c fits_hdecompress.c fitscore.c
    getcol.c getcolb.c getcold.c getcole.c getcoli.c getcolj.c getcolk.c
    getcoll.c getcols.c getcolsb.c getcolui.c getcoluj.c getcoluk.c
    getkey.c group.c grparser.c histo.c imcompress.c iraffits.c modkey.c
    pliocomp.c
    putcol.c putcolb.c putcold.c putcole.c putcoli.c putcolj.c putcolk.c
    putcoll.c putcols.c putcolsb.c putcolu.c putcolui.c putcoluj.c putcoluk.c
    putkey.c quantize.c region.c ricecomp.c scalnull.c simplerng.c swapproc.c
    wcssub.c wcsutil.c zlib/zcompress.c zlib/zuncompress.c
)
list(TRANSFORM cfitsio_SRCS PREPEND ${CMAKE_SOURCE_DIR}/external/cfitsio/)
add_library(cfitsio ${cfitsio_SRCS})

#=============================================
# Executable definitions
#=============================================
#---------------------------------------------
# Executable source
#---------------------------------------------
add_executable(growth_daq
  src/spacewire/rmapengine.cc
  src/growth_daq.cc
  src/EventListFileFITS.cc
  src/MessageServer.cc
  src/SerialPort.cc
  src/adcboard.cc
)

#---------------------------------------------
# Linked libraries
#---------------------------------------------
target_link_libraries(growth_daq
  cfitsio
  yaml-cpp
  zmq
  xerces-c
  ${ROOT_LIBRARIES}
  pthread
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
