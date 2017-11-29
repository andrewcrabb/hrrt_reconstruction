set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/gen_delays_lib
	)

# ADD_EXECUTABLE(tsp_ascii_writer-bin ${ASCII_WRITER_SRC})
# SET_TARGET_PROPERTIES(tsp_ascii_writer-bin
#   PROPERTIES OUTPUT_NAME tsp_ascii_writer)

# Executable with same name as library
add_executable (gen_delays_main main.cpp )
set_target_properties(gen_delays_main
	PROPERTIES OUTPUT_NAME gen_delays)
target_link_libraries (gen_delays_main LINK_PUBLIC )
target_include_directories (gen_delays_main PUBLIC ${LIB_INCLUDE_DIRS})

install(TARGETS gen_delays_main
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
