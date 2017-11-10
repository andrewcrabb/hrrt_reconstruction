# Create a library called "gen_delays" which includes the source file "gen_delays.cxx".
# The extension is already found. Any number of sources could be listed here.
add_library (gen_delays gen_delays.cpp geometry_info.cpp lor_sinogram_map.cpp segment_info.cpp)

# Make sure the compiler can find include files for our gen_delays library
# when other libraries or executables link to gen_delays
target_include_directories (gen_delays PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
