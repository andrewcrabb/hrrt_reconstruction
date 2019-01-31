add_library (convert
		dift.cpp bckprj3d.cpp fbp.cpp fwdprj3d.cpp 
		osem3d.cpp red_client.cpp convert.cpp atten_reco.cpp scatter.cpp)

install(TARGETS convert
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
