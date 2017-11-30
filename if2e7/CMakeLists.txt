set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	# ${CMAKE_SOURCE_DIR}/ecatx
	)

# find_library (LIBECATX    NAME ecatx    PATHS "${CMAKE_BINARY_DIR}/ecatx")


add_executable (if2e7 if2e7.c splitpath.c interfile_reader.c calibration_table.c)
# target_link_libraries (if2e7 LINK_PUBLIC ${LIBECATX} m)
target_link_libraries (if2e7 LINK_PUBLIC ecatx m)
target_include_directories (if2e7 PUBLIC ${LIB_INCLUDE_DIRS})

install(TARGETS if2e7
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
