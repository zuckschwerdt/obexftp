
find_package ( Fuse )

if ( Fuse_FOUND )
  include_directories ( ${Fuse_INCLUDE_DIRS} )
  add_definitions ( ${Fuse_DEFINITIONS} )

  add_executable ( obexfs
    obexfs.c
  )

  target_link_libraries ( obexfs
    obexftp
    ${Fuse_LIBRARIES}
  )


  add_executable ( obexautofs
    obexautofs.c
  )

  target_link_libraries ( obexautofs
    obexftp
    ${Fuse_LIBRARIES}
  )
endif ( Fuse_FOUND )
