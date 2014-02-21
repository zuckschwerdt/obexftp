
find_package ( Fuse )

if ( Fuse_FOUND )
  include_directories ( ${Fuse_INCLUDE_DIRS} )
  add_definitions ( ${Fuse_DEFINITIONS} )

  find_library ( xmlparse_LIBRARY xmlparse )
  if ( NOT xmlparse_LIBRARY )
    message ( FATAL_ERROR "xmlparse library from the expat package was not found!" )
  endif ( NOT xmlparse_LIBRARY )
  mark_as_advanced ( xmlparse_LIBRARY )

  add_executable ( obexfs
    obexfs.c
  )

  target_link_libraries ( obexfs
    obexftp
    ${Fuse_LIBRARIES}
    ${xmlparse_LIBRARY}
  )

  add_executable ( obexautofs
    obexautofs.c
  )

  target_link_libraries ( obexautofs
    obexftp
    ${Fuse_LIBRARIES}
  )

  install ( TARGETS obexfs obexautofs
    RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT applications
  )
endif ( Fuse_FOUND )
