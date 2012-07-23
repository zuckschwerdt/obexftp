
find_package ( Ruby REQUIRED )

include_directories ( ${RUBY_INCLUDE_DIRS} )

foreach ( src ${obexftp_SWIG_SOURCES} )
  list ( APPEND obexftp-ruby_SWIG_SOURCES ../${src} )
endforeach ( src )

set ( CMAKE_SWIG_FLAGS -module obexftp )
set_source_files_properties ( ${obexftp-ruby_SWIG_SOURCES} PROPERTIES
  SWIG_MODULE_NAME obexftp
)

swig_add_module ( obexftp-ruby ruby ${obexftp-ruby_SWIG_SOURCES} )

set_target_properties ( ${SWIG_MODULE_obexftp-ruby_REAL_NAME} PROPERTIES
  OUTPUT_NAME obexftp
  PREFIX ""
)

swig_link_libraries ( obexftp-ruby
  obexftp
)

#
# Get the install directory for ruby modules and strip the ruby install prefix
# so that we can use our own prefix.
#
execute_process (
  COMMAND ${RUBY_EXECUTABLE} -r rbconfig
                             -e "print RbConfig::CONFIG['prefix']"
  OUTPUT_VARIABLE RUBY_PREFIX
)
file ( TO_CMAKE_PATH "${RUBY_PREFIX}" RUBY_PREFIX )
execute_process (
  COMMAND ${RUBY_EXECUTABLE} -r rbconfig
                             -e "print RbConfig::CONFIG['vendorarchdir']"
  OUTPUT_VARIABLE RUBY_FULL_VENDOR_ARCH_DIR
)
file ( TO_CMAKE_PATH "${RUBY_FULL_VENDOR_ARCH_DIR}" RUBY_FULL_VENDOR_ARCH_DIR )
string ( REGEX REPLACE "^${RUBY_PREFIX}/" ""
  RUBY_VENDOR_ARCH_DIR "${RUBY_FULL_VENDOR_ARCH_DIR}"
)


install ( TARGETS ${SWIG_MODULE_obexftp-ruby_REAL_NAME}
  LIBRARY
    DESTINATION ${RUBY_VENDOR_ARCH_DIR}
    COMPONENT library
)
