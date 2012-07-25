
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

if ( ICONV_FOUND )
  set ( ICONV_CONST_TEST_SOURCE "
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, const char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
")
  unset ( CMAKE_REQUIRED_FLAGS )
  unset ( CMAKE_REQUIRED_DEFINITIONS )
  set ( CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIRS} )
  unset ( CMAKE_REQUIRED_LIBRARIES )
  check_c_source_compiles ( "${ICONV_CONST_TEST_SOURCE}" ICONV_USES_CONST )
endif ( ICONV_FOUND )

if ( NOT ICONV_FOUND )
  if ( Iconv_REQUIRED )
    message ( FATAL_ERROR "Iconv not found" )
  endif ( Iconv_REQUIRED )
endif ( NOT ICONV_FOUND )
