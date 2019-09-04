# cmake_minimum_required (VERSION 3.7)

add_library (interfile_lib interfile_reader.cpp)

target_include_directories (interfile_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS interfile_lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
