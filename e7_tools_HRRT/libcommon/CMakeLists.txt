# add_subdirectory(e7_atten_prj)

add_library (common 
	vecmath.cpp unique_name.cpp thread_wrapper.cpp syngo_msg.cpp str_tmpl.cpp 
	stopwatch.cpp rng.cpp raw_io.cpp hwinfo.cpp gm.cpp global_tmpl.cpp 
	gantryinfo.cpp stream_buffer.cpp swap_tmpl.cpp e7_common.cpp mem_ctrl.cpp)

install(TARGETS common
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
