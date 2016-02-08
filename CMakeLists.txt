
cmake_minimum_required( VERSION 3.1 FATAL_ERROR )
cmake_policy ( VERSION 3.1 )
project ( obexftp
  VERSION 0.24.1
  LANGUAGES C
)

#
# The path for our own CMake modules
#
set ( CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/CMakeModules
)

include ( MaintainerMode )
include ( GNUInstallDirs )

set ( ENABLE_DEBUG 0 CACHE STRING "Set DEBUG level" )
set_property ( CACHE ENABLE_DEBUG PROPERTY STRINGS 0 1 2 3 4 )
add_definitions( -DOBEXFTP_DEBUG=${ENABLE_DEBUG} )

#
# define how to build libraries
#
option ( BUILD_SHARED_LIBS "Build shared libraries" ON )

#
# Set the required C standard: C99
#
set ( CMAKE_C_STANDARD 99 )
set ( CMAKE_C_STANDARD_REQUIRED TRUE )

include_directories (
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/includes
)

#
# many parts of this project need the OpenObex includes
#
find_package ( OpenObex ${OpenObex_FORCE_VERSION} QUIET NO_MODULE)
if ( NOT OpenObex_FOUND )
  find_package ( OpenObex REQUIRED )
endif ( NOT OpenObex_FOUND )
include_directories ( ${OpenObex_INCLUDE_DIRS} )

#
# some code is only included if bluetooth is available
#
find_package ( Bluetooth )
if ( Bluetooth_FOUND )
  include_directories ( ${Bluetooth_INCLUDE_DIRS} )
  add_definitions ( -DHAVE_BLUETOOTH -DHAVE_SDP )
endif ( Bluetooth_FOUND )

add_subdirectory ( bfb )
add_subdirectory ( multicobex )
add_subdirectory ( obexftp )
add_subdirectory ( apps )
add_subdirectory ( fuse )
add_subdirectory ( swig )
add_subdirectory ( doc )
#examples

set ( prefix      "${CMAKE_INSTALL_PREFIX}" )
set ( exec_prefix "\${prefix}" )
set ( libdir      "\${prefix}/${CMAKE_INSTALL_LIBDIR}" )
set ( includedir  "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}" )
set ( VERSION     "${obexftp_VERSION}" )
set ( REQUIRES    "bluez openobex" )
configure_file (
  ${CMAKE_CURRENT_SOURCE_DIR}/obexftp.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/obexftp.pc
  @ONLY
)

if ( NOT CMAKE_INSTALL_PKGCONFIGDIR )
  set ( CMAKE_INSTALL_PKGCONFIGDIR ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        CACHE PATH "Where to install .pc files to" FORCE )
endif ( NOT CMAKE_INSTALL_PKGCONFIGDIR )
mark_as_advanced ( CMAKE_INSTALL_PKGCONFIGDIR )

install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/obexftp.pc
  DESTINATION ${CMAKE_INSTALL_PKGCONFIGDIR}
  COMPONENT devel
)

#
# The following adds CPack support
#
set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "ObexFTP" )
set ( CPACK_PACKAGE_VENDOR "The OpenObex Development Team" )

set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt" )
set ( CPACK_RESOURCE_FILE_README  "${CMAKE_CURRENT_SOURCE_DIR}/README" )

set ( CPACK_PACKAGE_VERSION_MAJOR "${obexftp_VERSION_MAJOR}")
set ( CPACK_PACKAGE_VERSION_MINOR "${obexftp_VERSION_MINOR}")
set ( CPACK_PACKAGE_VERSION_PATCH "${obexftp_VERSION_PATCH}")
set ( CPACK_PACKAGE_VERSION       "${obexftp_VERSION}" )

if ( UNIX )
  set ( CPACK_GENERATOR "TGZ" )
  set ( CPACK_SOURCE_GENERATOR "TGZ" )

elseif ( WIN32 )
  #
  # For NSIS, install from http://nsis.sf.net.
  # For ZIP, install e.g. info-zip from http://www.info-zip.org.
  #
  set ( CPACK_GENERATOR "ZIP;NSIS" )
  set ( CPACK_SOURCE_GENERATOR "ZIP" )
endif ( UNIX )
set ( CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/\\\\.git/"
  "/\\\\.gitignore$"
  "~$"
)

# this must _follow_ the settings!
include ( CPack )
