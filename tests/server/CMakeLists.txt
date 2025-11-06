project("server_tests" VERSION 0.0.1 LANGUAGES CXX)

add_executable(server_tests
    main.cpp
)

target_link_libraries(server_tests PRIVATE
    Boost::system
    GTest::gtest
    GTest::gtest_main
)