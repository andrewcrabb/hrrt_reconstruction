# add_subdirectory(e7_atten_prj)

add_library (libconvert SHARED 
		dift.cpp matrix.cpp bckprj3d.cpp fbp.cpp fwdprj3d.cpp 
		osem3d.cpp red_client.cpp convert.cpp atten_reco.cpp scatter.cpp)

install(TARGETS libconvert
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
