// hrrt_rebinner_lut.cpp : Defines the entry point for the console application.
/* Authors:  Merence Sibomana
  Creation 11/2008
  Modification history:
  22-MAY-2009: Added options -k to support Cologne gantry and
               -L for low tresolution mode

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>
#include <unistd.h>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace po = boost::program_options;
namespace bf = boost::filesystem;

std::string g_logfile;
extern std::shared_ptr<spdlog::logger> g_logger;

bf::path g_out_fname;
int g_tx_flag{0};
int g_span{9};
int g_maxrd{GeometryInfo::MAX_RINGDIFF};

void on_out(const std::string &outstr) {
  g_out_fname = bf::path(outstr);
  g_logger->debug("output file {}", g_out_fname.string());
}

void on_transmission(const std::string &outstr) {
  g_out_fname = bf::path(outstr);
  g_tx_flag = 1;
  g_span = 21;
  g_maxrd = 10;
}

void on_low(int intval) {
  std::vector<LR_Type> good_vals = {LR_Type::LR_20, LR_Type::LR_24};
  LR_type t = GeometryInfo::to_lrtype(intval, good_vals);
  GeometryInfo::LR_type = t;
}

void main(int argc, char* argv[]) {
  try {
    po::options_description desc("Options");
    desc.add_options()
    ("help,h", "Show options")
    ("out,o"         , po::value<std::string>()->notifier(&on_out)         , "Output LUT file" )
    ("transmission,t", po::value<std::string>()->notifier(&on_transmission), "Transmission LUT output file" )
    ("low,L"         , po::value<int>()->notifier(&on_low)                 , "set Low Resolution mode (1: binsize=2mm, nbins=160, nviews=144, 2:binsize=2.4375" )
    ;

    po::variables_map vm;
    po::store(
      po::command_line_parser(argc, argv)
      .options(desc)
      .style(
        po::command_line_style::unix_style
        | po::command_line_style::allow_long_disguise)
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

  head_crystal_depth_.assign(1.0f);
  GeometryInfo::init_geometry_hrrt();
  int nplanes = 0;
  SegmentInfo::init_segment_info(&SegmentInfo::m_nsegs, &nplanes, &SegmentInfo::m_d_tan_theta, g_maxrd, g_span, NYCRYS, m_crystal_radius, m_plane_sep);

  if (!g_tx_flag) {
    init_sol(SegmentInfo::m_segzoffset);
    save_lut_sol(g_out_fname);
  } else {
    init_sol_tx(g_span);
    save_lut_sol_tx(g_out_fname);
  }
  exit(0);
}