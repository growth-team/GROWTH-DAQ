project(growth_daq_external_deps)
cmake_minimum_required(VERSION 3.10)
set (CMAKE_CXX_STANDARD 14)

#=============================================
# cfitsio
#=============================================
set(CFITSIO_VER 3.49)
set(CFITSIO_SRC_ARCHIVE cfitsio-${CFITSIO_VER}.tar.gz)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_UNISTD_H -Wno-unused-value -Wno-absolute-value -Wno-pointer-bool-conversion")
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
list(TRANSFORM cfitsio_SRCS PREPEND ${CMAKE_CURRENT_BINARY_DIR}/cfitsio-${CFITSIO_VER}/)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/cfitsio-${CFITSIO_VER}/)
message("Include dir = ${CMAKE_CURRENT_BINARY_DIR}/cfitsio-${CFITSIO_VER}/")

add_custom_command(
    OUTPUT ${cfitsio_SRCS}
    COMMAND tar -zxf ${CMAKE_CURRENT_SOURCE_DIR}/${CFITSIO_SRC_ARCHIVE} -C ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(cfitsio ${cfitsio_SRCS})

#=============================================
# serial
#=============================================
if(APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(FOUNDATION_LIBRARY Foundation)
endif()

if(UNIX AND NOT APPLE)
    # If Linux, add rt and pthread
    set(rt_LIBRARIES rt)
    set(pthread_LIBRARIES pthread)
else()
    # Otherwise normal call
endif()

## Sources
set(serial_SRCS
    src/serial.cc
    include/serial/serial.h
    include/serial/v8stdint.h
)
if(APPLE)
    # If OSX
    list(APPEND serial_SRCS src/impl/unix.cc)
    list(APPEND serial_SRCS src/impl/list_ports/list_ports_osx.cc)
elseif(UNIX)
    # If unix
    list(APPEND serial_SRCS src/impl/unix.cc)
    list(APPEND serial_SRCS src/impl/list_ports/list_ports_linux.cc)
else()
    # If windows
    list(APPEND serial_SRCS src/impl/win.cc)
    list(APPEND serial_SRCS src/impl/list_ports/list_ports_win.cc)
endif()

list(TRANSFORM serial_SRCS PREPEND serial/)

include_directories(serial/include)

## Add serial library
add_library(serial ${serial_SRCS})
if(APPLE)
    target_link_libraries(serial ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif(UNIX)
    target_link_libraries(serial rt pthread)
else()
    target_link_libraries(serial setupapi)
endif()

