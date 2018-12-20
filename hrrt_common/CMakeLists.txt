# cmake_minimum_required (VERSION 3.7)
set (GCC_COMPILE_FLAGS "-std=c++11")
add_library (hrrt_common hrrt_util.cpp)
target_include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/lmhistogram_mp  ${CMAKE_SOURCE_DIR}/cheader_lib )
include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Boost REQUIRED)

add_executable(test_util test_util.cpp)
target_link_libraries(test_util hrrt_common fmt boost_system)
target_include_directories (test_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/lmhistogram_mp)

install(TARGETS hrrt_common
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
