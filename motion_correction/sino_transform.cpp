/*
 Authors : Merence Sibomana - HRRT Users Community
           Sune H. Keller - Rigshospitalet/HRRT Users Community
 Creation date :01-Dec-2009
 Modification history :
*/

#include "matpkg.h"
#include "nr_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
// ahc 11/15/17 This was including hrrt_open_2011/include/AIR/AIR.h
// But this was an old version of AIR.h from 2002
// Current AIR 5.3.0 is 2011, so set -I in CMake to AIR src dir.
// #include <AIR/AIR.h>
#include <AIR.h>
#include <ecatx/matrix.h>
#include <unistd.h>

#define MAX_SEG 45

static char *in_file=NULL, *out_file=NULL;
static int float_flag=0;
static float bin_size=1.21875f, plane_sep=1.21875f, radius=234.5f;
static int xdim=256, ydim=256,zdim=207;
static float pixel_size=1.21875f, z_pixel_size=1.21875f;
static int nsegs=15, nviews=288, nprojs=256, rd=GeometryInfo::MAX_RINGDIFF, span=9;

static int seg_offset[MAX_SEG], seg_planes[MAX_SEG];
template<class T>
static void init_seg_info()
{
  struct stat st;
  int nrings = 104;
  int nplns0 = 2*nrings-1;
  nsegs = (2*rd+1)/3;
	int nplanes_3 = nsegs*nplns0 - (nsegs-1)*(3*(nsegs-1)/2+1);
	nsegs = (2*rd+1)/9;
	int nplanes_9 = nsegs*nplns0 - (nsegs-1)*(9*(nsegs-1)/2+1);
	if (stat(in_file,&st) == -1)
	{
		perror(in_file);
		exit(1);
	}
	size_t data_size = st.st_size;
	if (data_size== (size_t)(nprojs*nviews*nplanes_3*sizeof(T))) span = 3;
	else if (data_size == (size_t)(nprojs*nviews*nplanes_9*sizeof(T))) span = 9;
  else crash("unkown file size: not span 9 or span 3\n");
  nsegs = (2*rd+1)/span;
  seg_offset[0] = 0; seg_planes[0] = nplns0;
  for (int iseg=1; iseg<nsegs; iseg += 2) {
    int seg = (iseg+1)/2;
    seg_offset[iseg] = seg_offset[iseg-1] + seg_planes[iseg-1];
    seg_planes[iseg+1] = seg_planes[iseg] = 2*nrings - span*(2*seg -1) -2;  
    seg_offset[iseg+1] = seg_offset[iseg] + seg_planes[iseg];
  } 
}

template <class T>
static int read_segment(FILE *fp, T ***sino, int nplanes)
{
  for (int plane=0; plane<nplanes; plane++)
    for (int view=0; view<nviews; view++)
      if (fread(sino[view][plane], sizeof(T), nprojs, fp) != nprojs) return 0;
  return 1;
}

static void usage(const char *pgm)
{
  fprintf(stderr,
	  "usage: %s -i input -o output [-t transformer | -a air_file] \n", pgm);
  fprintf(stderr,
	  "Transformer format: rz(yaw),rx(pitch),ry(roll),tx,ty,tz\n");
  fprintf(stderr,"-v set verbose mode on ( default is off)\n");
	fprintf(stderr,"ex: sino_transform -i point.s -o point_tx_20.s -t 0,0,0,20,0,0)\n");
  exit(1);
}

int  main(int argc, char **argv)
{
  int c=0, transformer=0, verbose=0, convention=0;
  float tx=0, ty=0, tz=0, rx=0, ry=0, rz=0;
  float c1[4], c2[4];  // original and transformed FOV centers
  float x1[4], x2[4];  // original and transformed given point
  float u2[3];         // transformed direction vector
  char *air_file = NULL;
  struct AIR_Air16 air_16;
  float phi=0.0f,phi2=0.0f, theta=0.0f, theta2=0.0f;
  int iseg,seg,view,proj,plane;
  float df=(float)(M_PI/nviews), dt=plane_sep*span/radius;
  short ***sino=NULL, ****sino2=NULL;
  float ***fsino=NULL, ****fsino2=NULL;
  FILE *fpi=NULL, *fpo=NULL;
 
  while ((c = getopt (argc, argv, "i:o:t:x:a:fNv")) != EOF) {
    switch (c) {
      case 'i':
        in_file = optarg;
        break;
      case 'o':
        out_file = optarg;
        break;
      case 'a':
        air_file = optarg;
        break;
      case 'f':
        float_flag = 1;
        break;
      case 't' :
        if (sscanf(optarg,"%g,%g,%g,%g,%g,%g",&rz,&rx,&ry,&tx,&ty,&tz) > 0)
          transformer = 1;
        break;
      case 'N' :
        convention = 1;
      break;
      case 'v' :
        verbose = 1;
      break;
    }
  }

  if (in_file==NULL || out_file==NULL) usage(argv[0]);
  if (!transformer && !air_file) usage(argv[0]);
  Matrix t = mat_alloc(4,4);
                          // apply Transformer w.r.t AIR convention
  c1[0] = 0.5f*xdim*pixel_size;
  c1[1] = 0.5f*ydim*pixel_size;
  c1[2] = 0.5f*zdim*z_pixel_size;
  c1[3]=1.0f;

  if (air_file)
  {
    float c_size;   // isotropic voxel size used in AIR transformer
    AIR_read_air16(air_file, &air_16);
    for (int j=0;j<t->nrows;j++)
      for (int i=0;i<t->ncols;i++)
        t->data[i+j*t->ncols] = (float)air_16.e[i][j];
    pixel_size = (float)air_16.r.x_size;
    z_pixel_size = (float)air_16.r.z_size;
    c_size = (pixel_size>z_pixel_size)? z_pixel_size : pixel_size;
	  scale(t, pixel_size/c_size, pixel_size/c_size, z_pixel_size/c_size);
  }
  else
  {
	  if (convention==1) {
		  rx = -rx;       // apply pitch (-)
		  tx = -tx;	      // reverse x
	  }
	  scale(t,pixel_size,pixel_size, z_pixel_size);
	  translate(t,-c1[0],-c1[1],-c1[2]);
	  rotate(t,0,0,-rz);        // apply yaw (-) : opposite sign wrt AIR because z not mirrored
	  rotate(t,rx,0,0); 
	  rotate(t,0,ry,0);       // apply roll (+) : opposite sign wrt AIR because y not mirrored
	  translate(t,c1[0],c1[1],c1[2]);
	  translate(t,tx,ty,tz);
	  scale(t, 1.0f/pixel_size, 1.0f/pixel_size, 1.0f/z_pixel_size);
  }
  mat_print(t);
  mat_apply(t, c1, c2);          // x0=translation (transformed origin)
  if (float_flag)
  {
    init_seg_info<float>();
    fsino = matrix3d(0,nviews-1, 0, seg_planes[0]-1, 0,nprojs-1);
    if (fsino==NULL) crash("memory allocation failure for 2D sinogram\n");
    if ((fsino2 = (float****)calloc(nsegs, sizeof(float***))) == NULL)
       crash("memory allocation failure for 3D sinogram\n");
    for (iseg=0; iseg<nsegs; iseg++) {
      fsino2[iseg] = matrix3d(0,nviews-1, 0,seg_planes[iseg]-1, 0,nprojs-1);
      if (fsino2[iseg]==NULL) 
        crash("memory allocation failure for 3D sinogram, segment %d\n", iseg);
    }
  }
  else
  {
    init_seg_info<short>();
    sino = matrix3dshort(0,nviews-1, 0, seg_planes[0]-1, 0,nprojs-1);
    if (sino==NULL) crash("memory allocation failure for 2D sinogram\n");
    if ((sino2 = (short****)calloc(nsegs, sizeof(short***))) == NULL)
       crash("mmory allocation failure for 3D sinogram\n");
    for (iseg=0; iseg<nsegs; iseg++) {
      sino2[iseg] = matrix3dshort(0,nviews-1, 0,seg_planes[iseg]-1, 0,nprojs-1);
      if (sino2[iseg]==NULL) 
        crash("memory allocation failure for 3D sinogram, segment %d\n", iseg);
    }
  }
  if ((fpi = fopen(in_file, "rb")) == NULL)
    crash("error opening input file %s\n", in_file);
  if ((fpo = fopen(out_file, "wb")) == NULL)
    crash("error opening output file %s\n", out_file);
//  for (iseg=0; iseg<nsegs; iseg++)
  for (iseg=0; iseg<1; iseg++)
  {
    // read one segment into sino[view][plane][proj];
    if (float_flag)
      read_segment<float>(fpi, fsino, seg_planes[iseg]);
    else
      read_segment<short>(fpi, sino, seg_planes[iseg]);
	
#ifdef UNIT_TEST1 // write loaded segment
    for (plane=0; plane<seg_planes[iseg]; plane++)
    {
      for (view=0; view<nviews; view++)
      {
        if (float_flag) 
          fwrite(fsino[view][plane], sizeof(float), nprojs, fpo);
        else
          fwrite(sino[view][plane], sizeof(short), nprojs, fpo);
      }
    }
#else // UNIT_TEST1
    seg = (iseg+1)/2;
    if (iseg%2 != 0) seg = -seg;
    theta = seg*dt;
    float cos_theta = (float)cos(theta);
    for (view=0; view<nviews; view++)
    {
      phi = view*df;
      float cos_phi = (float)cos(phi);
      float sin_phi = (float)sin(phi);
      // compute direction vector x1
      x1[0] = (float)(c1[0]+cos_theta*cos_phi);
      x1[1] = (float)(c1[1]+cos_theta*sin_phi);
      x1[2] = (float)(c1[2]+sin(theta));
      x1[3] = 1.0f;
      // compute transformed direction vector x2
      mat_apply(t, x1, x2);

      // convert transformed direction vector x2 back to polar coordinates
      u2[0] = x2[0]-c2[0];
      u2[1] = x2[1]-c2[1];
      u2[2] = x2[2]-c2[2];
      float d2 = (float)sqrt( u2[0]*u2[0] + u2[1]*u2[1]);
      phi2 = atan2(u2[1],u2[0]);
      theta2 = atan2(u2[2], d2);
      
      // convert back to sinogram space
      seg = round(theta2/dt);          // tranformed segment
      int view2 = round(phi2/df);           // tranformed view
      int view2r = view2;
      if (view2<0) view2r += nviews;
      else if (view2==nviews) view2r=0;
      if (view2r != view2) seg *= -1;
      int iseg2 = seg;
      if (seg < 0) iseg2 = -2*seg-1;
      else if (seg>0)iseg2 = 2*seg;
      // Reposition all projections within segment and view
      for (plane=0; plane<seg_planes[iseg]; plane++)
      {
        int plane_offset = (seg_planes[iseg]-seg_planes[0])/2;
        x1[0] = c1[0]; x1[1] = c1[1];
        x1[2] = (plane+plane_offset)*plane_sep;
        mat_apply(t, x1, x2); // center point
        for (proj=0; proj<nprojs/2; proj++)
        {
          if (proj>0)
          {
            // compute radial position; orthogonal direction: (-sin_phi, cos_phi)
            x1[0] = c1[0] + proj*bin_size*(-sin_phi);
            x1[1] = c1[1] + proj*bin_size*cos_phi;
            mat_apply(t, x1, x2);
          }
          // change origin to FOV center
          x2[0] -= c1[0];  x2[1] -= c1[1]; 
          int plane2 = round(x2[2]/plane_sep) + (seg_planes[iseg2]-seg_planes[0])/2;
          if (plane2<plane_offset || plane2>=plane_offset+seg_planes[iseg2]) continue;
          // LOR (x2,x2+u2)
          // code from sort3d_hrrt.cpp
          // d = sqrt( dx*dx + dy*dy);
          // r = (float)((deta[1]*detb[0]-deta[0]*detb[1])/d);
          float r = (x2[1]*(x2[0]+u2[0]) - x2[0]*(x2[1]+u2[1]))/d2;
          int proj2 = (int)(nprojs/2+r/bin_size+0.5);
          if (proj2<0 || proj2>=nprojs) continue;
          if (view2 != view2r) {
            proj2 = nprojs-proj2-1; // reverse r
          }
          if (float_flag)
            fsino2[iseg2][view2r][plane2][proj2] = fsino[view][plane][nprojs/2+proj];
          else
            sino2[iseg2][view2r][plane2][proj2]=sino[view][plane][nprojs/2+proj];
          if (proj==0) continue;
          // Opposite projection
          x1[0] = c1[0] - proj*bin_size*(-sin_phi);
          x1[1] = c1[1] - proj*bin_size*cos_phi;
          mat_apply(t, x1, x2);
          // change origin to FOV center
          x2[0] -= c1[0];  x2[1] -= c1[1]; 
          plane2 = round(x2[2]/plane_sep) + (seg_planes[iseg2]-seg_planes[0])/2;
          if (plane2<plane_offset || plane2>=plane_offset+seg_planes[iseg2]) continue;
          r = (x2[1]*(x2[0]+u2[0]) - x2[0]*(x2[1]+u2[1]))/d2;
          proj2 = (int)(nprojs/2+r/bin_size+0.5);
          if (proj2<0 || proj2>=nprojs) continue;
          if (view2 != view2r) {
            proj2 = nprojs-proj2-1; // reverse r
          }
          if (float_flag)
            fsino2[iseg2][view2r][plane2][proj2] = fsino[view][plane][nprojs/2-proj];
          else
            sino2[iseg2][view2r][plane2][proj2]=sino[view][plane][nprojs/2-proj];
        }
      }
    }
#endif //UNIT_TEST1
  }
  fclose(fpi);
#ifndef UNIT_TEST1
//  for (iseg=0; iseg<nsegs; iseg++)
  for (iseg=0; iseg<1; iseg++)
  {
    for (plane=0; plane<seg_planes[iseg]; plane++)
    {
      for (view=0; view<nviews; view++)
      {
        if (float_flag) 
          fwrite(fsino2[iseg][view][plane], sizeof(float), nprojs, fpo);
        else
          fwrite(sino2[iseg][view][plane], sizeof(short), nprojs, fpo);
      }
    }
  }
#endif

  fclose(fpo);
  if (verbose) printf("done\n");
  return 0;
}