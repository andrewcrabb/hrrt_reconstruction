add_library (norm_process SHARED 
	norm_process.cpp cal_sen.cpp fan_sum.cpp get_gs.cpp Header.cpp inter.cpp MarkTime.cpp rod_dwell.cpp rotation_dwell_sino.cpp	)

target_include_directories (norm_process PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS norm_process
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
