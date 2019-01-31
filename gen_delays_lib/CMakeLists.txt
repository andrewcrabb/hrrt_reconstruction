# cmake_minimum_required (VERSION 3.7)

add_library (gen_delays gen_delays_lib.cpp geometry_info.cpp lor_sinogram_map.cpp segment_info.cpp)

target_include_directories (gen_delays PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/gen_delays)
target_link_libraries(gen_delays LINK_PUBLIC hrrt_common )
install(TARGETS gen_delays
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
