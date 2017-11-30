add_library (motion_distance 
	motion_distance.cpp )

target_include_directories (motion_distance PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ../AIR5.3.0/src)
install(TARGETS motion_distance
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
