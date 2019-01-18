#include <stdio.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <map>
#include <gen_delays_lib/gen_delays.h>
#include <gen_delays_lib/geometry_info.h>
#include <gen_delays_lib/lor_sinogram_map.h>

using namespace std;
#define NPROJS 256
#define NVIEWS 288
#define NSINOS 207
static float eps=0.0001f;
static float sino[NPROJS*NVIEWS];
static float ca[NPROJS*NVIEWS];
static float cb[NPROJS*NVIEWS];
typedef struct _geomentry
{
  double sum;
  int count;
} GeomEntry;
static short ordered_mp[] = {0,3,8,13,17,  /* opposite heads */
                               2,7,12,16,   /*opposite head -1 */
                               4,9,14,19,  /*opposite head +1 */
                               1,6,11,15,  /*opposite head -2 */
                               5,10,18,20  /*opposite head +2 */
                             };

static  map<int, GeomEntry> gmap;
static  GeomEntry ge;
static  map<int, GeomEntry>::iterator mi;

static void pass1(int i)
{
  if (sino[i] <eps) return; // outlier
  if (fabs(ca[i]-cb[i])< eps)
  {
    int key = (int) (0.5f+(ca[i]+cb[i])/2);
    if ((mi=gmap.find(key)) == gmap.end())
    {
      ge.sum = sqrt(sino[i]);
      ge.count = 1;
      gmap.insert(make_pair(key,ge));
    }
    else
    {
      mi->second.sum += sqrt(sino[i]);
      mi->second.count++;
    }
    if (key==0) 
    {
      int v = i/256, p=i%256;
   //   printf("check here %d,%d\n", v,p);
    }
  }
}

static void pass2(int i)
{
  int k1, k2; //k1<k2;
  float g1=0.0f;

  if (sino[i] <eps) return; // outlier
  k1=ca[i]<cb[i]? (int) (.5f+ca[i]):(int) (.5f+cb[i]);
  k2=ca[i]<cb[i]? (int) (.5f+cb[i]):(int) (.5f+ca[i]);
  if ((mi=gmap.find(k1)) != gmap.end())
  {
    float g1 = (float)(mi->second.sum/mi->second.count);
    if ((mi=gmap.find(k2)) == gmap.end())
    {
      ge.sum = sino[i]/g1;
      ge.count = 1;
      gmap.insert(make_pair(k2,ge));
    }
    else
    {
      mi->second.sum += sino[i]/g1;
      mi->second.count++;
    }
  }
}
static void pass3(int i)
{
  int k1, k2; //k1<k2;
  float g1=0.0f;

  if (sino[i] <eps) return; // outlier
  k1=ca[i]<cb[i]? (int) (.5f+ca[i]):(int) (.5f+cb[i]);
  k2=ca[i]<cb[i]? (int) (.5f+cb[i]):(int) (.5f+ca[i]);
  if (sino[i]<.4) return; // exclude outliers
  if ((mi=gmap.find(k1)) != gmap.end())
  {
    float g1 = (float)(mi->second.sum/mi->second.count);
    if ((mi=gmap.find(k2)) == gmap.end())
    {
      ge.sum = sino[i]/g1;
      ge.count = 1;
      gmap.insert(make_pair(k2,ge));
    }
    else
    {
      mi->second.sum += sino[i]/g1;
      mi->second.count++;
    }
  }
}

void main(int argc, char ** argv)
{
  int i=0;

  // TODO use boost::program_options to read in a file_path for rebinner_lut_file
  const char *rebinner_lut_file=NULL;
  int *m_segzoffset=NULL;
  int mp, al, bl, ax, axx, bx, bxx;
  float d_theta=1.0, plane_sep=1.21875f, crystal_radius=234.5f;

  if ((rebinner_lut_file=hrrt_rebinner_lut_path())==NULL)
  {
    fprintf(stdout,"Rebinner LUT file not found\n");
    exit(1);
  }
  init_lut_sol(rebinner_lut_file, m_segzoffset);

  al=bl=1; // front layer
	if (argc != 4) 
	{
    printf("usage: %s gragrb_sino a_angle b_angle\n", argv[0]);
    exit(1);
  }
  FILE *fp1 = fopen(argv[1],"rb");
  FILE *fp2 = fopen(argv[2],"rb");
  FILE *fp3 = fopen(argv[3],"rb");
  if (fp1 == NULL || fp2 == NULL || fp3 == NULL)
  {
    printf("error opening file\n");
    exit(1);
  }
  fread(sino,  sizeof(float), NPROJS*NVIEWS, fp1);
  fread(ca, sizeof(float), NPROJS*NVIEWS, fp2);
  fread(cb, sizeof(float), NPROJS*NVIEWS, fp3);
  fclose(fp1);
  fclose(fp2);
  fclose(fp3);

  // First pass with opposite heads and +/-1
  for (mp=1; mp<=12; mp++)
  {
    for(ax=0;ax<NXCRYS;ax++)
    { 
      axx=ax+NXCRYS*al; 
      for(bx=0;bx<NXCRYS;bx++)
      {
        bxx=bx+NXCRYS*bl;
        if ((i=m_solution[ordered_mp[mp]][axx][bxx].nsino) >=0 ) pass1(i);
      }
    }
  }
  for (i=20; i<=22L; i++) gmap.erase(i);

  // second pass
  for (mp=5; mp<=20; mp++)
    for(ax=0;ax<NXCRYS;ax++)
    { 
      axx=ax+NXCRYS*al; 
      for(bx=0;bx<NXCRYS;bx++)
      {
        bxx=bx+NXCRYS*bl;
        if ((i=m_solution[ordered_mp[mp]][axx][bxx].nsino) >=0 ) pass2(i);
      }
    }
  for (i=48; i<=58; i++) gmap.erase(i);

  // third pass with opposite heads
  for (mp=13; mp<=20; mp++)
    for(ax=0;ax<NXCRYS;ax++)
    { 
      axx=ax+NXCRYS*al; 
      for(bx=0;bx<NXCRYS;bx++)
      {
        bxx=bx+NXCRYS*bl;
        if ((i=m_solution[ordered_mp[mp]][axx][bxx].nsino) >=0 ) pass3(i);
      }
    }

   for (mi=gmap.begin(); mi!= gmap.end(); mi++)
    printf("%d %g %d\n", mi->first, mi->second.sum/mi->second.count, mi->second.count);
}