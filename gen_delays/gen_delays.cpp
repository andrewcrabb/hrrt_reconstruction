// gen_delays.cpp : Defines the entry point for the console application.
/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
	10-DEC-2007: Modifications for compatibility with windelays.
  02-DEC-2008: Bug fix in searching hrrt_rebinner.lut path (library gen_delay_lib)
  02-DEC-2008: Added sw_version 1.0.1
  02-Jun-2009: Changed sw_version to 1.1
  21-JUL-2010: Bug fix in gen_delays_lib
  21-JUL-2010: Change sw_version to 1.2
*/

#include <boost/xpressive/xpressive.hpp>
#include <boost/program_options.hpp>
#include "spdlog/spdlog.h"
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "gen_delays.hpp"
#include "gen_delays_lib.hpp"
#include "hrrt_util.hpp"
#include "my_spdlog.hpp"

// static const char *sw_version = "HRRT_U 1.2";

namespace bf = boost::filesystem;
namespace bx = boost::xpressive;
namespace po = boost::program_options;


// Default values for globals that may be overridden by command line arguments
int g_num_elems = GeometryInfo::NUM_ELEMS;   // elements/projections
int g_num_views = GeometryInfo::NUM_VIEWS;
int g_span = 9;
int g_max_ringdiff = GeometryInfo::MAX_RINGDIFF;

float g_pitch = 0.0f;
float g_diam  = 0.0f;
float g_thick = 0.0f;
float g_tau   = 6.0e-9f;
float g_ftime = 1.0f;

// void on_rebinner(std::string const &instring) {
//   g_rebinner_lut_file = boost::filesystem::path(instring);
// }

void on_coincidence(std::string const &instring) {
  g_coincidence_histogram_file = boost::filesystem::path(instring);
}

void on_csingles(std::string const &instring) {
  g_crystal_singles_file = boost::filesystem::path(instring);
}

void on_delay(std::string const &instring) {
  g_delayed_coincidence_file = boost::filesystem::path(instring);
}

void on_outsing(std::string const &instring) {
  g_output_csings_file = boost::filesystem::path(instring);
}

/**
 * @brief      Parse sino size string
 * @param      instring  'elements/projections,views' (ints)
 */

void on_sino_size(std::string const &instring) {
  bx::sregex re_size = bx::sregex::compile("(?P<nelements>[0-9]+)\\,(<?P<nviews>[0-9]+)");
  bx::smatch match;

  if (bx::regex_match(instring, match, re_size)) {
    g_num_elems = boost::lexical_cast<int>(match["nelements"]);
    g_num_views = boost::lexical_cast<int>(match["nviews"]);
  } else {
    LOG_ERROR("Invalid sino size string: {}", instring);
    exit(1);
  }
}

/**
 * @brief      Parse span string
 * @param      instring  'span,maxrd' (ints)
 */

void on_span(std::string const &instring) {
  bx::sregex re_span = bx::sregex::compile("(?P<span>[0-9]+)\\,(<?P<ringdiff>[0-9]+)");
  bx::smatch match;

  if (bx::regex_match(instring, match, re_span)) {
    g_span         = boost::lexical_cast<int>(match["span"]);
    g_max_ringdiff = boost::lexical_cast<int>(match["ringdiff"]);
  } else {
    LOG_ERROR("Invalid span,ringdiff string: {}", instring);
    exit(1);
  }
}

/**
 * @brief      Parse input geometry
 * @param      instring  'pitch,diameter,thickness' (floats)
 */

void on_geometry(std::string const &instring) {
  bx::sregex re_geom = bx::sregex::compile("(?P<pitch>[0-9]+\\.[0-9]+)\\,(<?P<diam>[0-9]+\\.[0-9]+)\\,(<?P<thick>[0-9]+\\.[0-9]+)");
  bx::smatch match;

  if (bx::regex_match(instring, match, re_geom)) {
    g_pitch = boost::lexical_cast<float>(match["pitch"]);
    g_diam  = boost::lexical_cast<float>(match["diam"]);
    g_thick = boost::lexical_cast<float>(match["thick"]);
  } else {
    LOG_ERROR("Invalid pitch,diam,thick string: {}", instring);
    exit(1);
  }
}

void parse_args(int argc, char **argv) {
  try {
    po::options_description desc("Options");
    desc.add_options()
      ("help,h", "Show options")
      ("time,t"        , po::value<float>(&g_ftime)->required()                         , "Count time")
      // ("rebinner,r"    , po::value<std::string>()->notifier(on_rebinner)->required()    , "Full path of rebinner LUT file")
      ("delays,O"      , po::value<std::string>()->notifier(on_delay)->required()       , "Delayed coincidence file")
      ("geometry,g"    , po::value<std::string>()->notifier(on_geometry)                , "geometry 'pitch,diam,thick'")
      ("csingles,C"    , po::value<std::string>()->notifier(on_csingles)                , "Crystal singles file; old method specifying precomputed crystal singles data")
      ("outsing,S"     , po::value<std::string>()->notifier(on_outsing)                 , "Output crystal singles file")
      ("tau,T"         , po::value<float>(&g_tau)                                       , "time window parameter (6.0 10-9 sec)" )
      ("sino_size,p"   , po::value<std::string>()->notifier(on_sino_size)               , fmt::format("sinogram size 'nelements,nviews' ({},{})", GeometryInfo::NUM_ELEMS, GeometryInfo::NUM_VIEWS).c_str())
      ("coins,h"       , po::value<std::string>()->notifier(on_coincidence)->required() , "Coincidence histogram file")
      ("span,s"        , po::value<std::string>()->notifier(on_span)                    , fmt::format("span,maxrd ({},{})", 9, GeometryInfo::MAX_RINGDIFF).c_str())
    ;
    po::positional_options_description pos_opts;
    pos_opts.add("infile", 1);

    po::variables_map vm;
    po::store(
      po::command_line_parser(argc, argv)
      .options(desc)
      .style(po::command_line_style::unix_style | po::command_line_style::allow_long_disguise)
      .positional(pos_opts)
      .run(), vm
    );
    po::notify(vm);
    if (vm.count("help"))
      std::cout << desc << '\n';
  }
  catch (const po::error &ex) {
    std::cerr << ex.what() << std::endl;
  }
}


int main(int argc, char* argv[]) {
  init_logging();
  parse_args(argc, argv);
  // gen_delays is expecting to see all the globals.  Bring it in this file or pass them explicitly.
	gen_delays(0, 0.0f, NULL, NULL, NULL, g_span, g_max_ringdiff);
	return 0;
}

