
cmake_minimum_required(VERSION 2.8)
project ( obexftp C )

#
# The project version
#
set ( VERSION_MAJOR 0 )
set ( VERSION_MINOR 24 )
set ( VERSION "${VERSION_MAJOR}.${VERSION_MINOR}" )

#
# The path for our own CMake modules
#
set ( CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/CMakeModules
)

#
# Define the default build type
#
set ( CMAKE_CONFIGURATION_TYPES "Release;Debug"
      CACHE STRING "" FORCE )
if ( NOT CMAKE_BUILD_TYPE )
  set ( CMAKE_BUILD_TYPE Release
        CACHE STRING "Build type" FORCE )
endif ( NOT CMAKE_BUILD_TYPE )

include ( MaintainerMode )
include ( GNUInstallDirs )

#
# define how to build libraries
#
option ( BUILD_SHARED_LIBS "Build shared libraries" ON )


include_directories (
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/includes
)

add_subdirectory ( bfb )
add_subdirectory ( multicobex )
add_subdirectory ( obexftp )
add_subdirectory ( apps )
add_subdirectory ( swig )
add_subdirectory ( doc )
#examples

set ( prefix      "${CMAKE_INSTALL_PREFIX}" )
set ( exec_prefix "\${prefix}" )
set ( libdir      "\${prefix}/${CMAKE_INSTALL_LIBDIR}" )
set ( includedir  "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}" )
set ( REQUIRES    "bluetooth openobex" )
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
