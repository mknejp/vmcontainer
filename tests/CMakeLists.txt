find_package(Catch2 CONFIG REQUIRED)

file(GLOB_RECURSE vmcontainer.test-sources CONFIGURE_DEPENDS *.cpp)
add_executable(vmcontainer.test ${vmcontainer.test-sources})

target_link_libraries(vmcontainer.test PRIVATE vmcontainer Catch2::Catch2)
target_include_directories(vmcontainer.test PRIVATE .)

add_test(NAME vmcontainer.test COMMAND vmcontainer.test)
