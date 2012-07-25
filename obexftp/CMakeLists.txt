
set ( obexftp_SOURCES
  object.c
  client.c
  obexftp_io.c
  cache.c
  unicode.c
  bt_kit.c
)

set ( obexftp_PUBLIC_HEADERS
  obexftp.h
  client.h
  uuid.h
  object.h
)

set ( obexftp_HEADERS
  object.h
  obexftp_io.h
  cache.h
  unicode.h
  bt_kit.h
  ${obexftp_PUBLIC_HEADERS}
)

find_package ( Bluetooth REQUIRED )
include_directories ( ${Bluetooth_INCLUDE_DIRS} )
add_definitions ( -DHAVE_BLUETOOTH -DHAVE_SDP )

find_package ( OpenObex ${OpenObex_FORCE_VERSION} QUIET NO_MODULE)
if ( NOT OpenObex_FOUND )
  find_package ( OpenObex REQUIRED )
endif ( NOT OpenObex_FOUND )
include_directories ( ${OpenObex_INCLUDE_DIRS} )

find_package ( Iconv REQUIRED )
add_definitions ( -DHAVE_ICONV )
if ( ICONV_USES_CONST )
  add_definitions ( -DICONV_CONST=const )
endif ( ICONV_USES_CONST )
find_file ( HAVE_LANGINFO_H NAMES langinfo.h )
if ( HAVE_LANGINFO_H )
  add_definitions ( -DHAVE_LANGINFO_H )
endif ( HAVE_LANGINFO_H )

# always set this
add_definitions ( -DHAVE_USB )

add_library ( obexftp
  ${obexftp_SOURCES}
  ${obexftp_HEADERS}
)

set_property ( TARGET obexftp PROPERTY VERSION 0.3.0 )
set_property ( TARGET obexftp PROPERTY SOVERSION 0 )
set_property ( TARGET obexftp PROPERTY PUBLIC_HEADER ${obexftp_PUBLIC_HEADERS} )

target_link_libraries ( obexftp
  multicobex
  ${Bluetooth_LIBRARIES}
  ${OpenObex_LIBRARIES}
)

install ( TARGETS obexftp
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
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/obexftp
    COMPONENT devel
)
