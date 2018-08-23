# HRRT Open 2011

## Updates

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
* [catch](https://github.com/catchorg/Catch2) test framework
* [spdlog](https://github.com/gabime/spdlog) logger