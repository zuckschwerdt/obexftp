
find_package ( Fuse )

if ( Fuse_FOUND )
  include_directories ( ${Fuse_INCLUDE_DIRS} )
  add_definitions ( ${Fuse_DEFINITIONS} )

  find_package ( EXPAT REQUIRED )
  include_directories ( ${EXPAT_INCLUDE_DIRS} )

  add_executable ( obexfs
    obexfs.c
  )

  target_link_libraries ( obexfs
    obexftp
    ${Fuse_LIBRARIES}
    ${EXPAT_LIBRARIES}
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
