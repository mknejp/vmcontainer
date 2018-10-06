find_package(Catch2 CONFIG REQUIRED)

add_executable(
  vmcontainer.test

  detail/algorithms.cpp
  detail/value_init_when_moved_from.cpp
  instantiations.cpp
  main.cpp
  pinned_vector/access.cpp
  pinned_vector/cons.cpp
  pinned_vector/contiguous.cpp
  pinned_vector/iterators.cpp
  vm/page_stack.cpp
  vm/reservation.cpp

  allocator_mocks.hpp
)

target_link_libraries(vmcontainer.test PRIVATE vmcontainer Catch2::Catch2)
target_include_directories(vmcontainer.test PRIVATE .)

add_test(NAME vmcontainer.test COMMAND vmcontainer.test)
