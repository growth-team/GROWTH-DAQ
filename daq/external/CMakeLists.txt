#=============================================
# Definition of project
#=============================================
project(cfitsio)
cmake_minimum_required(VERSION 3.12)
set (CMAKE_CXX_STANDARD 14)

set(CMAKE_INSTALL_PREFIX $ENV{HOME}/work/install CACHE PATH "install prefix")
set(CMAKE_BUILD_TYPE Release CACHE STRING "build type")

include_directories(
  cfitsio
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
list(TRANSFORM cfitsio_SRCS PREPEND cfitsio/)
add_library(cfitsio ${cfitsio_SRCS})
