
set ( bfb_SOURCES
  bfb.c
  bfb_io.c
  crc.c
)

set ( bfb_PUBLIC_HEADERS
  bfb.h
  bfb_io.h
)

set ( bfb_HEADERS
  crc.h
  ${bfb_PUBLIC_HEADERS}
)

add_library ( bfb
  ${bfb_SOURCES}
  ${bfb_HEADERS}
)

set_property ( TARGET bfb PROPERTY VERSION 0.0.5 )
set_property ( TARGET bfb PROPERTY SOVERSION 0 )
set_property ( TARGET bfb PROPERTY PUBLIC_HEADER ${bfb_PUBLIC_HEADERS} )

install ( TARGETS bfb
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT library
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT library
  ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT devel
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bfb
    COMPONENT devel
)
