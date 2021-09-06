file(GLOB_RECURSE vmcontainer-sources CONFIGURE_DEPENDS *.cpp)
add_library(vmcontainer ${vmcontainer-sources})
add_library(vmcontainer::vmcontainer ALIAS vmcontainer)


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
