
find_package ( TCL )

if ( TCL_FOUND )
  include_directories ( ${TCL_INCLUDE_PATH} )

  foreach ( src ${obexftp_SWIG_SOURCES} )
    list ( APPEND obexftp-tcl_SWIG_SOURCES ../${src} )
  endforeach ( src )

  set ( CMAKE_SWIG_FLAGS -module obexftp )
  set_source_files_properties ( ${obexftp-tcl_SWIG_SOURCES} PROPERTIES
    SWIG_MODULE_NAME obexftp
  )

  swig_add_module ( obexftp-tcl tcl ${obexftp-tcl_SWIG_SOURCES} )

  set_target_properties ( ${SWIG_MODULE_obexftp-tcl_REAL_NAME} PROPERTIES
    OUTPUT_NAME obexftp
    PREFIX ""
    NO_SONAME TRUE
  )

  swig_link_libraries ( obexftp-tcl
    obexftp
  )

  install ( TARGETS ${SWIG_MODULE_obexftp-tcl_REAL_NAME}
    LIBRARY
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT library
  )
endif ( TCL_FOUND )
