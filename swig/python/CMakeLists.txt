
find_package ( PythonLibs REQUIRED )
find_package ( PythonInterp REQUIRED )

if ( PYTHON_VERSION_STRING AND PYTHONLIBS_VERSION_STRING )
  if ( NOT PYTHON_VERSION_STRING VERSION_EQUAL PYTHONLIBS_VERSION_STRING )
    message ( FATAL_ERROR
            "Version mismatch between python interpreter and libraries" )
  endif ( NOT PYTHON_VERSION_STRING VERSION_EQUAL PYTHONLIBS_VERSION_STRING )
endif ( PYTHON_VERSION_STRING AND PYTHONLIBS_VERSION_STRING )

include_directories ( ${PYTHON_INCLUDE_DIRS} )

foreach ( src ${obexftp_SWIG_SOURCES} )
  list ( APPEND obexftp-python_SWIG_SOURCES ../${src} )
endforeach ( src )

set ( CMAKE_SWIG_FLAGS -module obexftp )
set_source_files_properties ( ${obexftp-python_SWIG_SOURCES} PROPERTIES
  SWIG_MODULE_NAME obexftp
)

swig_add_module ( obexftp-python python ${obexftp-python_SWIG_SOURCES} )

set_target_properties ( ${SWIG_MODULE_obexftp-python_REAL_NAME} PROPERTIES
  OUTPUT_NAME _obexftp
  NO_SONAME TRUE
)

swig_link_libraries ( obexftp-python
  obexftp
)

#
# Get the install directory for ruby modules and strip the ruby install prefix
# so that we can use our own prefix.
#
execute_process (
  COMMAND ${PYTHON_EXECUTABLE} -c
  	"import site, sys; sys.stdout.write(site.PREFIXES[-1])"
  OUTPUT_VARIABLE PYTHON_PREFIX
)
file ( TO_CMAKE_PATH "${PYTHON_PREFIX}" PYTHON_PREFIX )
execute_process (
  COMMAND ${PYTHON_EXECUTABLE} -c
  	"import site, sys; sys.stdout.write(site.getsitepackages()[-1])"
  OUTPUT_VARIABLE PYTHON_SITE_DIR
)
file ( TO_CMAKE_PATH "${PYTHON_SITE_DIR}" PYTHON_SITE_DIR )
string ( REGEX REPLACE "^${PYTHON_PREFIX}/" ""
  PYTHON_SITE_DIR "${PYTHON_SITE_DIR}"
)

install ( TARGETS ${SWIG_MODULE_obexftp-python_REAL_NAME}
  LIBRARY
    DESTINATION ${PYTHON_SITE_DIR}
    COMPONENT library
)

install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/obexftp.py
  DESTINATION ${PYTHON_SITE_DIR}
  COMPONENT library
)
