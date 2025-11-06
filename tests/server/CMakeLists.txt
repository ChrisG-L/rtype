project("tests_server" VERSION 0.0.1 LANGUAGES CXX)

add_executable(tests_server
    main.cpp
)

target_link_libraries(tests_server PRIVATE
    Boost::system
    GTest::gtest
    GTest::gtest_main
)