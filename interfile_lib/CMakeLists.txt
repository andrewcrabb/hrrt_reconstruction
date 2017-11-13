# cmake_minimum_required (VERSION 3.7)

add_library (interfile SHARED interfile_reader.c)

target_include_directories (interfile PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS interfile
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
