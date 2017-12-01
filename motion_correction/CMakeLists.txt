set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/hrrt_common
	)

# add_definitions(-std=gnu99)
add_definitions(-D _POSIX_C_SOURCE=2)
add_executable (motion_qc motion_qc.cpp)
add_executable (motion_correct_recon motion_correct_recon.cpp qmatrix.cpp)
add_executable (MAF_join MAF_join.cpp)
add_executable (sino_transform sino_transform.cpp matpkg.cpp mm_malloc.cpp nr_utils.cpp)


target_link_libraries (motion_qc LINK_PUBLIC ecatx hrrt_common)
target_include_directories (motion_qc PUBLIC ${LIB_INCLUDE_DIRS})

install(TARGETS motion_qc motion_correct_recon MAF_join sino_transform
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
