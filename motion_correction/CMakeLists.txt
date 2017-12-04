set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/hrrt_common
	${CMAKE_SOURCE_DIR}/AIR5.3.0/src
	)

find_library (LIBRPW   NAME RPW    PATHS "${CMAKE_SOURCE_DIR}/AIR5.3.0/src")

# add_definitions(-std=gnu99)
# add_definitions(-D _POSIX_C_SOURCE=2)
add_definitions(-D_XOPEN_SOURCE=600 -D _GLIBCXX_USE_CXX11_ABI=0)
add_executable (motion_qc motion_qc.cpp)
add_executable (motion_correct_recon motion_correct_recon.cpp qmatrix.cpp)
add_executable (MAF_join MAF_join.cpp)
add_executable (sino_transform sino_transform.cpp matpkg.cpp mm_malloc.cpp nr_utils.cpp)

foreach(EXECUTABLE motion_qc MAF_join motion_correct_recon sino_transform)
	target_link_libraries      (${EXECUTABLE} LINK_PUBLIC ecatx hrrt_common ${LIBRPW})
	target_include_directories (${EXECUTABLE} PUBLIC ${LIB_INCLUDE_DIRS})
endforeach(EXECUTABLE)

install(TARGETS motion_qc motion_correct_recon MAF_join sino_transform
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
