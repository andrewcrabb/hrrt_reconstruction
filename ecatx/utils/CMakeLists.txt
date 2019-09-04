# cmake_minimum_required (VERSION 3.7)

# set (GCC_COMPILE_FLAGS "-std=gnu99")
# set (GCC_COMPILE_FLAGS "-std=c99")
# add_definitions(${GCC_COMPILE_FLAGS})

# target_include_directories(${PROJECT_SOURCE_DIR}/ecatx)
link_directories(${PROJECT_SOURCE_DIR}/ecatx)
link_libraries(ecatx m)

add_executable(sum_frames  sum_frames.cpp)
add_executable(show_header show_header.cpp)
add_executable(matlist     matlist.cpp)
add_executable(fix_mh      fix_mh.cpp)
add_executable(matcopy     matcopy.cpp)
add_executable(cti2analyze cti2analyze.cpp)

# target_link_libraries(matlist ecatx m)
# target_link_libraries(show_header ecatx m)

# target_include_directories (interfile PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# install(TARGETS interfile
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
