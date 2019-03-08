# cmake_minimum_required (VERSION 3.7)
# set (GCC_COMPILE_FLAGS "-std=c++11")
# set (GCC_COMPILE_FLAGS "-std=c++14")
add_library (hrrt_common hrrt_util.cpp)
# target_include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/cheader_lib )
target_include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Boost REQUIRED)

# ahc 1/29/19 Moved this from a library-specific subdirectory to make its own library
# Exception definitions, even for matrix maths, are all dumped in e7_tools_hrrt by some mad logic.
add_library (math_matrix math_matrix.cpp)
target_include_directories (math_matrix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception ${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon)

add_library (my_spdlog my_spdlog.cpp)
target_link_libraries(my_spdlog hrrt_common)

add_library(date_time date_time.cpp)

# add_executable(test_util test_util.cpp)
# # link_directories(${PROJECT_SOURCE_DIR}/cheader_lib)
# # target_link_libraries(test_util hrrt_common fmt boost_system boost_filesystem boost_date_time cheader)
# # target_link_libraries(test_util  cheader boost_system boost_date_time boost_filesystem my_spdlog)
# target_link_libraries(test_util  boost_system boost_date_time boost_filesystem my_spdlog)
# target_include_directories (test_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/lmhistogram_mp)

install(TARGETS hrrt_common
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# # Make test executable
# set(TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp)
# add_executable(tests ${TEST_SOURCES})
# target_include_directories (tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(tests LINK_PUBLIC Catch hrrt_common cheader fmt boost_system boost_filesystem boost_date_time)

set(TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/hrrt_common_tests.cpp)
add_executable(hrrt_common_tests ${TEST_SOURCES})
target_include_directories (hrrt_common_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hrrt_common_tests LINK_PUBLIC Catch hrrt_common my_spdlog date_time fmt boost_system boost_filesystem boost_date_time)

enable_testing()
add_test(NAME TestingHrrtCommon COMMAND hrrt_common_tests)
