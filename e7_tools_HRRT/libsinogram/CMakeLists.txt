add_library (sinogram
		fore_fft.cpp randoms_smoothing.cpp rebin_x.cpp norm_ecat.cpp rebin_sinogram.cpp 
		ssr.cpp fore.cpp gapfill.cpp sino2d_3d.cpp sinogram_conversion.cpp sinogram_io.cpp)

install(TARGETS sinogram
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
