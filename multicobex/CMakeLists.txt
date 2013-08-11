
set ( multicobex_SOURCES
  multi_cobex.c
)

set ( multicobex_PUBLIC_HEADERS
  multi_cobex.h
)

set ( multicobex_HEADERS
  multi_cobex.h
  ${multicobex_PUBLIC_HEADERS}
)

add_library ( multicobex
  ${multicobex_SOURCES}
  ${multicobex_HEADERS}
)

set_property ( TARGET multicobex PROPERTY VERSION 1.0.1 )
set_property ( TARGET multicobex PROPERTY SOVERSION 1 )
set_property ( TARGET multicobex PROPERTY PUBLIC_HEADER ${multicobex_PUBLIC_HEADERS} )

target_link_libraries ( multicobex
  bfb
)

#disable link chaining
set_property(TARGET multicobex PROPERTY LINK_INTERFACE_LIBRARIES "")

install ( TARGETS multicobex
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
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/multicobex
    COMPONENT devel
)
