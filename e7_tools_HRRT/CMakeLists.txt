# Note that all subdirs inherit this very long list.
# Might be worth doing them individually in each subdir
set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libecat7
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libimage
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libinterfile
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libparser
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libsinogram
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libfft
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libipc
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libatten
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libfilter
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libconvert
	)

if (MSVC)
	set (LIBSCATTER 'libscatter')
else()
	set (LIBSCATTER 'scatter')
endif()

# find_library (LIBSCATTER   NAME scatter    PATHS "${CMAKE_SOURCE_DIR}/libscatter")
find_library (LIBSCATTER   NAME libscatter    PATHS "${CMAKE_SOURCE_DIR}/libscatter")
message("LIBSCATTER in root is ${LIBSCATTER}")

add_subdirectory(libatten)
add_subdirectory(libcommon)
add_subdirectory(libconvert)
add_subdirectory(libecat7)
add_subdirectory(libexception)
add_subdirectory(libfft)
add_subdirectory(libfilter)
add_subdirectory(libimage)
add_subdirectory(libinterfile)
add_subdirectory(libipc)
add_subdirectory(libparser)
add_subdirectory(libsinogram)
add_subdirectory(e7_atten_prj)
add_subdirectory(e7_fwd_prj)
add_subdirectory(e7_sino_prj)

target_include_directories (atten     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (common    PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (convert   PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (ecat7     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (exception PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (fft       PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (filter    PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (image     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (interfile PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (ipc       PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (parser    PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (sinogram  PUBLIC ${LIB_INCLUDE_DIRS})

