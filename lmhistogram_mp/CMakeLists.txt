set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	# ${CMAKE_SOURCE_DIR}/ecatx
	)

# find_library (GEN_DELAYS NAME gen_delays PATHS "${CMAKE_BINARY_DIR}/gen_delays_lib")
# message("GEN_DELAYS is ${GEN_DELAYS}")

add_definitions(-D HRRT_REBINNER_LUT)
add_executable (lmhistogram_mp convert_span9.cpp dtc.cpp gantryinfo.cpp Header.cpp 
	histogram_mp.cpp lmhistogram_mp.cpp LM_Reader_mp.cpp LM_Rebinner_mp.cpp Flags.cpp)
# target_link_libraries (lmhistogram_mp LINK_PUBLIC ${GEN_DELAYS})
target_link_libraries (lmhistogram_mp LINK_PUBLIC gen_delays)
target_include_directories (lmhistogram_mp PUBLIC ${LIB_INCLUDE_DIRS})

install(TARGETS lmhistogram_mp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
