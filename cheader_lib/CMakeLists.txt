find_package(Boost REQUIRED)

add_library (cheader_lib CHeader.cpp)
target_include_directories (cheader_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (cheader_lib LINK_PUBLIC hrrt_common fmt)
install(TARGETS cheader_lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
