# This file is intended for use by library developers/maintainers.
# It declares targets for unit tests and benchmarks.
# If you only want to use the library consume lib/CMakeLists.txt instead.

cmake_minimum_required(VERSION 3.12)

project(vmcontainer CXX)

enable_testing()

# For CMAKE_INSTALL_<LIBDIR, INLCUDEDIR, BINDIR, DATADIR>
include(GNUInstallDirs)

add_subdirectory(lib)

add_subdirectory(benchmarks)
add_subdirectory(tests)

install(TARGETS vmcontainer
    EXPORT vmcontainer-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

export(
    TARGETS vmcontainer
    NAMESPACE vmcontainer::
    FILE "${PROJECT_BINARY_DIR}/vmcontainer-config.cmake"
)

install(EXPORT vmcontainer-config
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/vmcontainer"
    NAMESPACE vmcontainer::
)
