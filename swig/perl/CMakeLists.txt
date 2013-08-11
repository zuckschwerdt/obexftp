
find_package ( PerlLibs REQUIRED )

#foreach ( lib SITESEARCH SITELIB VENDORARCH VENDORLIB ARCHLIB PRIVLIB )
#  message ( "PERL_${lib}: ${PERL_${lib}}" )
#endforeach ( lib )

add_definitions ( ${PERL_EXTRA_C_FLAGS} )
include_directories ( ${PERL_ARCHLIB}/CORE )

foreach ( src ${obexftp_SWIG_SOURCES} )
  list ( APPEND obexftp-perl_SWIG_SOURCES ../${src} )
endforeach ( src )

set ( CMAKE_SWIG_FLAGS -module OBEXFTP )
set ( SWIG_PYTHON_EXTRA_FILE_EXTENSION "pm" )
set_source_files_properties ( ${obexftp-python_SWIG_SOURCES} PROPERTIES
  SWIG_MODULE_NAME OBEXFTP
)

swig_add_module ( obexftp-perl perl ${obexftp-perl_SWIG_SOURCES} )

set_target_properties ( ${SWIG_MODULE_obexftp-perl_REAL_NAME} PROPERTIES
  OUTPUT_NAME OBEXFTP
  PREFIX ""
  NO_SONAME TRUE
)

swig_link_libraries ( obexftp-perl
  obexftp
)

#
# Get the install directory for ruby modules and strip the ruby install prefix
# so that we can use our own prefix.
#
execute_process (
  COMMAND ${PERL_EXECUTABLE} -V:installprefix
  OUTPUT_VARIABLE PERL_PREFIX
)
string ( REGEX REPLACE "install[a-z]+='([^']+)'.*" "\\1"
  PERL_PREFIX ${PERL_PREFIX} )
file ( TO_CMAKE_PATH "${PERL_PREFIX}" PERL_PREFIX )

execute_process (
  COMMAND ${PERL_EXECUTABLE} -V:installvendorarch
  OUTPUT_VARIABLE PERL_FULL_VENDOR_ARCH_DIR
)
string ( REGEX REPLACE "install[a-z]+='([^']+)'.*" "\\1"
  PERL_FULL_VENDOR_ARCH_DIR ${PERL_FULL_VENDOR_ARCH_DIR} )
file ( TO_CMAKE_PATH "${PERL_FULL_VENDOR_ARCH_DIR}" PERL_FULL_VENDOR_ARCH_DIR )

string ( REGEX REPLACE "^${PERL_PREFIX}/" ""
  PERL_VENDOR_ARCH_DIR "${PERL_FULL_VENDOR_ARCH_DIR}"
)


install ( TARGETS ${SWIG_MODULE_obexftp-perl_REAL_NAME}
  LIBRARY
    DESTINATION ${PERL_VENDOR_ARCH_DIR}/auto/OBEXFTP
    COMPONENT library
)

install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/OBEXFTP.pm
  DESTINATION ${PERL_VENDOR_ARCH_DIR}
  COMPONENT library
)
