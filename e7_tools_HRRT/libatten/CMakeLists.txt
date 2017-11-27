# add_subdirectory(e7_atten_prj)

add_library (libatten SHARED 
	Gauss_3D.cpp GM_3D.cpp PB_3D.cpp PB_TV_3D.cpp Projected_Image_3D.cpp 
	TV_3D.cpp Tx_PB_Gauss_3D.cpp Tx_PB_GM_3D.cpp Tx_PB_TV_3D.cpp Tx_PB_3D.cpp)

# target_include_directories (libatten PUBLIC ${CMAKE_SOURCE_DIR} 
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libecat7
# 	)
install(TARGETS libatten
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
