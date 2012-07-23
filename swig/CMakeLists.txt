
find_package ( SWIG )

if ( SWIG_FOUND )
  include ( ${SWIG_USE_FILE} )
  include_directories ( ${CMAKE_CURRENT_SOURCE_DIR} )
  set ( obexftp_SWIG_SOURCES
    client.i
  )

  option ( ENABLE_PERL "Build the obexftp PERL module (using swig)" ON )
  if ( ENABLE_PERL)
    add_subdirectory ( perl )
  endif ( ENABLE_PERL )

  option ( ENABLE_PYTHON "Build the obexftp Python module (using swig)" ON )
  if ( ENABLE_PYTHON)
    add_subdirectory ( python )
  endif ( ENABLE_PYTHON )

  option ( ENABLE_RUBY "Build the obexftp Ruby module (using swig)" ON )
  if ( ENABLE_RUBY)
    add_subdirectory ( ruby )
  endif ( ENABLE_RUBY )

  option ( ENABLE_TCL "Build the obexftp Tcl module (using swig)" OFF )
  if ( ENABLE_TCL)
    add_subdirectory ( tcl )
  endif ( ENABLE_TCL )
endif ( SWIG_FOUND )
