add_library (exception
		exception.cpp logging.cpp semaphore_al.cpp timedate.cpp)

target_include_directories (hrrt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon )

install(TARGETS exception
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
