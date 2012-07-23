# - Find Open Object Exchange library
#
# It will use PkgConfig if present and supported, else search
# it on its own. If the OPENOBEX_ROOT_DIR environment variable
# is defined, it will be used as base path.
# The following standard variables get defined:
#  OpenObex_FOUND:        true if OpenObex was found
#  OpenObex_VERSION_STRING: the version of OpenObex, if any
#  OpenObex_INCLUDE_DIRS: the directory that contains the include file
#  OpenObex_LIBRARIES:    full path to the libraries
#  OpenObex_HAVE_TcpObex  true if OpenObex has new TcpObex_* functions

include ( CheckLibraryExists )
include ( CheckIncludeFile )
include ( CheckFunctionExists )

find_package ( PkgConfig )
if ( PKG_CONFIG_FOUND )
  if ( OpenObex_FIND_VERSION )
    pkg_check_modules ( PKGCONFIG_OPENOBEX openobex=${OpenObex_FIND_VERSION} )
  else ( OpenObex_FIND_VERSION )
    pkg_check_modules ( PKGCONFIG_OPENOBEX openobex )
  endif ( OpenObex_FIND_VERSION )    
endif ( PKG_CONFIG_FOUND )

if ( PKGCONFIG_OPENOBEX_FOUND )
  set ( OpenObex_FOUND ${PKGCONFIG_OPENOBEX_FOUND} )
  set ( OpenObex_VERSION_STRING ${PKGCONFIG_OPENOBEX_VERSION} )
  set ( OpenObex_INCLUDE_DIRS ${PKGCONFIG_OPENOBEX_INCLUDE_DIRS} )
  foreach ( i ${PKGCONFIG_OPENOBEX_LIBRARIES} )
    find_library ( ${i}_LIBRARY
                   NAMES ${i}
                   PATHS ${PKGCONFIG_OPENOBEX_LIBRARY_DIRS}
                 )
    if ( ${i}_LIBRARY )
      list ( APPEND OpenObex_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )

else ( PKGCONFIG_OPENOBEX_FOUND )
  find_path ( OpenObex_INCLUDE_DIRS
    NAMES
      openobex/obex.h
    PATHS
      $ENV{ProgramFiles}/OpenObex
      $ENV{OPENOBEX_ROOT_DIR}
    PATH_SUFFIXES
      include
  )
  mark_as_advanced ( OpenObex_INCLUDE_DIRS )
#  message ( STATUS "OpenObex include dir: ${OpenObex_INCLUDE_DIRS}" )

  find_library ( openobex_LIBRARY
    NAMES
      libopenobex openobex
    PATHS
      $ENV{ProgramFiles}/OpenObex
      $ENV{OPENOBEX_ROOT_DIR}
    PATH_SUFFIXES
      lib
  )
  mark_as_advanced ( openobex_LIBRARY )
  if ( openobex_LIBRARY )
    set ( OpenObex_LIBRARIES ${openobex_LIBRARY} )
  endif ( openobex_LIBRARY )

  if ( OpenObex_INCLUDE_DIRS AND OpenObex_LIBRARIES )
    set ( OpenObex_FOUND true )
  endif ( OpenObex_INCLUDE_DIRS AND OpenObex_LIBRARIES )
endif ( PKGCONFIG_OPENOBEX_FOUND )

if ( OpenObex_FOUND )
  set ( CMAKE_REQUIRED_INCLUDES "${OpenObex_INCLUDE_DIRS}" )
  check_include_file ( openobex.h OpenObex_FOUND )
#  message ( STATUS "OpenObex: openobex.h is usable: ${OpenObex_FOUND}" )
endif ( OpenObex_FOUND )
if ( OpenObex_FOUND )
  check_library_exists ( ${openobex_LIBRARY} OBEX_Init "${OpenObex_LIBRARY_DIRS}" OpenObex_FOUND )
#  message ( STATUS "OpenObex: library is usable: ${OpenObex_FOUND}" )
endif ( OpenObex_FOUND )
if ( OpenObex_FOUND )
  set ( CMAKE_REQUIRED_FLAGS "" )
  set ( CMAKE_REQUIRED_DEFINITIONS "" )
  set ( CMAKE_REQUIRED_INCLUDES ${OpenObex_INLUDE_DIRS} )
  set ( CMAKE_REQUIRED_LIBRARIES ${OpenObex_LIBRARIES} )
  check_function_exists ( TcpOBEX_ServerRegister OpenObex_HAVE_TcpObex )
endif ( OpenObex_FOUND )

if ( NOT OpenObex_FOUND )
  if ( NOT OpenObex_FIND_QUIETLY )
    message ( STATUS "OpenObex not found, try setting OPENOBEX_ROOT_DIR environment variable." )
  endif ( NOT OpenObex_FIND_QUIETLY )
  if ( OpenObex_FIND_REQUIRED )
    message ( FATAL_ERROR "" )
  endif ( OpenObex_FIND_REQUIRED )
endif ( NOT OpenObex_FOUND )
#message ( STATUS "OpenObex: ${OpenObex_FOUND}" )

### Local Variables:
### mode: cmake
### End:
