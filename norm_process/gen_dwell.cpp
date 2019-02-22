#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gen_delays_lib/gen_delays_lib.hpp>
#include <gen_delays_lib/geometry_info.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/lor_sinogram_map.h>
#include <boost/filesystem.hpp>

#include <boost/filesystem.hpp>
namespace bf = boost::filesystem;

// bf::path g_logfile;

void usage(const char *pgm)
{
  printf("usage: %s -o out_file [-s span,maxrd] [-l layer]\n", pgm);
	printf("    -s span,rd  : set span and maxrd values [9,67]\n");
	printf("    -l layer    : 0=back layer,1=front layer, 2=all layers (default=2)\n");
  exit(1);
}

void main(int argc, char ** argv) {
  // int ax, ay, bx, by;
  int alayer, blayer, rd;
	// int axx,bxx;
	int headxcrys = GeometryInfo::NHEADS * GeometryInfo::NXCRYS;
	float cay;
  float *dptr;
	int nprojs=256, nviews=288;
	int bs,be;
	lor_sinogram::SOL *sol;
	int segnum,plane;
	float dz2[104],seg;
  float *dwell_data=NULL, **dwell_data2 = NULL;
  int maxrd=GeometryInfo::MAX_RINGDIFF, span=9, nplanes=0;
  int i, npixels=0, nvoxels=0;
  // const char *rebinner_lut_file=NULL;
  const char *out_file =NULL;
  double m_d_tan_theta=0;
  int layer=GeometryInfo::NDOIS; // all layers

  while ((i = getopt( argc, argv, "o:l:s:")) != -1)
  {
    switch(i)
    {
    case 'o':  
      out_file = optarg; break;
    case 'l':
      sscanf(optarg, "%d", &layer);
      if (layer<0 || layer>GeometryInfo::NDOIS) 
      {
        LOG_ERROR("Invalid layer %d\n", layer);
        usage(argv[0]);
      }
      break;
      case 's': 
        sscanf( optarg, "%d,%d", &span, &maxrd); 
        break;
      default: usage(argv[0]);
    }
  }
  if (out_file  == NULL) usage(argv[0]);
  // boost::filesystem::path rebinner_lut_file;
  // if ((rebinner_lut_file=hrrt_rebinner_lut_path()) == NULL)
  // {
  //   LOG_INFO("Rebinner LUT file not found\n");
  //   exit(1);
  // }
  SegmentInfo::init_segment_info(&SegmentInfo::m_nsegs, &nplanes, &m_d_tan_theta, maxrd, span, GeometryInfo::NYCRYS, GeometryInfo::crystal_radius_, GeometryInfo::plane_sep_);
  npixels=nprojs*nviews;
  nvoxels=npixels*nplanes;

  // lor_sinogram::init_lut_sol(rebinner_lut_file, SegmentInfo::m_segzoffset);
  lor_sinogram::init_lut_sol(SegmentInfo::m_segzoffset);
  if ((dwell_data = (float *) calloc( nvoxels, sizeof(float) ))  ==  NULL)	{
		printf("memory allocation failed\n");
		exit(1);
	}
	if ((dwell_data2 = (float **) calloc( npixels, sizeof(float*) ))  ==  NULL)	{
		printf("memory allocation failed\n");
		exit(1);
	}
  for (i = 0; i<npixels; i++)
    dwell_data2[i] =  dwell_data + i*nplanes;

  for (int mp=1; mp <= GeometryInfo::NMPAIRS; mp++)  {
    printf("mp %d\n", mp);
	  for (alayer=0; alayer<GeometryInfo::NDOIS; alayer++)    {
      if (layer<GeometryInfo::NDOIS && alayer != layer) 
        continue; // not requested layer
		  for (ay=0; ay<GeometryInfo::NYCRYS; ay++){
			  cay=lor_sinogram::m_c_zpos2[ay];
			  bs=1000;
        be=-1000;
			  for(by=0;by<GeometryInfo::NYCRYS;by++){
				  dz2[by]=lor_sinogram::m_c_zpos[by]-lor_sinogram::m_c_zpos[ay]; // z diff. between det A and det B
          rd = ay-by; if (rd<0) rd=by-ay; 
				  if(rd < maxrd+6){  // dsaint31 : why 6??
					  if(bs>by) 
              bs=by; //start ring # of detB
					  if(be<by) 
              be=by; //end   ring # of detB
				  }
			  }
			  for (int ax=0; ax<GeometryInfo::NXCRYS; ax++){
				  int axx=ax+GeometryInfo::NXCRYS*alayer;
				  for (blayer=0; blayer<GeometryInfo::NDOIS; blayer++){
            if (layer<GeometryInfo::NDOIS && blayer != layer) 
            continue; // not requested layer
					  int bxx=GeometryInfo::NXCRYS*blayer;
					  for (int bx=0; bx<GeometryInfo::NXCRYS; bx++,bxx++){
						  if(lor_sinogram::solution_[mp][axx][bxx].nsino == -1) 
                continue;
						  sol=&lor_sinogram::solution_[mp][axx][bxx];
  						
						  dptr = dwell_data2[sol->nsino];//result bin location

						  for (int by=bs; by <= be; by++){
                plane        = (cay+sol->z*dz2[by]);
                segnum = seg = (0.5+dz2[by] * sol->d);
							  if(seg<0) 
                  segnum=1-(segnum<<1);
							  else      
                  segnum=segnum<<1;
							  if(lor_sinogram::m_segplane[segnum][plane]!=-1)
                  dptr[lor_sinogram::m_segplane[segnum][plane]] +=1;
						  }
					  }
				  }
			  }
      }
    }
  }
  //-------------------------------------------------------
// file write
	float *dtmp  = (float *) calloc( npixels, sizeof(float));		
  FILE *fptr=fopen(out_file, "wb");
	if (!fptr) {
		printf("Can't create output file '%s'\n", out_file);
    exit(1);
	}
  printf("writing file '%s'\n", out_file);
	for(i = 0;i<nplanes;i++) {
    for(int n=0;n<npixels;n++) {
      dtmp[n]=dwell_data2[n][i];
      if (dtmp[n]>0) 
        dtmp[n] = 5.0f/dtmp[n];
    }
    fwrite( dtmp, sizeof(float), npixels, fptr);
  }
  fclose(fptr);
}