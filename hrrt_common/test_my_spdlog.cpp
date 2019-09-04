// $ g++ -c --std=c++14 -I../spdlog/include -I../hrrt_commonx/  my_spdlog.cpp ../hrrt_common/hrrt_util.cpp test_my_spdlog.cpp -Lfmt
// $ g++ -o test_my_spdlog hrrt_util.o my_spdlog.o test_my_spdlog.o /home/ahc/BIN/hrrt_open_2011/build/fmt-5.1.0/libfmt.a -lboost_system

#include <iostream>

#include "my_spdlog.hpp"

int main(int argc, char **argv) {
	my_spdlog::init_logging(argv[0]);
	LOG_ERROR("Test of LOG_ERROR");
	LOG_INFO("Test of LOG_INFO");
	LOG_DEBUG("Test of LOG_DEBUG");
	LOG_TRACE("Test of LOG_TRACE");
	LOG_EXIT("Test of LOG_EXIT");
	LOG_INFO("Test of LOG_INFO again");
	return 0;
}