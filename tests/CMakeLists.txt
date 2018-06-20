find_package(Catch2 CONFIG REQUIRED)

add_executable(
  pinned_vector.test

  instantiations.cpp
  main.cpp
  virtual_memory_page_stack.cpp
  virtual_memory_reservation.cpp

  allocator_mocks.hpp
)

target_link_libraries(pinned_vector.test PRIVATE pinned_vector Catch2::Catch)

add_test(NAME pinned_vector.test COMMAND pinned_vector.test)
