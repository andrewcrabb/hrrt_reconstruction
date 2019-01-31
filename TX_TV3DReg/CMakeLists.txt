find_package(X11 REQUIRED)

# find_library(LIB_XLIB name Xlib)
# find_path(INCL_XLIB name X11/Xlib.h)
# message("LIB_XLIB is ${LIB_XLIB}")
# message("INCL_XLIB is ${INCL_XLIB}")

message("X11_FOUND is ${X11_FOUND}")
message("X11_INCLUDE_DIR is ${X11_INCLUDE_DIR}")
message("X11_LIBRARIES is ${X11_LIBRARIES}")


add_executable (TX_TV3DReg TX_TV3DReg.cpp )
target_link_libraries (TX_TV3DReg LINK_PUBLIC LIB_XLIB)
target_include_directories (TX_TV3DReg PUBLIC ${LIB_INCLUDE_DIRS})

install(TARGETS TX_TV3DReg
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
