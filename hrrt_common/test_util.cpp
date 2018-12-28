#include <iostream>
#include <vector>
#include <boost/io/ios_state.hpp>
#include <boost/xpressive/xpressive.hpp>
#include "hrrt_util.hpp"
#include "CHeader.hpp"

// #define FMT_HEADER_ONLY
#include <fmt/format.h>

using std::string;
using std::cout;
using std::endl;

namespace bg = boost::gregorian;
namespace bt = boost::posix_time;

int test_date(const string &str) {
	bt::ptime t0;
	bool ret = parse_interfile_date(str, t0);
	cout << "parse_interfile_date(" << str << ") returned " << ret << ": " << t0 << endl;
	return 0;
}

int test_time (const string &str) {
	bt::ptime pt;
	boost::io::ios_all_saver ias( cout );
	bt::time_facet* facet(new bt::time_facet("%H%M%S"));
	std::cout.imbue(std::locale(std::cout.getloc(), facet));
	parse_interfile_time(str, pt);
	cout << "test_time(" << str << "): " << pt << endl;
	return 0;
}

// static int get_dose_delay(CHeader &hdr, int &t) {
// 	using namespace boost::xpressive;
// 	string str;

// 	if (hdr.Readchar(CHeader::DOSE_ASSAY_DATE, str)) {
// 		smatch match;
// 		sregex reg = sregex::compile(CHeader::DOSE_ASSAY_DATE + "\\s+:=\\s+(?P<value>.+)$");
// 		if (regex_match(str, match, reg)) {
// 			bt::ptime t;
// 			parse_interfile_time(match["key"], t);
// 		}
// 	}
// }


int main() {
	std::vector<string> dates = {"18:09:2017"};
	for (auto &date : dates) {
		test_date(date);
	}
	std::vector<string> times = {"10:30:00", "10:35:00"};
	for (auto &time : times) {
		test_time(time);
	}

	bt::ptime t0, t1;
	parse_interfile_time("10:30:00", t0);
	parse_interfile_time("10:35:00", t1);
	bt::time_duration diff = t1 - t0;
	cout << "diff: " << diff.total_seconds() << endl;

	string sdate = "!study date (dd:mm:yryr) := 18:09:2017";
	string key, value;
	if (!parse_interfile_line(sdate, key, value)) {
		std::cout << fmt::format("key '{key}' value '{value}'\n", fmt::arg("key", key), fmt::arg("value", value)) << std::endl;
	}
}