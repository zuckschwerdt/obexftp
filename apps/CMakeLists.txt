
add_definitions( -DVERSION=${VERSION} )

# always set this
add_definitions ( -DHAVE_USB )

add_executable ( obexftp_app obexftp.c )
target_link_libraries ( obexftp_app obexftp )
set_target_properties ( obexftp_app PROPERTIES
  OUTPUT_NAME obexftp
)


add_executable ( obexftpd_app obexftpd.c )
target_link_libraries ( obexftpd_app obexftp )
set_target_properties ( obexftpd_app PROPERTIES
  OUTPUT_NAME obexftpd
)

add_executable ( discovery_app discovery.c )
target_link_libraries ( discovery_app obexftp )
set_target_properties ( discovery_app PROPERTIES
  OUTPUT_NAME obexftp-discovery
)

install ( TARGETS obexftp_app obexftpd_app
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT applications
)
