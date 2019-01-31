set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/ecatx
	)

add_executable (ecat2dicom ecat2dicom.cpp )
target_link_libraries (ecat2dicom LINK_PUBLIC )
target_include_directories (ecat2dicom PUBLIC ${LIB_INCLUDE_DIRS})

install(TARGETS ecat2dicom
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
