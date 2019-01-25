/* Authors: Larry Byars, Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
  10-DEC-2007: Modifications for compatibility with windelays.
  11-Nov-2008: Added LUT for 32bit and 64bit compatibility
  17-Mar-2009: Modified init_sol_lut to load only sinogram map
               if seginfo not provided (segzoffset==NULL)
  07-Apr-2009: Changed filenames from .c to .cpp and removed debugging printf
  30-Apr-2009: Integrate Peter Bloomfield __linux__ support
  02-JUL-2009: Add Transmission(TX) LUT
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "segment_info.h"
#include "geometry_info.h"
#include "lor_sinogram_map.h"

namespace lor_sinogram {

//lor_sinogram_map.h
float *m_c_zpos = NULL;
float *m_c_zpos2 = NULL;
short **m_segplane = NULL;
SOL ***solution_ = NULL;
SOL ***solution_tx_[2];

string const LUT_FILENAME_ENVT_VAR = 'LUT_FILENAME';

// ahc 1/22/19

/**
 * @brief      Gets the LUT filename from envt var LUT_FILENAME.
 * rebinner_lut_file was a required argument passed through many functions but only used in init_lut_sol()
 *  Now get it from required environment variable LUT_FILENAME, defined in LUT_FILENAME_ENVT_VAR
 *
 * @return  boost::filesystem::path which is empty on failure.
 */


bf::path get_lut_filename(void) {
  bf::path lut_filename;
  std::shared_ptr<spdlog::logger> logger = spdlog::get("HRRT");  // Must define logger named HRRT in your main application

  if (const char *env_p = std::getenv(LUT_FILENAME_ENVT_VAR)) {
    bf::path env_path{env_p};
    if (bf::exists(env_path)) {
      logger->debug("Using LUT filename {} from environment variable {}", env_path.string(), LUT_FILENAME_ENVT_VAR);
      lut_filename = env_path;
    } else {
      logger->error("Cannot find LUT filename {} from environment variable {}", env_path.string(), LUT_FILENAME_ENVT_VAR);
    }
  } else {
    logger->error("Cannot file required environment variable {}", LUT_FILENAME_ENVT_VAR);
  }
  return lut_filename;
}

// Return true if this is a LUT file

bool valid_lut_filename(bf::path const &t_path) {
  // Some test goes here.
  return false;
}


// ahc 1/14/19 this was in gen_delays.h, moved to geometry_info.h

// static int nGeometryInfo::HRRT_MPAIRS=20;
// static int mpairs[][2]={{-1,-1},{0,2},{0,3},{0,4},{0,5},{0,6},
//                 {1,3},{1,4},{1,5},{1,6},{1,7},
//                              {2,4},{2,5},{2,6},{2,7},
//                              {3,5},{3,6},{3,7},
//                              {4,6},{4,7},
//                              {5,7}};

//  This procedure converts the line between 2 detector physical
//  coordinates (deta and detb are 3 value arrays, x, y, z in cm)
//  and LOR projection coordinates (4 value array with values of
//  r, phi, z, tan_theta in appropriate units (cm, radians, cm, value).
/*
function phy_to_pro, adet, bdet
  ax=adet[0]
  bx=bdet[0]
  ay=adet[1]
  by=bdet[1]
  az=adet[2]
  bz=bdet[2]
  dz=az-bz
  dx=ax-bx
  dy=ay-by
  d=sqrt(dx*dx+dy*dy)
  r=(ay*bx-ax*by)/d
  if ((dx < 0.0) && (dy < 0.0)) { dx *= -1.0; dy *= -1.};
  phi=atan(dx,dy)/!dtor
  tan_theta=dz/d
  z=az-(ax*dx+ay*dy)*dz/(d*d)
    if (phi < 0.0) then begin
        phi = phi + 180.
        r = -r
        tan_theta=-tan_theta
    endif
  return,[r,phi,z,tan_theta]
end
*/
void init_phy_to_pro( float deta[3], float detb[3], SOL *sol)
{
  // double dx, dy, d;
  // float r,phi, z;
  float pi = (float)M_PI;
  int bin, view;
  // int addr=-1;

  double dy = deta[1] - detb[1];
  double dx = deta[0] - detb[0];
  double d = sqrt( dx * dx + dy * dy);
  float r = (float)((deta[1] * detb[0] - deta[0] * detb[1]) / d); //Xr
  float phi = (float)(atan2( dx, dy));                    //view

  if (phi < 0.0) {
    phi += pi;
    r = -r;
    d = -d;
  }
  if (phi >= M_PI) { //M_PI){//pi) {
    phi = 0.0;
    r = -r; //(float)(-dy);
    d = -d; //(float)(-dy);
  }

  view = (int)(GeometryInfo::nviews_ * phi / M_PI);
  if (view >= 288) view = 0;
  if (view <= -1) {
    view = 0;
    printf("error \t%e\n", phi);
  }
  float z = (float)((deta[0] * dx + deta[1] * dy) / (d * d)); //????
//    view = nviews*phi/M_PI;
  bin = (int)(GeometryInfo::nprojs_ / 2 + (r / GeometryInfo::binsize_ + 0.5));
  if ((bin < 0) || (bin > GeometryInfo::nprojs_ - 1)) {
    sol->nsino = -1;
  } else  sol->nsino = bin + view * GeometryInfo::nprojs_;
  sol->d = (float)(1.0 / d / SegmentInfo::m_d_tan_theta);
  sol->z = (float)( z / GeometryInfo::plane_sep_);
}


void init_sol(int *segzoffset)
{
  // int i,ax,bx,al,bl,plane;
  // int axx,bxx;
  int ahead;
  int bhead;
  float deta_pos[3], detb_pos[3];

  m_c_zpos   = (float *)  calloc(GeometryInfo::NYCRYS, sizeof(float ));
  m_c_zpos2  = (float *)  calloc(GeometryInfo::NYCRYS, sizeof(float ));
  m_segplane = (short **) calloc(63, sizeof(short *));

  for (int i = 0; i < 63; i++) {
    m_segplane[i] = (short *) calloc(GeometryInfo::NYCRYS * 2 - 1, sizeof(short));
  }

  // Initialize and populate solution if NULL
  if (solution_ == NULL) {
    //solution_ = phi(=view angle)
    solution_ = (SOL ***) calloc(21, sizeof(SOL **));
    for (int i = 1; i <= 20; i++) {
      /*    5, 10
          4  9  14
          8  13 17
          7  12 16 19
          11 15 18
      */
      solution_[i] = (SOL**) calloc(GeometryInfo::NUM_CRYSTALS_X_DOIS, sizeof(SOL *));
      for (int ax = 0; ax < GeometryInfo::NUM_CRYSTALS_X_DOIS; ax++) {
        solution_[i][ax] = (SOL *) calloc(GeometryInfo::NUM_CRYSTALS_X_DOIS, sizeof(SOL));
      }
    }
    /*  solution[10]=solution[5];
      solution[9 ]=solution[4];
      solution[14]=solution[4];
      solution[13]=solution[8];
      solution[17]=solution[8];
      solution[12]=solution[7];
      solution[16]=solution[7];
      solution[19]=solution[7];
      solution[15]=solution[11];
      solution[18]=solution[11];
    */
    for (int i = 1; i <= 20; i++) {
      //    if(i==10 || i==9 || i==14 || i==13 || i==17 || i==12 || i==16 || i==19 || i==10 || i==15 || i==18) continue;
      ahead = GeometryInfo::HRRT_MPAIRS[i][0];
      bhead = GeometryInfo::HRRT_MPAIRS[i][1];
      for (int al = 0; al < GeometryInfo::NDOIS; al++) { //layer
        for (int ax = 0; ax < GeometryInfo::NXCRYS; ax++) { //x축
          int axx = ax + GeometryInfo::NXCRYS * al;       //x축+레이어
          det_to_phy( ahead, al, ax, 0, deta_pos);
          for (int bl = 0; bl < GeometryInfo::NDOIS; bl++) {
            for (int bx = 0; bx < GeometryInfo::NXCRYS; bx++) {
              int bxx = bx + GeometryInfo::NXCRYS * bl;
              det_to_phy( bhead, bl, bx, 0, detb_pos);
              init_phy_to_pro(deta_pos, detb_pos, &solution_[i][axx][bxx]);
            }
          }
        }
      }
    }
  }
  printf("\n");
  for (int i = 0; i < GeometryInfo::NYCRYS; i++) {
    det_to_phy( 0, 0, 0, i, deta_pos);
    m_c_zpos[i] = deta_pos[2];
    m_c_zpos2[i] = (float)(deta_pos[2] / GeometryInfo::plane_sep_ + 0.5);
  }

  for (int i = 0; i < 63; i++) {
    for (int plane = 0; plane < GeometryInfo::NYCRYS * 2 - 1; plane++) {
      if (i > SegmentInfo::m_nsegs - 1) {
        m_segplane[i][plane] = -1;
        continue;
      }
      if (plane < SegmentInfo::m_segz0[i]) m_segplane[i][plane] = -1;
      if (plane > SegmentInfo::m_segzmax[i]) m_segplane[i][plane] = -1;
      if (m_segplane[i][plane] != -1) m_segplane[i][plane] = plane + segzoffset[i];
    }
  }
}


int init_lut_sol(std::vector<int> const &segzoffset) {
  std::shared_ptr<spdlog::logger> logger = spdlog::get("HRRT");  // Must define logger named HRRT in your main application
  bf::path lut_filename = get_lut_filename();
  if (!bf::exits(lut_filename)) 
    return 1;
  if (solution_ != NULL) {
    logger->error("solution_ not NULL");
    return 1;
  }

  std::ifstream instream;
  if (hrrt_util::open_istream(lut_filename, instream, std::ifstream::in | std::ifstream::binary)) {
    logger->error("Could not open LUT file {}", lut_filename.string());
    exit(1);
  }

  // Initialize and populate solution if NULL
  if (solution_ == NULL) {
    //solution_ = phi(=view angle)
    solution_ = (SOL ***) calloc(21, sizeof(SOL **));
    for (int i = 1; i <= 20; i++) {
      solution_[i] = (SOL**) calloc(GeometryInfo::NUM_CRYSTALS_X_DOIS, sizeof(SOL *));
      for (int ax = 0; ax < GeometryInfo::NUM_CRYSTALS_X_DOIS; ax++) {
        solution_[i][ax] = (SOL *) calloc(GeometryInfo::NUM_CRYSTALS_X_DOIS, sizeof(SOL));
        // if (fread(solution_[i][ax],sizeof(SOL),GeometryInfo::NUM_CRYSTALS_X_DOIS, fp) != GeometryInfo::NUM_CRYSTALS_X_DOIS)
        instream.read((char *)solution_[i][ax], sizeof(SOL) * GeometryInfo::NUM_CRYSTALS_X_DOIS);
        if (!instream.good()) {
          instream.close();
          logger->error("instream not good for {}", lut_filename.string());
          return 0;  // TODO really return 0 on error?
        }
      }
    }
  }

  if (segzoffset.empty()) {
    instream.close();
    return 1;
  }

  m_c_zpos   = (float *)  calloc(GeometryInfo::NYCRYS, sizeof(float ));
  m_c_zpos2  = (float *)  calloc(GeometryInfo::NYCRYS, sizeof(float ));
  m_segplane = (short **) calloc(63, sizeof(short *));

  for (int i = 0; i < 63; i++) {
    m_segplane[i] = (short *) calloc(GeometryInfo::NYCRYS * 2 - 1, sizeof(short));
  }

  // if (fread(m_c_zpos,sizeof(float),GeometryInfo::NYCRYS, fp) != GeometryInfo::NYCRYS)
  instream.read((char *)m_c_zpos, sizeof(float) * GeometryInfo::NYCRYS);
  if (!instream.good()) {
    logger->error("instream not good for {}", lut_filename.string());
    instream.close();
    return 0;
  }
  // if (fread(m_c_zpos2,sizeof(float),GeometryInfo::NYCRYS, fp) != GeometryInfo::NYCRYS)
  instream.read((char *)m_c_zpos2, sizeof(float) * GeometryInfo::NYCRYS);
  if (!instream.good()) {
    logger->error("instream not good for {}", lut_filename.string());
    instream.close();
    return 0;
  }
  instream.close();

  for (int i = 0; i < 63; i++) {
    for (int plane = 0; plane < GeometryInfo::NYCRYS * 2 - 1; plane++) {
      if (i > SegmentInfo::m_nsegs - 1) {
        m_segplane[i][plane] = -1;
        continue;
      }
      if (plane < SegmentInfo::m_segz0[i]) 
        m_segplane[i][plane] = -1;
      if (plane > SegmentInfo::m_segzmax[i]) 
        m_segplane[i][plane] = -1;
      if (m_segplane[i][plane] != -1) 
        m_segplane[i][plane] = plane + segzoffset[i];
    }
  }
  return 1;
}

int save_lut_sol(bf::path const &lut_filename) {
  std::shared_ptr<spdlog::logger> logger = spdlog::get("HRRT");  // Must define logger named HRRT in your main application

  if (solution_ != NULL) {
    std::ofstream outstream;
    if (hrrt_util::open_ostream(lut_filename, outstream, std::ifstream::out | std::ifstream::binary))
      return 0;
    for (int i = 1; i <= 20; i++)    {
      for (int ax = 0; ax < GeometryInfo::NUM_CRYSTALS_X_DOIS; ax++) {
        // if (fwrite(solution_[i][ax],sizeof(SOL),GeometryInfo::NUM_CRYSTALS_X_DOIS, fp) != GeometryInfo::NUM_CRYSTALS_X_DOIS)
        outstream.write(solution_[i][ax], sizeof(SOL) *GeometryInfo::NUM_CRYSTALS_X_DOIS);
        if (!outstream.good()) {
          logger->error("outstream not good for {}", lut_filename.string());
          outstream.close()
          return 0;
        }
      }
    }
    // if (fwrite(m_c_zpos,sizeof(float),GeometryInfo::NYCRYS, fp) != GeometryInfo::NYCRYS)
    outstream.write(m_c_zpos, sizeof(float) * GeometryInfo::NYCRYS);
    if (!outstream.good()) {
          logger->error("outstream not good for {}", lut_filename.string());
      outstream.close()
      return 0;
    }
    // if (fwrite(m_c_zpos2,sizeof(float),GeometryInfo::NYCRYS, fp) != GeometryInfo::NYCRYS)
    outstream.write(m_c_zpos, sizeof(float) * GeometryInfo::NYCRYS);
    if (!outstream.good()) {
          logger->error("outstream not good for {}", lut_filename.string());
      outstream.close()
      return 0;
    }
    outstream.close();
  }
  return 1;
}

void init_sol_tx(int tx_span) {
  float deta_pos[3], detb_pos[3];

  m_d_tan_theta = (float)(tx_span * GeometryInfo::plane_sep_ / GeometryInfo::crystal_radius_);
  // Initialize and populate solution if NULL
  // TX m_sol
  //solution_ = phi(=view angle)
  solution_tx_[0] = (SOL ***) calloc(GeometryInfo::NMPAIRS + 1, sizeof(SOL **));
  solution_tx_[1] = (SOL ***) calloc(GeometryInfo::NMPAIRS + 1, sizeof(SOL **));
  for (int i = 1; i <= GeometryInfo::NMPAIRS; i++) {
    /*    5, 10
        4  9  14
        8  13 17
        7  12 16 19
        11 15 18
    */
    solution_tx_[0][i] = (SOL**) calloc(GeometryInfo::NUM_CRYSTALS_X_DOIS, sizeof(SOL *));
    solution_tx_[1][i] = (SOL**) calloc(GeometryInfo::NUM_CRYSTALS_X_DOIS, sizeof(SOL *));
    for ( int ax = 0; ax < GeometryInfo::NUM_CRYSTALS_X_DOIS; ax++) {
      solution_tx_[0][i][ax] = (SOL *) calloc(GeometryInfo::NXCRYS, sizeof(SOL));
      solution_tx_[1][i][ax] = (SOL *) calloc(GeometryInfo::NXCRYS, sizeof(SOL));
    }
  }
  // Table 0,  B is  TX source
  for (int i = 1; i <= GeometryInfo::NMPAIRS; i++) {
    int ahead = GeometryInfo::HRRT_MPAIRS[i][0];
    int bhead = GeometryInfo::HRRT_MPAIRS[i][1];
    for (int al = 0; al < GeometryInfo::NDOIS; al++) { //layer
      for (int ax = 0; ax < GeometryInfo::NXCRYS; ax++) { //x축
        int axx = ax + GeometryInfo::NXCRYS * al;       //x축+레이어
        det_to_phy( ahead, al, ax, 0, deta_pos);
        for (int bx = 0; bx < GeometryInfo::NXCRYS; bx++) {
          det_to_phy( bhead, 7, bx, 0, detb_pos);
          init_phy_to_pro(deta_pos, detb_pos, &solution_tx_[0][i][axx][bx]);
        }
      }
    }
  }

  // Table 1,  A is  TX source
  for (int i = 1; i <= GeometryInfo::NMPAIRS; i++) {
    int ahead = GeometryInfo::HRRT_MPAIRS[i][0];
    int bhead = GeometryInfo::HRRT_MPAIRS[i][1];
    for (int ax = 0; ax < GeometryInfo::NXCRYS; ax++) { //x축
      det_to_phy( ahead, 7, ax, 0, deta_pos);
      for (int bl = 0; bl < GeometryInfo::NDOIS; bl++) { //layer
        for (int bx = 0; bx < GeometryInfo::NXCRYS; bx++) {
          int bxx = bx + GeometryInfo::NXCRYS * bl;
          det_to_phy( bhead, bl, bx, 0, detb_pos);
          init_phy_to_pro(deta_pos, detb_pos, &solution_tx_[1][i][bxx][ax]);
        }
      }
    }
  }
}

int save_lut_sol_tx(bf::path const &lut_filename) {
  std::shared_ptr<spdlog::logger> logger = spdlog::get("HRRT");  // Must define logger named HRRT in your main application

  std::ofstream outstream;
  if (hrrt_util::open_ostream(lut_filename, outstream, std::ifstream::out | std::ifstream::binary))
    return 0;
  for (int i = 1; i <= GeometryInfo::NMPAIRS; i++) {
    for (int ax = 0; ax < GeometryInfo::NUM_CRYSTALS_X_DOIS; ax++) {
      // if (fwrite(solution_tx_[0][i][ax], sizeof(SOL), GeometryInfo::NXCRYS, fp) != GeometryInfo::NXCRYS ||
      //     fwrite(solution_tx_[1][i][ax], sizeof(SOL), GeometryInfo::NXCRYS, fp) != GeometryInfo::NXCRYS)
        outstream.write(solution_tx_[0][i][ax], sizeof(SOL) * GeometryInfo::NXCRYS);
        outstream.write(solution_tx_[1][i][ax], sizeof(SOL) * GeometryInfo::NXCRYS);
        if (!outstream.good()) {
          logger->error("outstream not good for {}", lut_filename.string());
        fclose(fp);
        return 0;
      }
    }
  }
  outstream.close();
  return 1;
}

int init_lut_sol_tx(void) {
  std::shared_ptr<spdlog::logger> logger = spdlog::get("HRRT");  // Must define logger named HRRT in your main application
  bf::path lut_filename = get_lut_filename();
  if (!bf::exits(lut_filename)) 
    return 1;
  if (solution_ != NULL) 
    return 1;

  std::ifstream instream;
  if (hrrt_util::open_istream(lut_filename, instream, std::ifstream::in | std::ifstream::binary))
    exit(1);

  // Initialize and populate solution if NULL
  if (solution_tx_[0] == NULL) {
    //solution_ = phi(=view angle)
    solution_tx_[0] = (SOL ***) calloc(21, sizeof(SOL **));
    solution_tx_[1] = (SOL ***) calloc(21, sizeof(SOL **));
    for (int i = 1; i <= 20; i++) {
      solution_tx_[0][i] = (SOL**) calloc(GeometryInfo::NUM_CRYSTALS_X_DOIS, sizeof(SOL *));
      solution_tx_[1][i] = (SOL**) calloc(GeometryInfo::NUM_CRYSTALS_X_DOIS, sizeof(SOL *));
      for (int ax = 0; ax < GeometryInfo::NUM_CRYSTALS_X_DOIS; ax++) {
        solution_tx_[0][i][ax] = (SOL *) calloc(GeometryInfo::NXCRYS, sizeof(SOL));
        solution_tx_[1][i][ax] = (SOL *) calloc(GeometryInfo::NXCRYS, sizeof(SOL));
        // if (fread(solution_tx_[0][i][ax], sizeof(SOL), GeometryInfo::NXCRYS, fp) != GeometryInfo::NXCRYS ||
        //     fread(solution_tx_[1][i][ax], sizeof(SOL), GeometryInfo::NXCRYS, fp) != GeometryInfo::NXCRYS)
          instream.read(solution_tx_[0][i][ax], sizeof(SOL) * GeometryInfo::NXCRYS);
          instream.read(solution_tx_[1][i][ax], sizeof(SOL) * GeometryInfo::NXCRYS);
          if (!instream.good()) {
          logger->error("instream not good for {}", lut_filename.string());
          instream.close();
          return 0;
        }
      }
    }
  }

  instream.close();
  return 1;
}

}  // namespace lor_sinogram
