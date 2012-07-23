option ( USE_MAINTAINER_MODE "Enable some stuff only relevant to developers" OFF )
if ( USE_MAINTAINER_MODE )
  if ( CMAKE_COMPILER_IS_GNUCC )
    set ( MAINTAINER_MODE_WARN_FLAGS
      all
      extra
      no-unused-parameter
      no-missing-field-initializers
      declaration-after-statement
      missing-declarations
      redundant-decls
      cast-align
      error
    )
    set ( MAINTAINER_MODE_FLAGS
#      pedantic
#      std=c99
    )
    foreach ( flag ${MAINTAINER_MODE_WARN_FLAGS} )
      list ( APPEND MAINTAINER_MODE_FLAGS "W${flag}" )
    endforeach ( flag )

    foreach ( flag ${MAINTAINER_MODE_FLAGS} )
      set ( cflag "-${flag}" )
      set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}" )
      foreach ( type DEBUG RELEASE MINSIZEREL RELWITHDEBINFO )
	set ( CMAKE_C_FLAGS_${type} "${CMAKE_C_FLAGS_${type}} ${cflag}" )
      endforeach ( type )
    endforeach ( flag )

  elseif ( MSVC )
    set ( MAINTAINER_MODE_FLAGS
      W3
      WX
    )
    foreach ( flag ${MAINTAINER_MODE_FLAGS} )
      set ( cflag "/${flag}" )
      set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}" )
      foreach ( type DEBUG RELEASE MINSIZEREL RELWITHDEBINFO )
	set ( CMAKE_C_FLAGS_${type} "${CMAKE_C_FLAGS_${type}} ${cflag}" )
      endforeach ( type )
    endforeach ( flag )
  endif ( CMAKE_COMPILER_IS_GNUCC )

#  foreach ( type DEBUG RELEASE MINSIZEREL RELWITHDEBINFO )
#    message ( "CMAKE_C_FLAGS_${type} set to \"${CMAKE_C_FLAGS_${type}}\"" )
#  endforeach ( type )
endif ( USE_MAINTAINER_MODE )
