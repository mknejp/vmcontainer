add_executable(
    pinned_vector_tests
    main.cpp
    instantiations.cpp)

target_link_libraries(pinned_vector_tests PRIVATE pinned_vector)

add_test(NAME pinned_vector_tests COMMAND pinned_vector_tests)
