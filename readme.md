# HRRT Open 2011

## Updates 2017-18

* Migrate C and old C++ code to C++11
	* `char *` to `std::string`
	* Arrays to `std::vector`
	* Command line argument parsing
	* `printf` to logging library
	* Timing to Boost
* Version control: Git
* Add tests

## Technologies Used

* [CMake](https://cmake.org/)
* [Boost](https://www.boost.org/)
	* String operations: upcase
	* Times
	* Xpressive: Regular expressions
	* [ProgramOptions](https://theboostcpplibraries.com/boost.program_options): Command line parsing.  Also consider [cxxopts](https://github.com/jarro2783/cxxopts)
	* [Timer](https://theboostcpplibraries.com/boost.timer)
* [fmt](http://fmtlib.net/latest/index.html) formatting library
* [catch2](https://github.com/catchorg/Catch2) test framework
* [spdlog](https://github.com/gabime/spdlog) logger
* [Git](https://git-scm.com/) version control

## Notes from pre-conversion system

* invert_air must be copied from the AIR directory to the path of all the e7_ programs.
* Make sure  libraries/hrrt_rebinner.lut is executable
* In folder TX_TV3DReg, do the following (turn display mode off)
	* `g++ TX_TV3DReg.cpp -I . -D cimg_OS=0 -D cimg_display_type=0 -o TX_TV3DReg`
* The motion programs use the gnuplot command. The package needs to be installed.


