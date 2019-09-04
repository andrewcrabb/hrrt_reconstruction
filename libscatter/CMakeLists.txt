# add_subdirectory(e7_atten_prj)

add_library (scatter)

# target_include_directories (atten PUBLIC ${CMAKE_SOURCE_DIR} 
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libecat7
# 	)
install(TARGETS scatter
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
