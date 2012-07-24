# - Find Fuse (file system in userspace)
#
# It will use PkgConfig if present and supported, else search
# it on its own.
#
# The following standard variables get defined:
#  Fuse_FOUND:          true if fuse was found
#  Fuse_VERSION_STRING: the version of fuse, if any
#  Fuse_INCLUDE_DIRS:   the directory that contains the include file
#  Fuse_LIBRARIES:      full path to the libraries
#  Fuse_DEFINITIONS     additional compiler flags

find_package ( PkgConfig )
if ( PKG_CONFIG_FOUND )
  if ( Fuse_FIND_VERSION )
    pkg_check_modules ( PKGCONFIG_FUSE fuse=${Fuse_FIND_VERSION} )
  else ( Fuse_FIND_VERSION )
    pkg_check_modules ( PKGCONFIG_FUSE fuse )
  endif ( Fuse_FIND_VERSION )
endif ( PKG_CONFIG_FOUND )

if ( PKGCONFIG_FUSE_FOUND )
  set ( Fuse_FOUND ${PKGCONFIG_FUSE_FOUND} )
  set ( Fuse_VERSION_STRING ${PKGCONFIG_FUSE_VERSION} )
  set ( Fuse_DEFINITIONS ${PKGCONFIG_FUSE_CFLAGS_OTHER} )
  set ( Fuse_INCLUDE_DIRS ${PKGCONFIG_FUSE_INCLUDE_DIRS} )
  foreach ( i ${PKGCONFIG_FUSE_LIBRARIES} )
    find_library ( ${i}_LIBRARY
                   NAMES ${i}
                   PATHS ${PKGCONFIG_FUSE_LIBRARY_DIRS}
                 )
    if ( ${i}_LIBRARY )
      list ( APPEND Fuse_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  
else ( PKGCONFIG_FUSE_FOUND )
  find_patch ( Fuse_INCLUDE_DIRS
    NAMES
      fuse/fuse.h
    PATH_SUFFIXES
      include    
  )
  mark_as_advanced ( Fuse_INCLUDE_DIRS )

  find_library ( fuse_LIBRARY
    NAMES
      fuse
  )
  mark_as_advanced ( fuse_LIBRARY )
  if ( fuse_LIBRARY )
    set ( Fuse_LIBRARIES ${fuse_LIBRARY} )
  endif ( fuse_LIBRARY )

  set ( Fuse_DEFINITIONS -D_FILE_OFFSET_BITS=64 )

  if ( Fuse_INCLUDE_DIRS AND Fuse_LIBRARIES )
    set ( Fuse_FOUND true )
  endif ( Fuse_INCLUDE_DIRS AND Fuse_LIBRARIES )
endif ( PKGCONFIG_FUSE_FOUND )

if ( NOT Fuse_FOUND )
  if ( NOT Fuse_FIND_QUIETLY )
    message ( STATUS "Fuse not found." )
  endif ( NOT Fuse_FIND_QUIETLY )
  if ( Fuse_FIND_REQUIRED )
    message ( FATAL_ERROR "" )
  endif ( Fuse_FIND_REQUIRED )
endif ( NOT Fuse_FOUND )
