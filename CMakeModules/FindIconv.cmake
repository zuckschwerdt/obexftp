
find_path ( ICONV_INCLUDE_DIR iconv.h )
mark_as_advanced ( ICONV_INCLUDE_DIR )

set ( ICONV_INCLUDE_DIRS ${ICONV_INCLUDE_DIR} )

if ( ICONV_INCLUDE_DIRS )
  include ( CheckFunctionExists )

  unset ( CMAKE_REQUIRED_FLAGS )
  unset ( CMAKE_REQUIRED_DEFINITIONS )
  set ( CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIRS} )
  unset ( CMAKE_REQUIRED_LIBRARIES )
  check_function_exists ( iconv_open ICONV_FOUND )

  if ( NOT ICONV_FOUND )
    find_library ( iconv_LIBRARY iconv )
    if ( iconv_LIBRARY )
      set ( CMAKE_REQUIRED_LIBRARIES ${iconv_LIBRARY} )
      check_function_exists ( iconv_open ICONV_FOUND )
      if ( ICONV_FOUND )
	set ( ICONV_LIBRARIES ${iconv_LIBRARY} )
      endif ( ICONV_FOUND )
    endif ( iconv_LIBRARY )
  endif ( NOT ICONV_FOUND )
endif ( ICONV_INCLUDE_DIRS )

if ( NOT ICONV_FOUND )
  if ( Iconv_REQUIRED )
    message ( FATAL_ERROR "Iconv not found" )
  endif ( Iconv_REQUIRED )
endif ( NOT ICONV_FOUND )
