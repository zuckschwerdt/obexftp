
set ( bfb_SOURCES
  bfb.c
  bfb_io.c
  irda_fcs.c
  ${CMAKE_CURRENT_BINARY_DIR}/irda_fcs_table.c
)

set ( bfb_PUBLIC_HEADERS
  bfb.h
  bfb_io.h
)

set ( bfb_HEADERS
  irda_fcs.h
  ${bfb_PUBLIC_HEADERS}
)

add_executable (crctable
  crcmodel.h
  crcmodel.c
  crctable.c
)

add_custom_command ( OUTPUT irda_fcs_table.c
  COMMAND crctable
)

add_library ( bfb
  ${bfb_SOURCES}
  ${bfb_HEADERS}
)

set_property ( TARGET bfb PROPERTY VERSION 0.0.6 )
set_property ( TARGET bfb PROPERTY SOVERSION 1 )
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
