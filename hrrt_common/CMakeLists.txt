# cmake_minimum_required (VERSION 3.7)
# set (GCC_COMPILE_FLAGS "-std=c++11")
# set (GCC_COMPILE_FLAGS "-std=c++14")
add_library (hrrt_common hrrt_util.cpp)
target_include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/cheader_lib )
include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Boost REQUIRED)

# ahc 1/29/19 Moved this from a library-specific subdirectory to make its own library
# Exception definitions, even for matrix maths, are all dumped in e7_tools_hrrt by some mad logic.
add_library (math_matrix math_matrix.cpp)
target_include_directories (math_matrix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception ${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon)

add_executable(test_util test_util.cpp)
# link_directories(${PROJECT_SOURCE_DIR}/cheader_lib)
target_link_libraries(test_util hrrt_common fmt boost_system boost_date_time cheader)
target_include_directories (test_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/lmhistogram_mp)

install(TARGETS hrrt_common
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
