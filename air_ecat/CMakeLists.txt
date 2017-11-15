add_library (air_ecat SHARED 
	ecat2air.c air2ecat.c ecat_map_value.c ecat_do_alignlinear.c ecat_do_reslice.c
)

target_include_directories (air_ecat PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ../AIR5.3.0/src)
install(TARGETS air_ecat
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
