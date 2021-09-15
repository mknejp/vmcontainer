cmake_minimum_required(VERSION 3.12)

project(vmcontainer CXX)

file(GLOB_RECURSE vmcontainer-sources CONFIGURE_DEPENDS *.cpp)
add_library(vmcontainer ${vmcontainer-sources})
add_library(vmcontainer::vmcontainer ALIAS vmcontainer)

# For CMAKE_INSTALL_<LIBDIR, INLCUDEDIR, BINDIR, DATADIR>
include(GNUInstallDirs)

target_include_directories(vmcontainer PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(vmcontainer PUBLIC cxx_std_14)
set_target_properties(vmcontainer PROPERTIES DEBUG_POSTFIX -d)

install(
  DIRECTORY include/vmcontainer
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp"
)

install(TARGETS vmcontainer
  EXPORT vmcontainer-config
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Since there is two projects, we need to export into the parent directory
export(
  TARGETS vmcontainer
  NAMESPACE vmcontainer::
  FILE "${PROJECT_BINARY_DIR}/../vmcontainer-config.cmake"
)

install(EXPORT vmcontainer-config
  DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/vmcontainer"
  NAMESPACE vmcontainer::
)
