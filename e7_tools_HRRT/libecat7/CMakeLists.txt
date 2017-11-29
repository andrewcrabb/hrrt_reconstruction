add_library (ecat7
		data_changer.cpp ecat7_attenuation.cpp ecat7.cpp ecat7_directory.cpp ecat7_image.cpp 
		ecat7_mainheader.cpp ecat7_matrix_abc.cpp ecat7_norm3d.cpp ecat7_norm.cpp ecat7_polar.cpp 
		ecat7_scan3d.cpp ecat7_scan.cpp ecat7_utils.cpp ecat_tmpl.cpp)

install(TARGETS ecat7
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
