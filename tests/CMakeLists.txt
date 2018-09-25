find_package(Catch2 CONFIG REQUIRED)

add_executable(
  vmcontainer.test

  instantiations.cpp
  main.cpp
  pinned_vector/iterators.cpp
  virtual_memory_page_stack.cpp
  virtual_memory_reservation.cpp

  allocator_mocks.hpp
)

target_link_libraries(vmcontainer.test PRIVATE vmcontainer Catch2::Catch2)

add_test(NAME vmcontainer.test COMMAND vmcontainer.test)
