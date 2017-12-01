# cmake_minimum_required (VERSION 3.7)

add_library (hrrt_common hrrt_util.cpp)

target_include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS hrrt_common
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
