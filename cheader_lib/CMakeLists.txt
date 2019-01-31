find_package(Boost REQUIRED)

add_library (cheader CHeader.cpp)
target_include_directories (cheader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (cheader LINK_PUBLIC hrrt_common fmt)
target_link_libraries(cheader PRIVATE fmt-header-only)

# Make test executable
# Moved this to main CMakeLists
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/catch_main.cpp)
add_executable(tests ${TEST_SOURCES})
target_include_directories (tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests LINK_PUBLIC Catch hrrt_common cheader fmt boost_system boost_filesystem boost_date_time)

enable_testing()
add_test(NAME TestingOneTwoThree COMMAND tests)

install(TARGETS cheader
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
