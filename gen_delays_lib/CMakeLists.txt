# cmake_minimum_required (VERSION 3.7)

set (GCC_COMPILE_FLAGS "-std=c99")

add_definitions(${GCC_COMPILE_FLAGS})

add_library (gen_delays SHARED gen_delays.cpp geometry_info.cpp lor_sinogram_map.cpp segment_info.cpp)

target_include_directories (gen_delays PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS gen_delays
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
