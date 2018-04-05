set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libecat7
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libimage
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libinterfile
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libparser
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libsinogram
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libfft
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libipc
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libatten
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libfilter
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libconvert
	)

add_executable (compute_norm matrix.cpp compute_norm.cpp gantryinfo.cpp self_norm.cpp det_to_ve.cpp 
	gapfill.cpp plot.cpp  sino_to_crys.cpp dwell_sino.cpp global_tmpl.cpp polyfit.cpp fit_dwell.cpp make_norm.cpp polyfitw.cpp)

target_link_libraries (compute_norm LINK_PUBLIC )

# add_definitions(-D GSMOOTH)
target_include_directories (compute_norm PUBLIC ${LIB_INCLUDE_DIRS})
install(TARGETS compute_norm
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
