/** Program name. */

#define PROG_NAME "if2e7"



/** Program version. */

#define PROG_VERSION "2.4.3"



/** Copyright. */

#define COPYRIGHT "(c) 2005-2008 by Turku PET Centre"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#include <direct.h>
#include <sys/types.h>
#define stat _stat
#define access _access
#define mkdir _mkdir
#define F_OK 0
#define R_OK 4
#else
#include <unistd.h>
#include "splitpath.h"
#define _MAX_PATH 256
#define _MAX_DRIVE 0
#define _MAX_DIR 256
#define _MAX_EXT 256
#define _MAX_FNAME 256
#define _splitpath splitpath
#endif

#include "../ecatx/matrix.h"               /* ecatx lib (ms): handling ECAT format     */
#include "../ecatx/isotope_info.h"         /* ecatx lib (ms): physcal nuklid constants */
#include "interfile_reader.h"                     /* reading Interfile header */
#include "calibration_table.h"                       /* find calibration file */

/*******************************************************************************
SYNTAX:
   see void usage ()

DESCRIPTION:
   This program calibrates HRRT interfile images (if there is calibration
   information available) and converts them to the ECAT7 format.

   Conversion and calibration of the data include
   a) looking for all frames belonging to the study
   if the '-x' switch is set (no corrections) continue with i)
   b) read relevant values from the interfile header
   c) normalise for differnt acquisition times, i.e. calibrate from 
      counts to counts/s
   d) decay correction as well for frame duration (in frame) as to scan start
   if there is calibration information available and attenuation and scatter
   correction done:
   e) normalise for different slice sensitivity
   f) correct for branching fraction
   g) correct for deadtime
   h) if the -m switch is set, actually calibrate image matrix pixels
      (to Bq/ml for example), i.e. multiply them with the calibration
      factor. Otherwise the factor is only written to the header and the
      multiplication is done by the image tool.
   i) fill the header fields of the ECAT7 file correctly (calibration factors
      etc)

   The ECAT7 subheader field 'processing_code' contains the information
   which corrections actually have been applied to the data.

   For historical reasons the scan start time, framing information and
   the isotope can be specified in the command line. Normally they are
   taken from the interfile header.

   There are two ways to specify the calibration factor:
   1. from the command line with the -c switch (highest priority)
   2. from a slice sensitivity calibration file with the -s switch
   The program assumes that the calibration factor is given in 
   (Bq/ml)/(counts/s)

   The slice sensitivity values have to be specified in a slice 
   sensitivity file with the -s switch. (see FILES section)

   The units of the calibrated images can be chosen with the -u switch.

   The scanner specific default values (bin size, numAngles etc) can be
   set from the command line with the -t switch.
   (just to fill the header correctly)

   This program makes some assumptions about the input image filenames.
   It expects something of the form A_B_C_(...).Z with A = study name.
   HRRT: If the filename contains one field Y of the form '_framex_'
         the program assumes that it is a dynamic study and includes
         all other frames into the ECAT7 file.


KNOWN BUGS:
   The "scan start time" is interpreted differently by different
     programs/systems (UNIX, Windows XP). Sometimes it is supposed to
     be GMT sometimes local time. So the time in the ECAT7 header may be wrong 
     (two or three hours too high in Turku).
   Calibrating images directly with the CPS reconstruction software
     ('Recon GUI') presently (04/2005) does not work. So the program
     cannot check if the interfile images are already calibrated
     (keywords not known).

FILES:
   Slice Sensitivity file (-s switch) (containing necessary calibration data):
     The program expects the fiel to ave the following syntax:
     1. it has to be an ASCII file
     2. lines beginning with a ';' are ignored (comments)
     3. empty lines are ignored
     4. other lines are expected to have the format
           keyword := value
        Keywords different from 'calibration factor' and 
        'efficient factor for plane x' where x are integers (=plane numbers,
        starting with plane 0) are ignored.
        Example for a slice sensitivity file:
           ; This is a slice sensitivity file, 5 iterations, 14 subsets, mrp=0.3
 
           ; calibration factor in (Bq/cc)/(counts/s)
           calibration factor := 6.52630e+06

           ; slice sensitivity values for planes
           ; plane := value
           efficient factor for plane 0 := 0.985115
           efficient factor for plane 1 := 1.013143
           (...)

   PET Centre Libraries:
     libtpcimgio - for reading interfile and writing ECAT7
     libtpcmisc  - for big/little endian handling

   HRRT interfile:
     The programs expects to find the following keywords in HRRT headers:
     !originating system, calibration file header name used, norm file used,
     atten file used, scatter file used, data format, !study date (dd:mm:yryr),
     !study time (hh:mm:ss), Patient name, Patient DOB, Patient sex, Dose type,
     isotope halflife, branching factor, Dosage Strength, !PET data type

     energy window lower level[1], energy window upper level[1],
     sinogram data type, number format, matrix size [1], matrix size [2],
     matrix size [3], scaling factor (mm/pixel) [1],
     scaling factor (mm/pixel) [2], scaling factor (mm/pixel) [3],
     image relative start time, image duration, decay correction factor,
     decay correction factor2, Dead time correction factor, 
     reconstruction method, number of iterations, number of subsets

SEE ALSO:
 
HISTORY/VERSION:
   Authors:
     01/2004 Roman Krais (krs), PET keskus, Turku, www.TurkuPETCentre.fi
     10/2005 Jarkko Johansson (jj), Turku PET Centre
     05/2006 Floris van Velden (vv), VU University Medical Center Amsterdam
     06/2007 Jorgen Kold (jk), AArhus PET Centre
     06/2007 Kim Vang Hansen (kv), AArhus PET Centre
     02/2008 Merence Sibomana (ms), HRRT Users Community
   2004-02-10 (krs) 
     conversion for HRRT interfile dynamic images added (no longer STIR only)
     bug fix: 'buffer' was not initialised again after use. Corrected.
   2004-02-17 (krs) 
     bug fix: slice sens values for FLB-Hietala corrected (reciprocal!)
   2004-02-18 (krs) 
     -r and -p switch added for reducing image size (e.g. in animal studies)
   2004-02-25 (krs)
     bug fix: branching fraction was neglected in calibration. Now it is used.
     Ga-68 as isotope option added.
   2004-03-12 (krs) version 1.0
     HRRT images need decay correction. Added. 
     HRRT images need dead time correction. Added.
   2004-09-29 (krs) version 1.0.1
     detected: Scan Start Time in ecat 7 file is one hour too high. 
     Reason unclear. Nothing changed.
     HRRT interfile header keyword 'image start time' changed to 
     'image relative start time'. Adapted.
   2005-03-10 version 1.1
     (krs) length of input file name rased from 50 to 256
     (Jarkko Johansson) print out telling the version and the build date of 
     libpet added
   2005-04-08 (krs) version 2.0
     STIR images no longer supported!
     a new function (library) is used for reading interfile
     new libraries libtpcimgio and libtpcmisc used instead of libpet.
     new features:
       switch -a to make image anonymous (patient name, ID and birth date)
       only part of the frames can be included in the ecat7 file
       default now kBq/ml
       Bq/ml, kBq/ml, MBq/ml and MBq/cc also accepted as valid units now 
       conversion without calibration if there is no calibration file
       or calibration factor specified.
       Images are not calibrated if they are already calibrated.
       support for full filenames (including path)
     bug fixes etc:
       now correct scan start time (daylight saving time allowed for)
       file name may contain now 50 fields (before 10)
   2005-04-12 (jj) version 2.0.1
     Help text re-formatted to Turku PET Centre style.
   2005-05-12 (jj) version 2.0.2
     Option --version for printing out the build information added.
     Compiled with the new merged libraries:
        libtpcimgio_1.a (Windows: MinGW)
        libtpcimgmisc_1.a (Windows: MinGW)
     --> long command lines are supported in Windows command prompt
   2005-05-18 (jj) version 2.0.3
      Bug fixes:
         - sliceSensDim parsed from the calibration header increased
           by one to match with number of planes
         ((krs) major bug fix! (causing occasionally segmentation faults)
          The array sliceSensDim had one field less than it was supposed to 
          have, causing the program to write nonsense to some places in the
          memory when a calibration file was specified.) 
   2005-06-22 (krs) version 2.1.0
      new features: 
         correction for decay and acquisition duration is always done
         if the isotope halflife and acquisition duration is known.
         switch to supress any corrections
      bug fixes etc:
         main header entry 'Calibration Units' was used incorretly. Now:
         If the image is calibratet 'Calibration Units' = 1, otherwise 0
         processCode was not reset for each frame. So there were wrong
         values in the subheader for all except the first frame. Fixed.
   2005-06-29 (krs) version 2.2.0
      bug fixes etc:
         reduction in size to an odd matrix size (odd imgdim in -p option,
         e.g. 63x63 or 65x65) produced nonsense images. Fixed.
   2005-10-07 (jj) version 2.2.1
      bug fixes etc:
         Looking for frames studyname_??_framex_??.i fixed.
         Added fflush(stdout) for MSYS.
   2005-11-04 (jj) version 2.2.2
      bug fixes:
         Instead of looking only for keyword 'scatter file used' in interfile
         header, program looks for 'scatter file used' and 'scatter file name
         used'. And same for attenuation file used.
   2006-05-31 (fvv) version 2.2.3 
      (Floris van Velden, VU University Medical Center Amsterdam, The Netherlands)
      new features:
         Added patient age and an option to define a maximum pixel value 
         (recommended 0.015 cnts/sec, set by option -q <value>).
      bug fixes:
          Changed the way of calculating patient date of birth, so it also works
          for Windows systems (mktime fix) and is fixed for some odd values
          produced by CTI's ScanIt.

  2007-06-11 (jk) version 2.2.0(special)
          enabled input of patient weight and dose_start_time. 

  2007-08-11 (kv & jk) version 2.2.5 
          Enabled converting data when there are missing frames.
          Enabled an option [-l] for the patient weight and dose start time.
          Bug fix of -w option. It crashed when there where missing frames 
          before startFrame.
  2008-03-18 (ms) version 2.3.0
          Merged modifications from Aarhus (kv&jk)
          Merged modifications from Amsterdam(vv)
          Replaced Turku libraries by EcatX library
          Added "-F" option to specify facility name 
          Added "-D" option to specify a second (duplicate) storage directory
          Added "-T" option to trim the image using umap boundaries
  2008-05-22 (ms) 
          bug fix: Make sure to allocate 512 blocks memory for trimmed images.
          bug fix: multiframe copy to storage directory 
  2008-10-02 (ms)
          Use mu-map to mask hot pixels and set their values to 0
          Add "-S" option to specify directory to search calibration file
                             corresponding to study date
  2008-12-22 (ms)
          Bug fix: Last block missing in secondary storage (-D) file
  2009-05-04 (krs) version 2.4.0
    Bug fixes:
      - -r options for reducing size created corrupt (shifted) images
        when dimx=dimxOutfile (x/y matirx size not changed). Fixed.
      - definition of 'calibration_units' in Ecat7 mainheader:
        0=uncalibrated, 1=calibrated in Bq/ml, 2="calibrated" in 'data_units'
        (some programs asume '1' to be Bq/cc)
      - the interfile_find function returns the values of an interfile header
        from a Windows system with an '\r' in the end, e.g. 'HRRT\r' 
        instead of 'HRRT'. New function RemoveCRfromEndOfString to corrects it
      - dataUnits were read from sliceSensFile. Does not make sense. Fixed.
    New featrues:
     - HRRT multiframe studies: Expected syntax before: 'xxx_frameX_xxx'
       Now also syntax 'xxx_frameX.xxx' accepted.
     - more log messages
     - '-L ' switch added to create log file

2009-07-20 (Kim Vang Hansen & Jørgen Kold, Aarhug PET Centre) version 2.4.1
           Changed the time of injection code. Before when typing time 
           including eg. 08 or 09 time went wrong. Now this is possible.
           Reason was that we used scanf where 08 is end of line.
2009-07-20: Add patient height (Jørgen Kold,Aarhug PET Centre) version 2.4.2
2009-07-20: Remove '\r' in Interfile reading instead of multiple  RemoveCRfromEndOfString
            function calls (Merence Sibomana) version 2.4.2
2009-08-03: Bug fix plane sensitivity not applied: move up calibration directory lookup 
            and sensitivity read to be applied

2010-09-21: Added data data rates (prompt,random,single) and scatter fraction
            to image header; and changed sw_version from 72 to 73 in main header
            (Merence sibomana) - changed version to 2.4.3

     - to do: optons to include radiopharmaceutical, date in outfilename

  Remember to change version number in very first lines of this file!
*******************************************************************************/

#define MAX_CMD_LEN 1024

void split_line(char *line, char *key, char *val);
void if2e7_itoa(int n, char *s);
void print_build(); // (jj) 2005-05-12

static char command[MAX_CMD_LEN];
static char inFile[_MAX_PATH];                       /* image filename (input file) */

static short int verboseMode, multPixWithCalibFactor, anonymousMode, correctionMode;
static float     calibFactor=0.0f;       /* scales counts/s to activity concentration */
static char      frameInfo[160], dataUnits[32]; 
static char      field[50][128]={"","","","","","","","","",""};
static char      inFileTail[256], outFile[256], outFileTail[256];/* filenames */
static char      logFileName[256], logMsg[1000];
static FILE      *logFile=NULL;


///////////////////////////////////////////////////////////////////////////////
void usage(char *cmd)
{                             /* if called without arguments */
    printf("%s %s %s\n",PROG_NAME,PROG_VERSION,COPYRIGHT);
    printf("\nSYNTAX: \n");
    printf(cmd);
    printf("\n      [--version] [-v] [-L logfile] [-m] [-x] [-t scanner]\n");
    printf("      [-a [anonyName]] [-d dd mm yyyy] [-f f1,..,fn] [-i isotope]\n");
    printf("      [-g witdh] [-c calibfactor] [-s slicesensfile] [-u units]\n");
    printf("      [-r startplane endplane] [-p pixel x0 y0] [-w startframe endframe]\n");
    printf("      [-o outfile] [-e deadtimeExponent] [-q maxpixel]\n");
    printf("      [-F FacilityName] [-D DuplicateDir] [-T pixel] [-l] \n");
    printf("      interfile_image\n" );
    printf("\n");
    printf("The program converts interfile images to ECAT7 format.\n");
    printf("The Output images are quantified if there is calibration information available.\n");
    printf("\n");
    printf("-a [anonyName] anonymous. Erases patient information from the output image\n");
    printf("               (patient name, ID and birth date). <anonyName> will be\n");
    printf("               placed in the 'patient name' filed, if specified.\n");
    printf("-c calibfactor factor which scales counts/s to Bq/ml\n");
    printf("-d dd mm yyyy  date of scan. Obsolete as it can be taken from the header.\n");
    printf("               The date must be in the format dd mm yyyy, e.g. '-d 31 1 1999' \n");
    printf("-F facility_name Facility name (left blank by default)\n");
    printf("-D DuplicateDir Secondary Storage directory (none by default)\n");
    printf("-f f1,..,fn    framing information. Obsolete as it can be taken from the header.\n");
    printf("               Frame length of frame 1 to n in sec, numbers are sperated by ','.\n");
    printf("               Example: '-f 120,300,300' for 3 frames with 2min, 5min and 5min\n");
    printf("-e deadtimeExponent     \n ");
    printf("              Scan-dependent deadtimeExponent to be multiplied by average singles \n");
    printf("               per block in the header, exp(...) of which yields the deadtime \n");
    printf("               correction factor (default is 8.94E-6).\n");

    printf("-l             Enables a menu for user input of injected dosage,\n");
    printf("               patient weight and dose start time\n");     
    printf("-L logfile     writes log messages to specified logfile\n");
    printf("-g gsmooth_width  width of blurring kernel in mm used by the gsmooth function.\n");
    printf("-i isotope     Obsolete as it can be taken from the header.\n");
    printf("               <isotope> can be 'C-11', 'O-15', 'F-18' 'Ga-68' or 'Ge-68' \n");
    printf("-m             multiplies matrix pixel values with <calibFactor>.\n");
    printf("               Obsolete as image tools (MEDx, Vinci, YaIT) and the conversion\n");
    printf("               program to Analyse format do the calibration internally.\n");
    printf("-p pixel x0 y0 reduces the size of the ECAT7 image. This is useful for \n");
    printf("               studies, where the outer parts of a plane contain only '0's.\n");
    printf("               The new image centre will be at pixel (<x0>/<y0>). The pixel size\n");
    printf("               is unchanged. <pixel> is the x-y-dimension of the ECAT7 image.\n");
    printf("               Example: '-p 64 128 128' for a 64x64 image cut out of the\n");
    printf("               centre of a 256x256 image.\n");
    printf("-r start end   reduces the number of slices in the ECAT7 image. This is\n");
    printf("               useful for studies where most planes contain only '0's.\n");
    printf("               <start> is the number of the first plane to take <end> the\n");
    printf("               number of the last. (First plane is plane 0)\n");
    printf("               Example: '-r 0 9' for the first 10 slices only\n");
    printf("-T             Trim image to dimension using transmission image boundaries\n");
    printf("               This option is a combines -p and -r, the center x0,y0 and \n");
    printf("               plane boundaries are obtained from the transmission image\n");
    printf("-s filename    reads the slice sensitivity values from file <filename>.\n");
    printf("               The file has to be an ASCII file with lines 'keyword := value'.\n");
    printf("               'calibration factor' and 'efficient factor for plane x', where\n");
    printf("               'x' are integers (i.e. plane numbers, starting with plane 0),\n");
    printf("               are the only relevant keywords. ';' marks a comment line.\n");
    printf("               Example for a slice sensitivity file:\n");
    printf("                  ; This is a slice sensitivity file, 3D OSEM, 5 iter, 14 subs\n");
    printf("                  \n");
    printf("                  ; calibration factor in (Bq/ml)/(counts/s)\n");
    printf("                  calibration factor := 6.52630e+06\n");
    printf("                  ; plane := value\n");
    printf("                  efficient factor for plane 0 := 0.985115\n");
    printf("                  efficient factor for plane 1 := 1.01314\n");
    printf("-S directory   directory to search for scan date calibration/sensitivity file\n");
    printf("-t scanner     sets defaults for several scanner specific fields like plane\n");
    printf("               separation, bin size, number of planes etc. and determines\n");
    printf("               the kind of interfile format. Options for <scanner> are 'HRRT'.\n");
    printf("               If not specified it is taken form the image file name extension;\n");
    printf("               '.i' for HRRT.\n");
    printf("-u units       data units in the calibrated ECAT7 image.\n");
    printf("               Options for <units> are 'Bq/ml', 'kBq/ml' (default) and 'MBq/ml'\n");
    printf("-v             verbose mode. Prints useful information to screen. \n");
    printf("--version      print build information and exit(0). \n");
    printf("-w start end   reduces the number of frames in the ECAT7 image. This is\n");
    printf("               useful if one needs only a part of a dynamic study.\n");
    printf("               <start> is the number of the first frame to take <end> the\n");
    printf("               number of the last. (First frame is frame 1)\n");
    printf("               Example: '-w 1 3' for the first 3 frames only\n");
    printf("-x             supress all corrections (also decay and duration)\n");
    printf("\nExplicit command line switches override the values from the header.\n");
    printf("\n");

    printf("The interfile_image filename is expected to consist of several fields\n");
    printf("separated by '_'. The last '.' is the beginning of the file name extension.\n");
    printf("HRRT-image filenames have the form RB9999_bla_framex_bla.i. The first field is\n");
    printf("expected to be the study name. Dynamic studies must contain a field 'framex'.\n");
    printf("The extension for the header is '.i.hdr' (e.g. RB9999_bla_framex_bla.i.hdr).\n");
    printf("\n");

    printf("It is enough to specify one image (frame) in the command line as the programm\n");
    printf("looks for all other frames (e.g. RB9999_bla_framex_bla.i).\n");
    printf(" \n");

    printf(cmd);
    printf("\n      [--version] [-v] [-L logfile] [-m] [-x] [-t scanner]\n");
    printf("      [-a [anonyName]] [-d dd mm yyyy] [-f f1,..,fn] [-i isotope]\n");
    printf("      [-g witdh] [-c calibfactor] [-s slicesensfile] [-u units]\n");
    printf("      [-r startplane endplane] [-p pixel x0 y0] [-w startframe endframe]\n");
    printf("      [-o outfile] [-e deadtimeExponent] [-q maxpixel]\n");
    printf("      [-F FacilityName] [-D DuplicateDir] [-T pixel] [-l] \n");
    printf("      interfile_image\n" );
    printf("\n");
    exit(0);
}

/*****************************************************************************/
/*!
 * Write ECAT 7.x image or volume matrix header and data
 *
 * @param fp		output file pointer
 * @param matrix_id	coded matrix id
 * @param h		Ecat7 image header
 * @param fdata 	float data to be written
 * @return 0 if ok.
 */


///////////////////////////////////////////////////////////////////////////////
void Logging(char *logMessage)
/* 
This function writes log messages to stdout and/or logfile.
*/
{
  if ((int)strlen(logFileName) > 0) {
    fprintf(logFile,logMessage); 
    fflush(logFile);
  }
  if (verboseMode == 1) {
    printf(logMessage);
    fflush(stdout);
  }
}

///////////////////////////////////////////////////////////////////////////////
void RemoveCRfromEndOfString(char *stringToEdit)
/* 
This function removes carriage return '\r' from the end of a string if
there is one. E.g 'HRRT\r' gets 'HRRT'. This is necessary as reading
the valus from a interfile header creates varaibles with a '\r' at the end.
*/
{
  if (stringToEdit[(int)(strlen(stringToEdit))-1] == '\r') {
    stringToEdit[(int)(strlen(stringToEdit))-1] = '\0';
  }
}


///////////////////////////////////////////////////////////////////////////////
unsigned char *CreateHRRTMuMask(const char *filename)
{
    struct stat st;
    int x=0,y=0, dim=128, z=0, nplanes=207;
    unsigned char *mask=NULL, *pmask=NULL;
    float *plane=NULL;
    FILE *fp=NULL;
    int mu_size = dim*dim;
    int plane_size = dim*dim;
    int skip_planes = 10;
    float mu_threshold = 0.07f;   // above bed, use only tissue voxels
      // check if file has expected size 128x128x207 or 256x256x207 float
    if (stat(filename,&st) == -1) return NULL;
    if (st.st_size != mu_size*nplanes*sizeof(float) &&
      st.st_size != mu_size*4*nplanes*sizeof(float)) return NULL;
    if (st.st_size == mu_size*4*nplanes*sizeof(float)) plane_size *= 4;   // input is 256x256
     
    if ((fp=fopen(filename, "rb")) == NULL) return NULL;
    
    // allocate 256x256x207 mask and plane buffer
    mask = (unsigned char*)calloc(mu_size*nplanes*4, 1); 
    plane = (float*)calloc(plane_size,sizeof(float));
    
    // skip first planes
    if (fseek(fp, skip_planes*plane_size*sizeof(float), SEEK_SET) == 0)
      pmask = mask+skip_planes*mu_size*4;
    for (z=skip_planes; z<nplanes-skip_planes; z++)
    {
      if (fread(plane,sizeof(float),plane_size,fp) == plane_size)
      {
        float *p=plane;
        if (plane_size==mu_size)
        { // expand 128x128 to 256x256
          for (y=0; y<dim; y++) 
          {
            // process current line
            for (x=0; x<dim; x++, p++)
            {
              if (*p>mu_threshold) pmask[2*x]=pmask[2*x+1]=1; 
            }
            // duplicate line
            memcpy(pmask+(2*dim), pmask, 2*dim);
            // advance 2 lines
            pmask += 4*dim;
          }
        }
        else // same size
        {
           for (x=0; x<plane_size; x++)
             if (plane[x]>mu_threshold) pmask[x]=1;
           pmask += plane_size;
        }
      } // end plane
    } // end all planes
    fclose(fp);
    free(plane);
//    if ((fp = fopen("TXMask.i", "wb"))!=NULL) {fwrite(mask,256*256,nplanes,fp); fclose(fp);}
    return mask;
}


///////////////////////////////////////////////////////////////////////////////
int ecat7WriteImageMatrix(MatrixFile *fp, int mat_id, Image_subheader *imh, 
                          float *fdata, unsigned char *mask)
{
    int i, nvoxels;
    float minval, maxval, scalef;
    MatrixData *matrix=NULL;
    short *sdata=NULL, smax=32766;  // not 32767 to avoid rounding problems
    int nblks = 0;

    
    
    matrix = (MatrixData*)calloc(1,sizeof(MatrixData));
    matrix->xdim = imh->x_dimension;	
    matrix->ydim = imh->y_dimension;
    matrix->zdim = imh->z_dimension;
    matrix->pixel_size = imh->x_pixel_size;
    matrix->y_size = imh->y_pixel_size;
    matrix->z_size = imh->z_pixel_size;
    imh->data_type = matrix->data_type = SunShort;
    matrix->data_max = imh->image_max*matrix->scale_factor;
    matrix->data_min = imh->image_min*matrix->scale_factor;
    matrix->data_size = matrix->xdim*matrix->ydim*matrix->zdim*sizeof(short);
    nblks = (matrix->data_size + MatBLKSIZE-1)/MatBLKSIZE;
   	matrix->data_ptr = (caddr_t)calloc(nblks, MatBLKSIZE);
    minval = maxval = fdata[0];
    nvoxels = matrix->xdim*matrix->xdim*matrix->zdim;
    if (mask != NULL)
    {  // Use mask to find image extrema and zero hot or cold pixels
      for (i=0; i<nvoxels; i++)
      {
        if (mask[i] && fdata[i]>maxval) maxval = fdata[i];
        if (mask[i] && fdata[i]<minval) minval = fdata[i];
      }
      // truncate hot and cold pixels
      for (i=0; i<nvoxels; i++)
      {
        if (!mask[i])
        {
          if (fdata[i]>maxval) fdata[i] = maxval;
          else if (fdata[i]<minval) fdata[i]=minval;
        }
      }
    }
    else
    {  // find image extream in whole image
      for (i=0; i<nvoxels; i++)
      {
        if (fdata[i]>maxval) maxval = fdata[i];
        if (fdata[i]<minval) minval = fdata[i];
      }
    }
    
    if (fabs(minval) < fabs(maxval)) scalef = (float)(fabs(maxval)/smax);
    else scalef = (float)(fabs(minval)/smax);
    sdata = (short*)matrix->data_ptr;
    for (i=0; i<nvoxels; i++)
        sdata[i] = (short)(0.5+fdata[i]/scalef);
    imh->image_max = find_smax(sdata,nvoxels);
    imh->image_min = find_smin(sdata,nvoxels);
    matrix->scale_factor = imh->scale_factor=scalef;
    matrix->data_max = imh->image_max*matrix->scale_factor;
    matrix->data_min = imh->image_min*matrix->scale_factor;
    matrix->shptr = (caddr_t)calloc(sizeof(Image_subheader),1);
    memcpy(matrix->shptr, imh, sizeof(Image_subheader));

    
    
    return matrix_write(fp,mat_id,matrix);
}

////////////////////////////////////////////////////////////////////////////////
int getBoundaries(unsigned char *muMask, short *Plane, short *endPlane, short *dimxOutfile, 
                  short *xCentre, short *yCentre)
{
  return 0;
}


///////////////////////////////////////////////////////////////////////////////
static int menu_input(Main_header *mh)
{
/******************************************************************************/
/*           Contribution from by Jorgen Kold, Aarhus PET Centre              */
/*           takes input from stdin to be used as time and weight values      */
/******************************************************************************/
  char      slut, choice, cls[5], valg;     /* (jk), Aarhus PET Centre */
  int       hh, mm, ss;
  char line[20];

  struct tm input_time, *p_time;
  time_t dose_time = mh->dose_start_time;

  slut = '1';   /* any character not '0' */
  if ((p_time=gmtime(&dose_time)) != NULL)
    memcpy(&input_time,p_time,sizeof(input_time));
  else memset(&input_time,0,sizeof(input_time));

  hh = input_time.tm_hour;
  mm = input_time.tm_min; 
  ss = input_time.tm_sec;   

  while (slut != '0')
    {
      sprintf(cls,"cls");
      system(cls);
      printf("**************************************************\n");
      printf("Please type injected dosage: ");    
      scanf("%f", &mh->dosage);  
      
      sprintf(cls,"cls");
      system(cls);
      printf("**************************************************\n");
      printf("Please type patient weight: ");  
      scanf("%f", &mh->patient_weight); 
      
      sprintf(cls,"cls");
      system(cls);
      printf("**************************************************\n");
      printf("Please type patient height(cm): ");   
      scanf("%f", &mh->patient_height);  

      system(cls);
      printf("**************************************************\n");
      printf("Dose start time is: %i:%i:%i\n", hh, mm, ss);
      printf("**************************************************\n");
      printf("1. Type new\n");
      printf("2. Use existing\n\n");
      printf("Your choice: ");
      choice=getchar();
      choice=getchar();
      system(cls);
      
      switch (choice)
      {
      case '1': 
        printf("***********************************************\n");
        printf("Dose start time is: %i:%i:%i\n", hh, mm, ss);
        printf("***********************************************\n\n");
        printf("Type new dose start time(hh:mm:ss): ");
        fflush(stdin);
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%2d:%2d:%2d",&hh,&mm,&ss);
        
        /*******************************************************************/
        input_time.tm_hour = hh; 
        input_time.tm_min = mm; 
        input_time.tm_sec = ss;   
        mh->dose_start_time = (unsigned)mktime(&input_time); 
        /*********************************************************************/
                 
        system(cls);
        printf("**************Summary*************************\n");
        printf("Weight = %.2f\n",mh->patient_weight);
        printf("Dose start time = %i:%i:%i\n", hh,mm,ss);
        printf("Injected dosage = %.4f\n",mh->dosage);
        printf("Patient height = %.2f\n", mh->patient_height);
        printf("**************************************************\n");
        break;
      case '2':
        mh->dose_start_time = mh->scan_start_time;
        system(cls);
        printf("**************Summary*************************\n");
        printf("Weight = %.2f\n",mh->patient_weight);
        printf("Dose start time = %i:%i:%i\n", hh, mm, ss);
        printf("Injected dosage = %.4f\n",mh->dosage);
        printf("Patient height = %.2f\n", mh->patient_height);
        printf("**************************************************\n");
        break;
      }
         
      printf("1. Ok\n");
      printf("2. Change values\n\n");
      printf("Your choice: ");
      valg=getchar();
      valg=getchar();
      
      switch (valg)
      {
      case '1':
        slut='0';
        break;
      case '2':
        slut='1';
        break;
      }
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////////
static char *date2str(time_t *t)
{
  struct tm *tm;
  static char str[12];       /* Date format YYYYMMDD */

  tm = localtime(t);
  if (tm != NULL)
  {
    sprintf(str,"%d",tm->tm_year+1900);
    if (tm->tm_mon<9) sprintf(str+4,"0%d",tm->tm_mon+1);
    else sprintf(str+4,"%d",tm->tm_mon+1);
    if (tm->tm_mday<10) sprintf(str+6,"0%d",tm->tm_mday);
    else sprintf(str+6,"%d",tm->tm_mday);
    return str;
  }
  return NULL;
}


///////////////////////////////////////////////////////////////////////////////
static int copyFile(char *in_fname, char *dest_dir, Main_header *mh)
{
  char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT]; 
  char patient_dir[_MAX_DIR];
  char *p=NULL, *study_date_str = NULL, *first_name=NULL;
  char path[_MAX_PATH];
  int i=0, len=0;
  time_t t;
  caddr_t buf=NULL;
  FILE *in=NULL, *out=NULL;
  int retOK=1;

        // Fail if not valid patient name
  len = 0;
  for (p=mh->patient_name; *p != '\0' ; p++)
    if (isalnum(*p)) len++;
  if (len==0) {
    fprintf(stderr,"Error creating patient directory: Invalid patient name");
    return 0; 
  }

  sprintf(patient_dir, "%s\\", dest_dir);
  len = strlen(patient_dir);
  first_name = strchr(mh->patient_name, ',');
  if (first_name != NULL) {
    // copy last and first names
    for (p=mh->patient_name; p<first_name; p++, len++) patient_dir[len] = *p;
    for (p=first_name+1; *p !=  '\0' ; p++, len++)
      if (isalnum(*p) == 0) patient_dir[len] = '_';
      else  patient_dir[len] = *p;
  } else {
    // copy unique name
    for (p=mh->patient_name; *p !=  '\0' ; p++, len++)
      if (isalnum(*p) == 0) patient_dir[len] = '_';
      else  patient_dir[len] = *p;
  }
  patient_dir[len++] = '\0';
  t = (time_t) mh->scan_start_time;
  len = strlen(patient_dir);
  for (i=strlen(dest_dir)+1; i<len; i++)
    if (isalnum(patient_dir[i]) == 0) patient_dir[i] = '_';
  study_date_str = date2str(&t);

  if (study_date_str != NULL) 
    sprintf(patient_dir+len, "_%s", study_date_str);
                  /*create patient directory if not existing */

#ifdef WIN32
  if (access(patient_dir, F_OK) != 0) 
    mkdir(patient_dir);
#endif

  _splitpath(in_fname, drive, dir, fname, ext);
  sprintf(path,"%s\\%s%s",patient_dir, fname, ext);
  if ((in=fopen(in_fname,"rb")) != NULL) {
    if ((out = fopen(path,"wb")) != NULL)  {
      buf = (caddr_t)calloc(1, MatBLKSIZE);

      while (retOK && fread(buf, MatBLKSIZE, 1, in) == 1) {
        if (fwrite(buf, MatBLKSIZE, 1, out) != 1) {
          perror(path);
          retOK = 0;
        }
      }
      fclose(out);
    } else {
      perror(path);
      retOK = 0;
    }
    fclose(in);
  } else {
    perror(in_fname);
    retOK = 0;
  }
  return retOK;
}


///////////////////////////////////////////////////////////////////////////////
static int read_sensitivity(const char *fname, float **pSliceSensitivity,
                            char *buffer, char *line, char *keyword, char *value)
{
  int i;
  FILE *sliceSensFile;
  float *sliceSensitivity;
  char chr;
  int sliceSensDim=0;

          if ((sliceSensFile = fopen(fname,"r"))==NULL) {
          fprintf(stderr, "ERROR opening slice sensitivity file %s.\n",fname);
          return 1;
        }
                                     /* look for highest plane number in file */
        sliceSensDim = 0;
        while (fread(&chr,1,1,sliceSensFile) == 1) {
                                         /* if the line is a comment or empty */
          if (chr == ';' || chr == '\r' || chr == '\n') { 
            while (chr != '\n') fread(&chr,1,1,sliceSensFile);
          } else {
            line[0] = '\0';             /* initialize line */
            i=0; 
            while (chr != '\r' && chr != '\n' && i < 516) {
              line[i] = chr;
              fread(&chr,1,1,sliceSensFile);
              i++;
              line[i] = '\0';
            }
                        /* each "valid" line must contain the field separator */
            if (strstr(line," := ") != NULL) {
                                           /* get keyword and value from line */
              split_line(line,keyword,value);
              if (!strncmp(keyword,"efficient factor for plane ",27)) {
                buffer[0]='\0';
                strncat(buffer,&keyword[27],4);  /* put plane nr in buffer */
                if (sliceSensDim < atoi(buffer)) sliceSensDim = atoi(buffer);
              }
            }
          } // else
        } //while fread

        if (sliceSensDim == 0) {
          fprintf(stderr, "ERROR: no slice sensitivity values found in %s\n",fname);
          return 1;
        }

	     /* enumeration in calibration header starts from 0 --> increment */
        sliceSensDim++;
        sprintf(logMsg,"  Number of slice sensitivity factors is %i\n",sliceSensDim);
        Logging(logMsg);
	                   /* allocate memory for sliceSensitivity (i slices) */
        sliceSensitivity = (float*)calloc(sliceSensDim, sizeof (float));
        for (i=0;i<sliceSensDim;i++) sliceSensitivity[i] = 1.0; /* initialize */

    	/* now read the values from file, rewind the file first */
        
        fseek(sliceSensFile,0,SEEK_SET);
        while (fread(&chr,1,1,sliceSensFile) == 1) {
                                         /* if the line is a comment or empty */
          if (chr == ';' || chr == '\r' || chr == '\n') { 
            while (chr != '\n') fread(&chr,1,1,sliceSensFile);
          } else {
            line[0] = '\0';
            i=0;
            while (chr != '\r' && chr != '\n' && i < 516) {
              line[i] = chr;
              fread(&chr,1,1,sliceSensFile);
              i++;
              line[i] = '\0';
            }
                                           /* get keyword and value from line */
            split_line(line,keyword,value);
            if (!memcmp(keyword,"calibration factor",17) && calibFactor == 0) { 
              calibFactor = (float)atof(value);
              sprintf(logMsg,"  calibration factor %g found - assuming it's in (Bq/ml)/(counts/s)\n",calibFactor);
              Logging(logMsg);
            } 
            else if (!strncmp(keyword,"calibration unit",15)) {
              sprintf(logMsg,"  %s := %s found - not used\n",keyword, value);
              Logging(logMsg);
            }
            else if (!strncmp(keyword,"efficient factor for plane ",27)) {
              buffer[0]='\0';
              strncat(buffer,&keyword[27],4);    /* put plane nr in buffer */
              if (atoi(buffer) > sliceSensDim) {
                fprintf(stderr, "ERROR: slice number is %i but should be <= %i ?"
                              "!? Check %s file\n\n",atoi(buffer),sliceSensDim,fname);
                fclose(sliceSensFile);
                return 1;
              }
              sliceSensitivity[atoi(buffer)] = (float)atof(value);
            }
          } // else
        } // while fread
        fclose(sliceSensFile);
	  //          Logging("    Slice sensitivity file read...\n");

   *pSliceSensitivity = sliceSensitivity; 
   return sliceSensDim;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
                                        /*** counters, auxiliary variables ***/
  short int i,k,l,x,y,z,ret,frame,pos,is_little;
  short int framePos;            /* position of the _frame_ field in filename */
  short int sliceSensDim;              /* dimension of sliceSensitivity array */
  short int frameDurDim;                  /* dimension of frameDuration array */
  short int dimx, dimy, dimz;                 /* dimension of interfile image */
  short int toCalibrate;     /* flag indicating if image has to be calibrated */
  long int  j;
  float     *sliceSensitivity = NULL;
  double    decayInFrame, decayFrameStart, frac ; /* decay correction factors */
  double    deadtimeCorFactor, deadtimeExponent;                /* dead time  correction factor */
  char      *str=NULL, chr, buffer[32], descr[128], ext[32];
  char      hdrName[256], fname[256], fnameTail[256]; 
  char      studyName[128];
  char      keyword[256], value[256], line[256];     /* interfile definitions */
                                /* for splitting file name into max 10 pieces */
  IFH_Table ifh;

                                                 /*** command line switches ***/
  short int scanStartDay, scanStartMonth, scanStartYear;
  short int startPlane, endPlane, dimxOutfile, xCentre, yCentre;
  short int startFrame, endFrame;
  char      scannerModel[32];                               /* scanner (HRRT) */
  char      anonymousName[128];          /* 'patient name' for anonymous data */
  char      studyType[32];   /* to set defaults for images from the same study*/
  char      *calibDir=NULL;                    /* image filename (input file) */

  short int menuFlag=0, trimFlag=0;
                                                   /*** ECAT7 header fields ***/
  struct IsotopeInfoItem *isotope_info = NULL;
  short int  transmissionSourceType, angularCompression;
  short int acquisitionType, coinSampMode, reconType;
  short int numPlanes, numRelements, numAngles, numFields, numFrames;
  short int scatterCor, scanStartHour, scanStartMinute, scanStartSecond;
  short int birthDay, birthMonth, birthYear, patientOrientation;
  long int  frameStart, *frameDuration = (long int*) NULL;
  float     distanceScanned, transaxialFOV, binSize, reconZoom;
  float     planeSeparation=0.0f, Dosage=0.0f, age=0.0f;
  float     pixelMax=0.0f;
  int       nOffset1=0, nOffset2=0;


                                                                    /* others */
  int       matrixId;
  float     *matrix_float,*matrix_float_reduced = (float*) NULL; /* img matrix*/


  FILE      *inputImage=NULL;
  MatrixFile *ECAT7file=NULL;
  Main_header    main_header;
  Image_subheader   image_header;
  char *duplicateDir = NULL;
  unsigned char *muMask=NULL;

  struct tm zeit;
  int gsmooth_width=0;
  time_t now;  


/******************************************************************************/
/*                                   Intro                                    */
/******************************************************************************/

  if (argc == 1) usage(argv[0]);

/******************************************************************************/
/*             Build information with --version                               */
/******************************************************************************/
  for(i=1; i<argc; i++)
  {
    if(strstr(argv[i], "--version")){
      print_build();
      exit(0);
    }
  }


/******************************************************************************/
/*                              default values                                */
/******************************************************************************/
  memset(&main_header,0,sizeof(main_header));
  memset(&image_header,0,sizeof(image_header));
  strncpy(main_header.magic_number, "MATRIX70", 14);
  main_header.sw_version = 73;           /* otherwise Vinci might not like it */
  main_header.file_type = 7;                /* let's create a Volume 16 image */
  angularCompression = 0;
  anonymousMode = 0;          /* default: header contains patient information */
  strcpy(anonymousName,"anonyymi");
  acquisitionType = 0;                                    /* default: unknown */
  binSize = 0;
  birthDay = 0;
  birthMonth = 0;
  birthYear = 0;
  calibFactor = 0.0;
  chr = '\0';
  coinSampMode = 0;                                     /* default: net trues */
  correctionMode = 1;                            /* default: corrections done */
  strcpy(dataUnits,"");
  deadtimeCorFactor = 1.0;  
  deadtimeExponent = 8.94E-6;                                /* default value */
  decayFrameStart = 1;
  decayInFrame = 1;
  strcpy(descr,"");
  dimx = 0;
  dimxOutfile = 0;
  dimy = 0;
  dimz = 0;
  distanceScanned = 0;
  endFrame = 0;
  endPlane = 0;
  framePos = 0;
  frameDurDim = 0;
  strcpy(frameInfo,"no");
  frameStart = 0;
  strcpy(logFileName,"");
  multPixWithCalibFactor = 0; /* dflt: don't multiply matrix with calibFactor */
  numAngles = 0;
  numFrames = 0;
  numPlanes = 0;
  numRelements = 0;
  patientOrientation = 8;                                          /* unknown */
  planeSeparation = 0;
  strcpy(main_header.radiopharmaceutical,"unknown");
  reconType=99;                                                    /* unknown */
  reconZoom=0.0;
  strcpy(scannerModel,"");
  scanStartDay = 0; 
  scanStartHour = 0; 
  scanStartMinute = 0;
  scanStartMonth = 1;
  scanStartSecond = 0;
  scanStartYear = 1900;
  scatterCor = 0;
  sliceSensDim = 0;
  startFrame = 0;
  startPlane = 0;
  strcpy(studyType,"");
  transaxialFOV = 0;
  transmissionSourceType = 0;
  toCalibrate = 0;
  verboseMode = 0;                               /* default: verbose mode off */
  xCentre = 0;        /* centre x pixel of output image - if image is reduced */
  yCentre = 0;        /* centre y pixel of output image - if image is reduced */
                                                        /* initialize strings */
  memset(inFileTail,0,sizeof(inFileTail));
  memset(outFile,0,sizeof(outFile));
  memset(outFileTail,0,sizeof(outFileTail));
  memset(fnameTail,0,sizeof(fnameTail));
  memset(studyName,0,sizeof(studyName));
  memset(buffer,0,sizeof(buffer));
  memset(ext,0,sizeof(ext));
  now = time(0);                                 /* get current time */

/******************************************************************************/
/*                        read command line arguments                         */
/******************************************************************************/
  while (--argc > 0 && (*++argv)[0] == '-') {
    for (str = argv[0]+1; *str!= '\0'; str++) {
      switch (*str) {
      case 'a':              /* let's be flexible and accept different cases: */
                     /* if2e7 -a -v interfile.i                       */
	                     /* if2e7 -v -a interfile.i                       */
	                     /* if2e7 -v -a "Donald Duck" interfile.i         */
        anonymousMode = 1;
        if (strncmp(*(argv+1),"-",1) && argc > 2) {
          strcpy(anonymousName,*(argv+1));
          argc--;
          argv++;
        }
        sprintf(logMsg,"image will be anonymised. Patient Name in header will be %s \n",
                  anonymousName);
        Logging(logMsg);
        break;

      case 'c':
        calibFactor = (float)atof(*(argv+1));
        argc--;
        argv++;
        sprintf(logMsg,"Calibration Factor: %g\n",calibFactor);
        Logging(logMsg);
        break;

      case 'd':
        scanStartDay = atoi(*(argv+1));
        scanStartMonth = atoi(*(argv+2));
        scanStartYear = atoi(*(argv+3));
        argc = argc - 3;
        argv = argv + 3;
        sprintf(logMsg,"Scan Start Time: %i.%i.%i\n",scanStartDay, scanStartMonth, scanStartYear);
        Logging(logMsg);
        break;

      case 'e':
        deadtimeExponent = atof(*(argv+1));
        argc--;
        argv++;
        sprintf(logMsg,"deadtimeExponent: %f\n", deadtimeExponent);
        Logging(logMsg);
        break;

      case 'D':
        duplicateDir = argv[1];
        argc--;
        argv++;
        break;

      case 'F':
        strncpy(main_header.facility_name,*(argv+1),
          sizeof(main_header.facility_name)-1);
        argc--;
        argv++;
        break;

      case 'f':
        strncpy(frameInfo,*(argv+1),159);
        strcat(frameInfo,",");        /* makes splitting of the string easier */
        argc--;
        argv++;
                                  /* check for number of frames: numFrames */
        i=0; numFrames=0;
        while (frameInfo[i] != '\0') {          /* check char by char for ',' */
          if (frameInfo[i] == ',') numFrames++;
          i++;
        }
                                          /* allocate memory for FrameDuration*/
        frameDurDim = numFrames + 1;
        frameDuration=(long int*)calloc(frameDurDim, sizeof (long int));
        frameDuration[0] =    0;                          /* field 0 not used */
        i=0; j=0; k=0;
        while (memcmp(&frameInfo[i],"\0",1)) {          /* fill frameDuration */
          if (!memcmp(&frameInfo[i],",",1)) {           /* if char is a comma */
            i++; j++; k=0;
            if (j >= frameDurDim) {
              fprintf(stderr, "ERROR: nubmer of frames is %li but should be <= %i.\n",j,frameDurDim);
              return 1;
            }
            frameDuration[j] = atoi(buffer);           /* Frame duration in s */
            for (l=0;l<32;l++) buffer[l] = '\0';   /* initialise buffer again */
            continue;
          }
          buffer[k] = frameInfo[i];
          k++;
          i++;
        }
        sprintf(logMsg,"%li frames. Frame duration (in s): %li",j,frameDuration[1]);
        for (i=2;i<frameDurDim;i++) {
          sprintf(str,",%li",frameDuration[i]);
          strcat(logMsg,str);
        }
        strcat(logMsg,"\n");
        Logging(logMsg);
        break;

      case 'g':
        gsmooth_width = atoi(*(argv+1));
        argc--;
        argv++;
        sprintf(logMsg,"gsmooth width (default 0): %d\n", gsmooth_width);
        Logging(logMsg);
      break;

      case 'i':
        if ( (isotope_info = get_isotope_info(argv[1])) == NULL)
          {
            fprintf(stderr, "ERROR: illegal isotope name: -i %s\n\n",argv[1]);
            return 1;
          }
        sprintf(logMsg,"isotope name: %s; isotope halflife: %g sec\n",
              isotope_info->name, isotope_info->halflife);
        Logging(logMsg);
        argc--;
        argv++;
       break;

      case 'l':
           menuFlag = 1;
           break;

      case 'L':
        strcpy(logFileName,*(argv+1));
        argc--;
        argv++;
        if ((logFile = fopen(logFileName,"w"))==NULL) {
          fprintf(stderr, "ERROR opening logfile %s.\n",logFileName);
          if (verboseMode == 1) {
            printf("No logfile will be created.\n");
          }
          strcpy(logFileName,"");
        } else {
          if (verboseMode == 1) {
            printf("started logging to file %s.\n",logFileName);
          }
          fprintf(logFile,"\n%s version %s started at %s",
            PROG_NAME,PROG_VERSION,ctime(&now));
        }
        break;
      case 'm':
        multPixWithCalibFactor = 1;
        Logging("pixel values will be multiplied with the calibration factor \n");
        break;

      case 'o':
        strcpy(outFile,*(argv+1));
        argc--;
        argv++;
        break;

      case 'p':
        dimxOutfile = atoi(*(argv+1));
        xCentre = atoi(*(argv+2));
        yCentre = atoi(*(argv+3));
        argc = argc - 3;
        argv = argv + 3;
        break;

      case 'q':
        pixelMax = (float)atof(*(argv+1));
        argc--;
        argv++;
        sprintf(logMsg,"Maximum pixel value: %g\n", pixelMax);
        Logging(logMsg);
      break;

      case 'r':
        startPlane = atoi(*(argv+1));
        endPlane = atoi(*(argv+2));
        if (startPlane > endPlane || startPlane < 0) {
          fprintf(stderr, "ERROR: condition 0 <= startPlane (%i) <= endPlane (%i) violated\n\n",startPlane,endPlane);
          return 1;
        }
        argc = argc - 2;
        argv = argv + 2;
       break;

      case 's':
        strncpy(fname,*(argv+1),160);
        argc--;
        argv++;
        sprintf(logMsg,"reading calib header %s\n",fname);
        Logging(logMsg);
        sliceSensDim = read_sensitivity(fname, &sliceSensitivity, buffer, line, keyword, value);
        break;

      case 'S':
        calibDir = strdup(*(argv+1));
        argc--;
        argv++;
        sprintf(logMsg,"calibration/sensitivity directory: %s\n",calibDir);
        Logging(logMsg);
        break;


      case 't':
        strcpy(scannerModel,*(argv+1));
        argc--;
        argv++;
        if (strcmp(scannerModel,"HRRT")) {
          fprintf(stderr,"ERROR: illegal scanner model: -t %s\n\n",scannerModel);
          return 1;
        }
        break;

      case 'T':
           trimFlag = 1;
           break;

      case 'u':
        strcpy(dataUnits,*(argv+1));
        argc--;
        argv++;
        if (strcmp(dataUnits,"Bq/ml") && strcmp(dataUnits,"kBq/ml") && 
          strcmp(dataUnits,"MBq/ml") && strcmp(dataUnits,"Bq/cc") && 
          strcmp(dataUnits,"kBq/cc") && strcmp(dataUnits,"MBq/cc")) {
            fprintf(stderr, "ERROR: illegal data unit: -u %s\n\n",dataUnits);
            return 1;
        }
        sprintf(logMsg,"image will be calibrated to %s \n",dataUnits);
        Logging(logMsg);
        break;

      case 'v':
        verboseMode = 1;
        printf("\n%s version %s started at %s",
          PROG_NAME,PROG_VERSION,ctime(&now));
        printf("verbose mode on \n");
        break;

      case 'w':
        startFrame = atoi(*(argv+1));
        endFrame = atoi(*(argv+2));
        if (startFrame > endFrame || startFrame < 0) {
          fprintf(stderr, "ERROR: condition 0 <= startFrame (%i) <= endFrame (%i) violated\n\n",startFrame,endFrame);
          return 1;
        }
        argc = argc - 2;
        argv = argv + 2;
       break;

      case 'x':
        correctionMode = 0;
        printf("no corrections will be done \n");
        break;

      default:
        fprintf (stderr, "ERROR: illegal option -%c\n", *str);
        argc = 0;
        return 1;
        break;
      } // end switch
    } // end for
  } // end while
  fflush(stdout);
   /* by now all switches should be processed and only the inputfile is left */
  if (argc > 1){
    fprintf(stderr,"ERROR: Don't know what is '%s' ?!? \n",argv[0]);
    fprintf(stderr,"       Please check the command line syntax.\n");
    return 1;
  }

  strcpy(inFile,*argv);
  if (argc == 0) {
    fprintf(stderr,"ERROR: No interfile image specified\n");
    return 1;
  }

    pos=0;    /* get the tail of the input file name for shorter log messages */
    for (i=0; i<256; i++) {            /* find position after last '/' or '\' */
      if (!memcmp(&inFile[i],"",1)) break;
      if (inFile[i] == '/' || inFile[i] == '\\') pos=i+1;
    }

    k=0;                                              /* get tail of filename */
    for (i=pos; i<256; i++) {
      if (!memcmp(&inFile[i],"",1)) break;
      inFileTail[k] = inFile[i];
      k++;
    }
  sprintf(logMsg,"processing image %s\n",inFileTail); 
  Logging(logMsg);
                                                            /* split filename */
             /* assumption: valid file name has the form A_B_C_(...).Z with   */
             /* Z = file name extension (.i for HRRT)                         */
             /* for HRRT (e.g. REH0003_sp3_frame0_3D.i):                      */
             /* A = study name, X = frame number (_framex), rest description  */
  pos=0; k=0;                       /* look for the file name extension first */
  for (i=0; i<256; i++) {           /* find position of last '.' in file name */
    if (!memcmp(&inFile[i],"",1)) break;
    if (inFile[i] == '.') pos=k;
    k++;
  }
  k=0;                                                       /* get extension */
  for (i=pos+1; i<256; i++) {
    if (!memcmp(&inFile[i],"",1) || k >= 32) break;
    ext[k] = inFile[i];
    k++;
  }

  sprintf(logMsg,"file name extension is .%s\n",ext); 
  Logging(logMsg);
  numFields=0;k=0;              /* now split the file name itself into pieces */
    /* works also somehow when full filename is given C:\Recon-Jobs\...\xxx.i */
  for (i=0; i<pos; i++) {
    if (numFields >= 50) {
      fprintf(stderr, "ERROR: Can't handle input filename %s (more than 50 fields).\n\n",inFileTail);
      return 1;
    }    
    if (inFile[i] == '_') {
      numFields++; k=0; 
      continue;
    }
    field[numFields][k] = inFile[i];
    k++;
  }
  numFields++;
  fflush(stdout);

/******************************************************************************/
/*             take care of some values which are not set yet                 */
/******************************************************************************/

                                                  /*construct header filename */

  strcpy(hdrName,inFile); strcat(hdrName,".hdr"); 
                                  /*Initialize table and load header filename */
  ifh.tags = ifh.data=NULL;
  ifh.size=0;
  switch(interfile_load(hdrName,&ifh))
  {
      case IFH_FILE_INVALID:                /* Not starting with '!INTERFILE' */
          fprintf(stderr, "%s: is not a valid interfile header\n",hdrName);
          return 1;

      case IFH_FILE_OPEN_ERROR:        /* interfile header cold not be opened */
          fprintf(stderr, "%s: Can't open file\n",hdrName);
          return 1;

      // default: file loaded
  }


                                    /* if scannerModel is still unknown try   */
                                    /* to get it from file name extension now */
  if (scannerModel[0] == '\0') {
    if (!strcmp(ext,"i"))  {
                             /* Check the header: Is it really from the HRRT? */
      if (!interfile_find(&ifh,"!originating system",scannerModel,
        sizeof(scannerModel))){
        fprintf(stderr, "ERROR: Which scanner model?? Can't interpret header format.\n");
        return 1;
      }
      // Allow 328 or HRRT
      if (strcmp(scannerModel,"328") == 0) strcpy(scannerModel,"HRRT");
    }
  }

  printf("scanner model %s \n",scannerModel);

                                         /* handle input and output file name */
  if (!strcmp(scannerModel,"HRRT")) {
             /* assumption: valid file name has the form A_B_C_(...).Z with   */
             /* for HRRT (e.g. REH0003_sp3_frame0_3D.i):                      */
             /* A = study name, X = frame number (_framex), rest description  */
                /* look for a field with 'frame..' indicating a dynamic study */
    pos = 0;                /* number of the field which contains 'frame' */
    for (i=1;i<numFields;i++)
      if (!strncmp(field[i],"frame",5)) framePos = i+1;
              /* construct file name if there is none given in the commandline*/
    if (!strcmp(outFile,"")) {
      strcpy(outFile,field[0]);
      for (i=1;i<numFields;i++)
        if (framePos != i+1) {
          strcat(outFile,"_");strcat(outFile,field[i]);
        }
        strcat(outFile,".v");
    }

                  /* take the first field of the output filename as studyName */
    pos=0;                         /* look for the tail of the filename first */
    for (i=0; i<256; i++) {            /* find position after last '/' or '\' */
      if (!memcmp(&outFile[i],"",1)) break;
      if (outFile[i] == '/' || outFile[i] == '\\') pos=i+1;
    }
 
    k=0;                                              /* get tail of filename */
    for (i=pos; i<256; i++) {
      if (!memcmp(&outFile[i],"",1)) break;
      outFileTail[k] = outFile[i];
      k++;
    }
        
    for (i=0; i<254; i++) {                        /* get first field of tail */
      if (!memcmp(&outFileTail[i],"",1) ||
           !memcmp(&outFileTail[i],"_",1) || k >= 128) break;
      if (!memcmp(&outFileTail[i],".",1) || 
          !memcmp(&outFileTail[i+2],"",1)) break;
      studyName[i] = outFileTail[i];
    }
    sprintf(logMsg,"Study Name is %s (got from output filename)\n",studyName); 
    Logging(logMsg);
  }
                                                           /*** calibration ***/
                  /* get study date only if not specified in the command line */
  if (scanStartDay == 0) {
    if (interfile_find(&ifh,"!study date (dd:mm:yryr)",value,sizeof(value))) {
      value[2] = '\0';  
      scanStartDay = atoi(&value[0]);
      value[5] = '\0';  
      scanStartMonth = atoi(&value[3]);
      value[10] = '\0';  
      scanStartYear = atoi(&value[6]);
    }
  }
                                                           /* scan start time */
  if (interfile_find(&ifh,"!study time (hh:mm:ss)",value, sizeof(value))) {
    value[2] = '\0'; 
    scanStartHour = atoi(&value[0]);
    value[5] = '\0';  
    scanStartMinute = atoi(&value[3]);
    value[8] = '\0'; 
    scanStartSecond  = atoi(&value[6]);
  }

  if (calibDir != NULL)
  {
    if (calibration_load(calibDir) > 0)
    {
      const char *scan_calib;

      sprintf(logMsg, 
        "Find calibration file for dd:mm:yyyy hh:mm:ss =%02d:%02d:%04d %02d:%02d:%02d\n",
        scanStartDay,scanStartMonth,scanStartYear, scanStartHour, 
        scanStartMinute,scanStartSecond);
      Logging(logMsg);
      scan_calib = calibration_find(scanStartYear,scanStartMonth,scanStartDay,
                                scanStartHour, scanStartMinute,scanStartSecond);
      if (scan_calib != NULL)   
      {
        sprintf(logMsg, "Found calibration file %s\n", scan_calib);
        Logging(logMsg);
        sliceSensDim = read_sensitivity(scan_calib, &sliceSensitivity, buffer, 
                      line, keyword, value);
      }
      else Logging("Calibration file not found\n");
    }
  }
                                /* check if input image is already calibrated */
       /* calibration does presently (04/2005) not work with the CPS software */
       /* so the names of the relevant keywords are unknown                   */
      
  if (!interfile_find(&ifh,"calibration file header name used",
    value,sizeof(value)))
  { /* keyword not found = image is not yet calibrated */
    if (calibFactor == 0)
    {         /* if no calibration information available */
      toCalibrate = -1;                                /* don't calibrate */
      strcpy(dataUnits,"");
    } 
    else 
    {
      toCalibrate = 1;
      if (!strcmp(dataUnits,""))                /* if no units are specified */
        strcpy(dataUnits,"kBq/ml"); /* kBq/ml is default for calibrated image*/
      if (!strcmp(dataUnits,"kBq/ml") || !strcmp(dataUnits,"kBq/cc")) 
        calibFactor = calibFactor / 1000;
      if (!strcmp(dataUnits,"MBq/ml") || !strcmp(dataUnits,"MBq/cc")) 
        calibFactor = calibFactor / 1000000;
    }
  }
  else // interfile_find
  {     /* image is already calibrated */
    toCalibrate = -2;
        /* --> read data units from file */
        /* presently (04/2005) the CPS recon software does dot give */
        /* to calibrate images, so the relevant keywords are not known */
  }

                             /* check if input image is attenuation corrected 
                                and if corresponding mu-map exists
                             */
  if (interfile_find(&ifh,"atten file used",value,sizeof(value)))
  {
    pos = strlen(value)-1;
    if (pos>0)
      {
        value[pos] = 'i';
        if (access(value,R_OK)==0) 
        {
          muMask = CreateHRRTMuMask(value);
          if (muMask != NULL) 
            printf ("Use %s mask to find for image extrema\n",
            value);
          else printf ("Error creating mask from %s\n", value);
        }
      }
    else toCalibrate = -3;                  /* value empty -> don't calibrate */
  }
  else toCalibrate = -3;          /* keyword not in header -> don't calibrate */

                                 /* check if input image is scatter corrected */
  if (!interfile_find(&ifh,"scatter file used",value,sizeof(value)))
  {
    toCalibrate = -4;             /* keyword not in header -> don't calibrate */
  }

       /* The byte order of floats and long integers on Intel-processor based */
       /* systems (little endian) and Sun-processor based Solaris systems     */
       /* (big endians) are different. If data are written on one system and  */
       /* read on the other, then the bytes ahve to be swaped while reading   */

#if defined(WIN32)|| defined(_LINUX) 
  is_little = 1;
#else
  is_little=little_endian();   /* Is byte order little endian on this system? */
#endif
  if (is_little) {
    Logging("byte order on this system is little endian \n");
  } else {
    Logging("byte order on this system is big endian\n");
  }
  fflush(stdout);

/******************************************************************************/
/*                 define scanner specific default values                     */
/******************************************************************************/
  if (!strcmp(scannerModel,"HRRT")) {
    main_header.system_type = 328;                        /* that is the HRRT */
    strncpy(main_header.serial_number,"HRRT",
      sizeof(main_header.serial_number)-1);
    main_header.transm_source_type =  3;                      /* point source */
    main_header.distance_scanned =  25.2281f;              /* axial FOV in cm */
    main_header.transaxial_fov = 31.2f;;
    main_header.angular_compression = 3;                           /* unknown */
    main_header.calibration_factor = 1;          /* to be set correctly later */
                                              /* 0=uncalibrated, 1=calibrated */
    image_header.num_r_elements = 256;    /* number of bins in cor. sinogramm */
    image_header.num_angles = 288;  /* number of angles in corrected sinogram */
    image_header.scatter_type = 2;  
  }

/******************************************************************************/
/*                find all images / frames (input files)                      */
/******************************************************************************/

  if (!strcmp(scannerModel,"HRRT")) {        /* frame naming is HRRT specific */
    sprintf(logMsg,"looking for frames for %s_??_framex_??.%s\n",field[0],ext); 
    Logging(logMsg);
      
    if (framePos == 0) {                                  /* static emission */
      Logging("seems to be a static emission (no dynamic study)\n");
      numFrames = 1;
    } else {  
      numFrames = 0;                                /* starting with _frame0_ */
      while (numFrames >= 0) {
        char strframes[10];
        if2e7_itoa(numFrames,strframes);
        strcpy(fname,field[0]);                         /* construct filename */
        for (i=1;i<numFields;i++)
        {
          if (framePos == i+1) {            /* if this field contains 'frame' */
            strcat(fname,"_"); 
            strcat(fname,"frame");
            strcat(fname,strframes);
	              /* filenames can be _frameX_bla.i but also _frameX.tr.i */
                      /* so we also have to look for a '.' as field separator */
                      /* find position of first '.' in this field             */
            for (pos=0; pos<(int)(strlen(field[i])); pos++) {
              if (field[i][pos] == '.') break;
	    }
	                   /* complete fname if we found a '.' in frame field */
            if (pos != (int)(strlen(field[i]))) {
              strcat(fname,".");
	                                    /* add everything after first '.' */
              for (k=pos+1; k<(int)(strlen(field[i])); k++){
                char helpstring[2];
                helpstring[0]=field[i][k]; helpstring[1]='\0';
		strcat(fname,helpstring);
	      }
            }
          }
          else {
            strcat(fname,"_");
            strcat(fname,field[i]);
          }
        }
        strcat(fname,".");
        strcat(fname,ext);
        if (access(fname,R_OK)!=0)  break;
        numFrames++;
      } // end while numFrames
        
      sprintf(logMsg,"%i frames found for study \n",numFrames); 
      Logging(logMsg);
    }//else
  } // if HRRT
  else { // not HRRT
    numFrames = 1;
    sprintf(logMsg,"treated as a static emission %s \n",scannerModel); 
    Logging(logMsg);
  }
                                                 /* determine range of frames */
  if (startFrame != 0 || endFrame !=0) { 
    if (startFrame > numFrames) startFrame = 1;                /* not allowed */
    if (endFrame > numFrames) endFrame = numFrames;            /* not allowed */
    sprintf(logMsg,"using frames %i-%i \n",startFrame,endFrame); 
    Logging(logMsg);
  }
  else {                                                 /* take all frames */
    startFrame = 1;
    endFrame = numFrames;
  }

  if (frameDuration == NULL) {         /* if no framing information available */
    frameDurDim = numFrames + 1;
    frameDuration=(long int*)calloc(frameDurDim, sizeof (long int));
    for (i=0; i<frameDurDim; i++) frameDuration[i] = 0;       /* initialize */
  }

  fflush(stdout);


/*            Extract image bounding box from transmission                    */
  if (trimFlag && muMask != NULL)
    getBoundaries(muMask, &startPlane, &endPlane, &dimxOutfile, &xCentre, &yCentre);

/******************************************************************************/
/*                          fill ecat7 main header                            */
/******************************************************************************/

  Logging("\nmain header\n");
	    /* if it is a HRRT interfile get values from the interfile header */
  if (main_header.system_type == 328) {
    int image_ok=0;
                                     /* check, if the file is really an image */
    if (interfile_find(&ifh,"data format",value,sizeof(value)))
    { 
      if (strcmp(value,"image") ==0) image_ok=1;
    }
    if (!image_ok) {
      fprintf(stderr, "ERROR: no header entry 'data format := image' found.\n");
      fprintf(stderr, "       %s is no valid HRRT interfile image header.\n",
        ifh.filename);
        return 1;
    }

   interfile_find(&ifh,"Patient name",main_header.patient_name, 
     sizeof(main_header.patient_name));
   interfile_find(&ifh,"Patient ID",main_header.patient_id,
     sizeof(main_header.patient_id));
   if (interfile_find(&ifh,"Patient DOB",value,sizeof(value)))
     {         /* expected format: m[m]/d[d]/yyyy  syntax is not checked! */
       char *p1, *p2; // resp. dd and yyyy start 
       if ((p1 = strchr(value,'/')) != NULL)
        {
          *p1++ = '\0';
          if ((p2 = strchr(p1,'/')) != NULL)
            {
              *p2++ = '\0';
              birthMonth = atoi(value);
              birthDay = atoi(p1);
              birthYear = atoi(p2);
            }
       }
     }
    if (interfile_find(&ifh,"Patient sex",value,sizeof(value)))
      {
        main_header.patient_sex[0] = toupper(value[0]); /*'M' Male,'F' Female */
        if ( main_header.patient_sex[0] != 'M' &&  main_header.patient_sex[0] != 'F' )
          main_header.patient_sex[0] = 'U';      // set to Unknown if not valid
      }

                                                          /* number of planes */
    if (interfile_find(&ifh,"matrix size [3]",value,sizeof(value)))
      {
        numPlanes = atoi(value);
      }
    // else initialized to 0
                                                          /* plane separation */
    if (interfile_find(&ifh,"scaling factor (mm/pixel) [3]",value,sizeof(value)))
      {
        planeSeparation = (float)(atof(value) / 10);  // convert mm to cm
      }
    // else initialized to 0
                                                                  /* bin size */
    if (interfile_find(&ifh,"scaling factor (mm/pixel) [1]",value,sizeof(value)))
      {
        binSize = (float)(atof(value) / 10);  // convert mm to cm
      }
    // else initialized to 0

    /* get isotope, halflife and branching factor only from header
       if not specified in command line
    */
    if (isotope_info==NULL)
      {
        if (interfile_find(&ifh,"Dose type",value,sizeof(value))) 
          {
            strncpy(main_header.isotope_code, value,
              sizeof(main_header.isotope_code)-1);
          }

        if (interfile_find(&ifh,"isotope halflife",value,sizeof(value)))
          main_header.isotope_halflife = (float)atof(value); 
        if (interfile_find(&ifh,"branching factor",value,sizeof(value)))
          main_header.branching_fraction = (float)atof(value); 
       }
    else
      {
        strncpy(main_header.isotope_code, isotope_info->name,
              sizeof(main_header.isotope_code)-1);
        main_header.isotope_halflife = isotope_info->halflife;
        main_header.branching_fraction = isotope_info->branch_ratio;
      }
                                                             /* injected dose */
    if (interfile_find(&ifh,"Dosage Strength",value,sizeof(value)))
      {
        Dosage = (float)atof(value);
      }

                                                          /* acquisition type */
    if (interfile_find(&ifh,"!PET data type",value,sizeof(value)))
      {                 /* 03/2005: HRRT interfile does not mark transmissons */
        if (strcmp(value,"emission") == 0) 
          {
            if (numFrames == 1) acquisitionType = StaticEmission;
            else acquisitionType = DynamicEmission;
          }
        else if (strcmp(value,"transmission") ==0 )
          {
            acquisitionType = TransmissionScan;
            correctionMode = 0;
          }
      }

                                                             /* energy window */
    if (interfile_find(&ifh,"energy window lower level[1]",value,
      sizeof(value)))
      {
        main_header.lwr_true_thres = atoi(value); 
      }

    if (interfile_find(&ifh,"energy window upper level[1]",value,
      sizeof(value)))
      {
        main_header.upr_true_thres = atoi(value); 
      }

                                                 /* coincidence sampling mode */
    if (interfile_find(&ifh,"sinogram data type",value,sizeof(value)))
      {
        if(!strcmp(value,"prompt")) 
          main_header.coin_samp_mode = 2;  /*PromptsDelayedMultiples*/
      }

    fflush(stdout);

    Logging("filling ecat7 main header \n");

    memset(&zeit,0,sizeof(zeit));
    zeit.tm_sec=scanStartSecond;
    zeit.tm_min=scanStartMinute; 
    zeit.tm_hour=scanStartHour;
    zeit.tm_mday=scanStartDay;
    zeit.tm_mon=scanStartMonth-1;
    zeit.tm_year=scanStartYear-1900;
    zeit.tm_isdst=-1;                      /* allows for daylight saving time */
    main_header.scan_start_time = (unsigned int)mktime(&zeit);

    /* (fvv) calculate age */
    /* Work around faulty input for birthyear, e.g. in CTI tools: 12:00:00 AM */
    if (birthYear != 0)
    {
      age = (float) (scanStartYear - birthYear);

      if (birthMonth >= scanStartMonth)
      {
  	    if (birthDay > scanStartDay)
  	    {
   	      age = age - 1.0f;
  	    }
      }
      /* (fvv) hack to work around the mktime under windows problem (by Hugo de Jong) */
      if (birthYear < 1970)
      {
        if (birthYear < 1902)
        {
	  // My change ahc 7/16/13 for phantoms etc given birth year 1900.
          // return 0;
	  birthYear = 1970;
        }
        else if (birthYear < 1952)
        {
          nOffset1 = -1325419200;
          nOffset2 = -1325419200;
          birthYear += 84;
          // Apparently dates before 1942 were never DST
          if (birthYear < 1942)
          {
            birthDay = 0;
          }
        }
        else
        {
          nOffset1 = -883612800;
          birthYear += 28;
        }
      } // end of hack
    }
    else
    {
  	  birthYear  = 0;
  	  birthMonth = 0;
  	  birthDay   = 0;
  	  age        = 0;
    }
    if (anonymousMode == 0)
      {
        strncpy(main_header.study_name,studyName, sizeof(main_header.study_name)-1);
        strncpy(main_header.original_file_name,inFileTail, 
          sizeof(main_header.original_file_name)-1);
        memset(&zeit,0,sizeof(zeit));
        zeit.tm_mday=birthDay;
        zeit.tm_mon=birthMonth-1;
        zeit.tm_year=birthYear-1900;
		    zeit.tm_isdst = -1; // don't use daylight saving
        main_header.patient_birth_date = (int)(mktime(&zeit)+nOffset1+nOffset2);
        main_header.patient_age = age;
      } 
    else 
      {
        strncpy(main_header.study_name,anonymousName, sizeof(main_header.study_name)-1);
        main_header.patient_birth_date = 0;
        strncpy(main_header.patient_id, "anonyymi", sizeof(main_header.patient_id)-1);
        strncpy(main_header.patient_name, anonymousName, sizeof(main_header.patient_name)-1);
        strncpy(main_header.original_file_name,anonymousName, sizeof(main_header.original_file_name)-1);
      }

    main_header.patient_dexterity[0] = 'U';                     /* always unknown */
    main_header.acquisition_type = acquisitionType;         /* dynamic emission */
    main_header.patient_orientation = patientOrientation;
    if (startPlane != 0 || endPlane !=0)
      {                 /* from command line */
        if (startPlane > numPlanes) startPlane = 0;               /* not allowed */
        if (endPlane > numPlanes) endPlane = numPlanes;           /* not allowed */
        main_header.num_planes = endPlane - startPlane +1;
      } 
    else
      {                                           /* from interfile header */
        main_header.num_planes = numPlanes;
      }
    main_header.num_frames = endFrame - startFrame +1;
    main_header.num_gates = 1;
    main_header.plane_separation = planeSeparation;
    main_header.bin_size= binSize;
    main_header.dosage = Dosage;
    strcpy(main_header.data_units,"counts");       /* to be set correctly later */
    main_header.septa_state = 1;                                     /* 3D mode */
    if (menuFlag) menu_input(&main_header);
  }

/******************************************************************************/
/*                           do for each frame                                */
/******************************************************************************/
  for (frame=startFrame;frame<=endFrame;frame++) 
  {
    if (frame >= frameDurDim) 
      {
        fprintf(stderr, "ERROR: frame %i of %i frames (check -f switch)\n",
          frame,frameDurDim-1);
        return 1;
      }
    sprintf(logMsg,"\n%i. frame\n",frame);
    Logging(logMsg);
    image_header.processing_code = 0;      /* reset list of corrections done */
      
    fflush(stdout);

                                      /* construct filename for HRRT data ... */
    if (main_header.system_type == 328) 
    {
                                    /* interfile image filename of this frame */
      char strframe[10];
      if2e7_itoa(frame-1,strframe);         /*starting from frame0 not frame1 */
      strcpy(fname,field[0]);
      for(i=1;i<numFields;i++) {
        if (framePos == i+1) {            /* if this field contains 'frame' */
          strcat(fname,"_"); strcat(fname,"frame"); strcat(fname,strframe);
	              /* filenames can be _frameX_bla.i but also _frameX.tr.i */
                      /* so we also have to look for a '.' as field separator */
                      /* find position of first '.' in this field             */
          for (pos=0; pos<(int)(strlen(field[i])); pos++) {
            if (field[i][pos] == '.') break;
	  }
	                   /* complete fname if we found a '.' in frame field */
          if (pos != (int)(strlen(field[i]))) {
            strcat(fname,".");
	                                    /* add everything after first '.' */
            for (k=pos+1; k<(int)(strlen(field[i])); k++){
              char helpstring[2];
              helpstring[0]=field[i][k]; helpstring[1]='\0';
	      strcat(fname,helpstring);
	    }
          }
        } else {
          strcat(fname,"_"); strcat(fname,field[i]);
        }
      }
      strcat(fname,"."); strcat(fname,ext);
    
      pos=0;  /* get the tail of the input file name for shorter log messages */
      for (i=0; i<256; i++) fnameTail[i] = '\0';
                                       /* find position after last '/' or '\' */
      for (i=0; i<(int)(strlen(fname)); i++) {
        if (fname[i] == '/' || fname[i] == '\\') pos=i+1;
      }
      k=0;                                            /* get tail of filename */
      for (i=pos; i<(int)(strlen(fname)); i++) {
        fnameTail[k] = fname[i];
        k++;
      }
      strcpy(hdrName,fname);                    /* construct header file name */
      strcat(hdrName,".hdr"); 
    }

    /*************************************************************************/
    /*                       read interfile header                           */
    /*************************************************************************/
    if (main_header.system_type == 328) 
    {                                                   /*** HRRT interfile ***/
      char strframe[10];
          
      image_header.processing_code += 256;                  /* arc corrected */
      if2e7_itoa(frame-1,strframe);       /* starting with frame0 not frame1 */
          
                                      /* now look for the relevant key words */
                                      /* number format (float,integer...) */
                                  /*Clean table and load header filename */
        interfile_clear(&ifh);
        switch(interfile_load(hdrName,&ifh))
        {
          case IFH_FILE_INVALID:            /* Not starting with '!INTERFILE' */
               fprintf(stderr, "%s: is not a valid interfile header\n",hdrName);
               return 1;
             
          case IFH_FILE_OPEN_ERROR:    /* interfile header cold not be opened */
               fprintf(stderr, "%s: Can't open file\n",hdrName);
               return 1;

               // default: file loaded
        }
          
        value[0]='\0';
        interfile_find(&ifh,"number format",value,sizeof(value));
        if (strcmp(value,"float") != 0)
        {
          fprintf(stderr,
            "ERROR: There is no 'number format := float' in the header. \n");
          fprintf(stderr,
            "       Don't know what to do !?! Float format is expected.\n");
          return 1;
        }
                                                        /* matrix dimensions */
        if (interfile_find(&ifh,"matrix size [1]",value,sizeof(value)))
          dimx = atoi(value);
        if (interfile_find(&ifh,"matrix size [2]",value,sizeof(value)))
          dimy = atoi(value);
        if (interfile_find(&ifh,"matrix size [3]",value,sizeof(value)))
          dimz = atoi(value);
                                                           /* scaling factors */
        if (interfile_find(&ifh,"scaling factor (mm/pixel) [1]",
          value,sizeof(value)))
          image_header.x_pixel_size = (float)(atof(value) / 10);
        if (interfile_find(&ifh,"scaling factor (mm/pixel) [2]",
          value,sizeof(value))) 
          image_header.y_pixel_size = (float)(atof(value) / 10);
        if (interfile_find(&ifh,"scaling factor (mm/pixel) [3]",
          value,sizeof(value)))
          image_header.z_pixel_size = (float)(atof(value) / 10);
                                  /* framing information (start and duration) */
        if (interfile_find(&ifh,"image relative start time",value,
          sizeof(value)))
          frameStart = atoi(value);
        if (interfile_find(&ifh,"image duration",value,sizeof(value))) {
          frameDuration[frame] = atoi(value); 
        }
                                                   /* decay correction factor */
        if (interfile_find(&ifh,"decay correction factor",value,
          sizeof(value))) 
          decayFrameStart = atof(value); 
        if (interfile_find(&ifh,"decay correction factor2",value,
          sizeof(value))) 
          decayInFrame = atof(value);
                                                /* deadtime correction factor */
        if (interfile_find(&ifh,"average singles per block",value,
          sizeof(value))) {
          deadtimeCorFactor = exp(1*deadtimeExponent*atof(value));
          //      printf("\n\n\n\n***** deadtimeCorFactor=%f", deadtimeCorFactor);
          //      fflush(stdout);
          if (frameDuration[frame]>0) image_header.singles_rate = (float)atof(value);
        }
                                                /* Data rates */
				if (frameDuration[frame]>0) {
					if (interfile_find(&ifh,"Total Prompts", value, sizeof(value)))
						image_header.prompt_rate = (float)(atof(value)/frameDuration[frame]);
					
					if (interfile_find(&ifh,"Total Randoms", value, sizeof(value)))
						image_header.random_rate = (float)(atof(value)/frameDuration[frame]);
        }
                                                /* Corrections */
        if (interfile_find(&ifh,"scatter fraction", value, sizeof(value)))
          image_header.scatter_fraction = (float)atof(value);

                                                  /* reconstruction algorithm */
        strcpy(line,"");
        if (interfile_find(&ifh,"reconstruction method",value,
          sizeof(value)))
        {
          strcpy(line,value);
          if (!strcmp(value,"FORE/OSEM")) 
          {
            reconType = 6;                              /* "Fourier rebinning */
            image_header.processing_code += 2048; 
          }
        }
        if (interfile_find(&ifh,"number of iterations",value,
          sizeof(value)))
        {
          strcat(line," i"); 
          strcat(line,value);
        }
          
        if (interfile_find(&ifh,"number of subsets",value,
          sizeof(value)))
        {
          strcat(line," s"); 
          strcat(line,value);
        }
        strncpy(image_header.annotation,line,40);
          
        strcpy(line,"");
                                                    /* check for corrections */
        if (interfile_find(&ifh,"norm file used",value,sizeof(value)))
          image_header.processing_code += 1;                    /* normalised */

        if (interfile_find(&ifh,"atten file used",value,sizeof(value)))
          image_header.processing_code += 2;        /* attenuation correction */

        if (interfile_find(&ifh,"scatter file used",value,sizeof(value)))
          image_header.processing_code += 128;          /* scatter correction */
    }   // end HRRT (systemType 328)

    fflush(stdout);

  /****************************************************************************/
  /*                       fill ecat7 subheaders                              */
  /****************************************************************************/

    Logging("  filling subheader \n");
    image_header.data_type = 6;                                  /* Sun short */
    image_header.num_dimensions = 3;
    if (dimxOutfile != 0) {                              /* from command line */
      if (dimxOutfile > dimx) dimxOutfile = dimx;              /* not allowed */
      image_header.x_dimension = dimxOutfile;
      image_header.y_dimension = dimxOutfile;
      image_header.x_offset = (xCentre-dimx/2)*image_header.x_pixel_size;
      image_header.y_offset = (yCentre-dimy/2)*image_header.y_pixel_size;
    } else {                                         /* from interfile header */
      image_header.x_dimension = dimx;
      image_header.y_dimension = dimy;
    }
    if (startPlane != 0 || endPlane !=0) {               /* from command line */
      if (startPlane > dimz) startPlane = 0;                   /* not allowed */
      if (endPlane > dimz) endPlane = dimz;                    /* not allowed */
      image_header.z_dimension = endPlane - startPlane +1;
      image_header.z_offset = ((startPlane+endPlane)/2-dimz/2)*planeSeparation;
    } else {                                         /* from interfile header */
      image_header.z_dimension = dimz;
      image_header.z_offset = 0;
    }
    image_header.recon_zoom = reconZoom;
   /* scale factor, image min and max are calculated by ecat7WriteImageMatrix */
    if((acquisitionType != TransmissionScan) && frameDuration[frame] == 0) {
      fprintf(stderr, "ERROR: information about the frame length is needed but not available.\n");
      fprintf(stderr, "       Don't know what to do.\n");
      return 1;
    } 
    image_header.frame_duration = frameDuration[frame] * 1000;       /* im ms */
    image_header.frame_start_time = frameStart * 1000;               /* in ms */
                        /* decay correction factor has to be calculated first */
    image_header.decay_corr_fctr = (float)(decayInFrame * decayFrameStart);
    image_header.recon_type = reconType;


  /****************************************************************************/
  /*                    read the image matrix from file                       */
  /****************************************************************************/
                         /* don't know what to do, if input file is not float */
    matrix_float = (float *) malloc(dimx*dimy*dimz*sizeof(float));

                  // gaussian smooth image if requested
    if (gsmooth_width>0) {
      char *pext;
      sprintf(command, "gsmooth %s %d", fname, gsmooth_width);
      sprintf(logMsg,"gsmooth width (default 0): %i \n",gsmooth_width);
      Logging(logMsg);
      sprintf(logMsg,"command: %s",command);
      Logging(logMsg);
      if ((pext = strrchr(fname,'.')) == NULL) {
        fprintf(stderr, "ERROR locating file %s extension\n",fname);
        return 1;
      }
      system(command);
      sprintf(pext,"_%dmm.%s", gsmooth_width, ext);  // update input file name
      if ((pext = strrchr(outFile,'.')) != NULL) {
        sprintf(pext,"_%dmm.v", gsmooth_width);
      }
      else {
        fprintf(stderr, "ERROR locating file %s extension\n",outFile);
        return 1;
      }
    }
                                                     /* ... and read the file */
    sprintf(logMsg,"  reading %s\n",fnameTail);
    Logging(logMsg);
    if ((inputImage = fopen(fname,"rb"))==NULL) {
      fprintf(stderr, "ERROR opening image file %s \n",fname);
      free(matrix_float);
      return 1;
    }
    fread(&matrix_float[0],sizeof (float),dimx*dimy*dimz,inputImage);
    fclose(inputImage);
      /* float byte order in HRRT interfile images is little endian. If       */
      /* conversion is done on a big endian sytem the bytes have to be swaped */
    if (!strcmp(scannerModel,"HRRT") && is_little == 0) {
#if !defined(WIN32) && !defined(_LINUX)
      Logging("  swapping bytes\n");
      for (j=0; j<dimx*dimy*dimz; j++) 
        swawbip(&matrix_float[j], sizeof(float));
#endif
      }

  /****************************************************************************/
  /*               correction/manipulation of matrix data                     */
  /****************************************************************************/

                             /* don't make any corrections if the data are    */
                             /* already corrected or if the '-x' switch is on */
    if (toCalibrate == -2) {
      printf("  matrix not calibrated (interfile image is already calibrated)\n");
      main_header.calibration_factor = calibFactor;

       /* 0=uncalibrated, 1=calibrated in Bq/ml, 2= calibrated in 'data_units'*/
      if (!strcmp(dataUnits,"Bq/ml") || !strcmp(dataUnits,"Bq/cc")) {
         main_header.calibration_units = 1;
      } else {
         main_header.calibration_units = 2;
      }
      main_header.calibration_units_label = 1;
      strcpy(main_header.data_units,dataUnits);
    } else if (correctionMode == 0){
      if (acquisitionType != TransmissionScan)
        printf("  matrix not calibrated (as requested in command line)\n");
      main_header.calibration_factor = 1;
       /* 0=uncalibrated, 1=calibrated in Bq/ml, 2= calibrated in 'data_units'*/
      main_header.calibration_units = 0; 
      main_header.calibration_units_label = 0;
      strcpy(main_header.data_units,"counts");  /* write units to main header */
      if (acquisitionType == TransmissionScan) {
        main_header.calibration_units = 2;
        strcpy(main_header.data_units,"1/cm");
      } else if (!strcmp(scannerModel,"HRRT")) 
        strcpy(main_header.data_units,"HRRT counts");
    } else {
        /*** normalize for different acquisition times (counts -> counts/s) ***/
      sprintf(logMsg,"  normalising for acquisition duration: %li s\n",frameDuration[frame]);
      Logging(logMsg);
      for (j=0;j<dimx*dimy*dimz;j++) 
        matrix_float[j] = matrix_float[j] / frameDuration[frame];
      strcpy(main_header.data_units,"counts/s");/* write units to main header */
      if (!strcmp(scannerModel,"HRRT")) 
        strcpy(main_header.data_units,"HRRT counts/s");

                           /*** decay correction in frame and to scan start ***/
      Logging("  applying decay correction: ");
      if(main_header.isotope_halflife == 0) {
        fprintf(stderr,"\nERROR: Which isotope?? Which halflife time??"
                      "Can't do decay correction.\n");
        free(matrix_float);
        return 1;
      }
                                                                  /* in frame */
      frac = frameDuration[frame]/main_header.isotope_halflife*log(2);
      decayInFrame = frac/(1-exp(-frac));
                                                            /* to frame start */
      decayFrameStart = exp(frameStart/main_header.isotope_halflife*log(2));
      sprintf(logMsg,"%f (in frame) and %f (frame start)\n",decayInFrame,decayFrameStart);
      Logging(logMsg);
                                      /* apply decay correction to scan start */
      for (j=0;j<dimx*dimy*dimz;j++) 
        matrix_float[j] = (float)(matrix_float[j] * decayInFrame * decayFrameStart);
                                /* write decay correction factor to subheader */
      image_header.decay_corr_fctr = (float)(decayInFrame * decayFrameStart);
      image_header.processing_code += 512;                     /* decay corrected */

      fflush(stdout);

   /* rest of calibration only if scatter and attenuation correction are done */
      if (toCalibrate != 1) { 
        main_header.calibration_factor = 1.0f;
        main_header.calibration_units = 0;    /* 0=uncalibrated, 1=calibrated */
        main_header.calibration_units_label = 0;
        Logging("  matrix not calibrated\n");
      } else {
                                         /*** slice sensitivity calibration ***/
        Logging("  applying slice sensitivity correction\n");
        if (sliceSensitivity == NULL) {
          fprintf(stderr, "WARNING: no slice sensitivity values available\n");
        }
        else {
          for (z=0;z<dimz;z++) {
            if (z > sliceSensDim) {
              fprintf(stderr, "WARNING: slice sensitivity values available only"
                "for %i planes out of %i!\n",sliceSensDim,dimz);
              Logging("CHECK IT !!!\n");
              sprintf(logMsg,"  slice sensitivity correction stoped after %i slices\n",sliceSensDim);
              Logging(logMsg);
              break;
            }
            for (j=0;j<dimx*dimy;j++) 
              matrix_float[z*dimx*dimy+j] *= sliceSensitivity[z] ;
          }
        }
      }
                                        /*** correct for branching fraction ***/

    /* Only a part of the positrons emitted by a specific isotop are          */
    /* annihilating, i.e. only part of these decays are detected in PET.      */
    /* This part is the branching factor. It is different for different       */
    /* isotopes. The calibration measurement is not necessarily done with the */
    /* same isotope like the actual measurment. So it is necessary to correct */
    /* for the branching fraction. Values in matrix have to be divided by the */
    /* branching fraction. Make sure that the branching fraction is taken into*/
    /* consideration while calculating the calibration factor.                */

        if (main_header.branching_fraction == 0) {
          fprintf(stderr, "error: branching fraction is unknown but needed for calibration.\n");
          fprintf(stderr, "       Try to use the '-i isotope' switch as this sets the"
                                  "branching factor.\n");
          free(matrix_float);
          if (ECAT7file != NULL) matrix_close(ECAT7file);
          return 1;
        }
        sprintf(logMsg,"  dividing matrix by branching fraction %g\n",main_header.branching_fraction);
        Logging(logMsg);
        for (j=0;j<dimx*dimy*dimz;j++) 
          matrix_float[j] = matrix_float[j] / main_header.branching_fraction;

                                                  /*** dead time correction ***/
        /* deadtime correction factor is taken from the HRRT-interfile-header */
       sprintf(logMsg,"  applying deadtime correction: %f \n",deadtimeCorFactor);
       Logging(logMsg);
        for (j=0;j<dimx*dimy*dimz;j++) 
          matrix_float[j] = (float)(matrix_float[j] * deadtimeCorFactor);

                  /*** multiply matrix with calibration factor if requested ***/

    /* Normally matrix values don't have to be multiplied with the calibration*/
    /* factor. In the ECAT7 format it is enough to write the calibration facor*/
    /* in the main header as the multiplication is done by the image tool.    */
    /* But sometimes it might be necessary.                                   */
     
        if (multPixWithCalibFactor == 1) {
          sprintf(logMsg,"  multiplying matrix with calibration factor %g\n",calibFactor);
          Logging(logMsg);
          for (j=0;j<dimx*dimy*dimz;j++) 
            matrix_float[j] = matrix_float[j] * calibFactor;
        }
     /* (fvv) Option to define a maximum pixel value. During ANW 3DOSEM, we   */
     /* obtained some image artifacts like overrepresentation of some pixel   */
     /* values. This caused the image to lose information during ecat         */
     /* conversion (truncation of value to 0 during conversion to integers).  */
     /* By lowering these very high values to the defined maximum pixel value */
     /* (recommended 0.015 cnts per s), we managed to retain the information  */
     /* in the scan.                                                          */
        if (pixelMax > 0.0f) 
        {                                                                 
          sprintf(logMsg,"  applying maximum pixel correction %g\n", pixelMax);
          Logging(logMsg);
          for (j = 0; j < dimx*dimy*dimz; j++)
          {
            if (matrix_float[j] > pixelMax)
            {	
        	  matrix_float[j] = pixelMax;
            }
          }
        }
       	                          /* update main header for calibrated images */
        main_header.calibration_factor = calibFactor;
       /* 0=uncalibrated, 1=calibrated in Bq/ml, 2= calibrated in 'data_units'*/
        if (toCalibrate == 1) { 
          if (!strcmp(dataUnits,"Bq/ml") || !strcmp(dataUnits,"Bq/cc")) {
             main_header.calibration_units = 1;
          } else {
             main_header.calibration_units = 2;
          }
          main_header.calibration_units_label = 1;
          strcpy(main_header.data_units,dataUnits);
	}
    }

                       /*** reduce matrix size of output image, if required ***/
                 /* only specified range of planes is taken                   */
                 /* only the centre part of each plane is taken (dimxOutfile) */
    if (startPlane != 0 || endPlane != 0 || dimxOutfile != 0) {
      if (startPlane == 0 && endPlane == 0) endPlane = dimz -1;
      if (dimxOutfile == 0) {
        dimxOutfile = dimx;
        xCentre = dimx/2;
        yCentre = dimy/2;
      }
      sprintf(logMsg,"  reducing image size to %i planes (%i-%i), %ix%i, centre pixel (%i/%i)\n",endPlane-startPlane+1,startPlane,endPlane,dimxOutfile,dimxOutfile,xCentre,yCentre);
      Logging(logMsg);
      if (xCentre-dimxOutfile/2 < 0 || xCentre+dimxOutfile/2 > dimx ||\
          yCentre-dimxOutfile/2 < 0 || yCentre+dimxOutfile/2 > dimy) {
        fprintf(stderr, "error: centre pixel coordiantes (%i/%i) out of range for a %ix%i matrix\n",xCentre,yCentre,dimxOutfile,dimxOutfile);
        fprintf(stderr, "       from a %ix%i image\n\n",dimx,dimy);
        free(matrix_float);
        if (ECAT7file != NULL) matrix_close(ECAT7file);
        return 1;
      }
                              /* allocate memory for matrix with reduced size */
      matrix_float_reduced = (float *) malloc(dimxOutfile*dimxOutfile*(endPlane-startPlane+1)*sizeof(float));
      j=0;
      for (z=0;z<dimz;z++) {
        if (z < startPlane || z > endPlane) continue;
        for (y=0;y<dimy;y++) {
          if (dimy != dimxOutfile &&
              (y<=floor(yCentre-(float)dimxOutfile/2) || 
               y>floor(yCentre+(float)dimxOutfile/2))
              ) 
            continue;
          for (x=0;x<dimx;x++) {
            if (dimx != dimxOutfile &&
                (x<=floor(xCentre-(float)dimxOutfile/2) ||
                 x>floor(xCentre+(float)dimxOutfile/2))
                )
              continue;
            if (j > dimxOutfile*dimxOutfile*(endPlane-startPlane+1)) {
              fprintf(stderr, "error: seems to be a programming error while reducing matrix size.\n j = %li x=%i, y=%i, z=%i\n",j,x,y,z);
              free(matrix_float);
              free(matrix_float_reduced);
              if (ECAT7file != NULL) matrix_close(ECAT7file);
              return 1;
            }
            matrix_float_reduced[j] = matrix_float[x+y*dimx+z*dimx*dimy];
            j ++;
          }
        }
      }
    }

  /****************************************************************************/
  /*     if frame == startFrame write mainheader and matrix to file           */
  /****************************************************************************/

    if (frame == startFrame) {  
      sprintf(logMsg,"-> writing main header to %s\n",outFileTail);
      Logging(logMsg);
                                                 /* write main header to file */
      ECAT7file=matrix_create(outFile, MAT_CREATE_NEW_FILE, &main_header);
      if(ECAT7file == NULL) {
        fprintf(stderr,"ERROR: cannot write main header %s.\n", outFile);
        return 1;
      }
    }

    fflush(stdout);

  /****************************************************************************/
  /*                 write subheader and matrix to file                       */
  /****************************************************************************/


    sprintf(logMsg,"  -> writing subheader and matrix to %s\n",outFileTail);
    Logging(logMsg);
                                                      /* Create new matrix id */

    matrixId=mat_numcod(frame-startFrame+1, 1, 1, 0, 0);
    //printf("\n\n\n matrixId: %d", matrixId); fflush(stdout);
    /* write subheader and matrix to file */
                     /* data are scaled to short int by ecat7WriteImageMatrix */
    if (matrix_float_reduced != NULL) {
      ret=ecat7WriteImageMatrix(ECAT7file,matrixId,&image_header,matrix_float_reduced, NULL);
      free(matrix_float_reduced);
    } else {
      ret=ecat7WriteImageMatrix(ECAT7file,matrixId,&image_header,matrix_float, muMask);
    }
    free(matrix_float);
    fflush(stdout);

  /****************************************************************************/
  /*                          goto next frame                                 */
  /****************************************************************************/

    frameStart = frameStart + frameDuration[frame];
  }

                                           /* Close and duplicate output file*/
  sprintf(logMsg,"\nclose file %s\n",outFileTail);
  Logging(logMsg);
  if (ECAT7file != NULL) {
    strcpy(inFile, ECAT7file->fname);  // make a copy before freeing memory
    matrix_close(ECAT7file);
    if (duplicateDir != NULL) {
      sprintf(logMsg,"\ncopy file %s to %s\n",inFile, duplicateDir);
      Logging(logMsg);
      copyFile(inFile,duplicateDir, &main_header);
    }
  }
  return 0;
}

/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/*   function to split the fields of a line read from the interfile header    */
/*                          format 'keyword := value'                         */
/******************************************************************************/

void split_line(char line[516], char keyword[256], char value[256])
{
  int       i, pos;
                               /* initialise the variables keyword and value */
  for (i=0;i<256;i++) {
    keyword[i] = '\0';
    value[i] = '\0';
  }
                                 /* find position of the field seperator ':=' */
  for (pos=1; pos<516; pos++)
    if (line[pos] == '=' && line[pos-1] == ':') break; 
                                     /* now get the fist and the second field */
  for (i=0;i<pos-2 && i<256;i++) keyword[i] = line[i];
  for (i=pos+2;i<256+pos+2;i++) {
    if (!memcmp(&line[i],"\0",1) || !memcmp(&line[i],"\r",1) || !memcmp(&line[i]
,"\n",1)) 

      break;         /* stop at the end of "line" */
    value[i-pos-2] = line[i];
  }
}


/******************************************************************************/
/*          Funktion integer to ascii (String), W. Alex, 02.11.88             */
/******************************************************************************/

void if2e7_itoa(int n, char *s)
{
  int sign;
  char x, *p;

  p = s;

  /* Ermittlung des Vorzeichens, Betrag */
  if ((sign = n) < 0) n = -n;
  /* Stellenweise Umwandlung in Zeichen, von rechts */
  do {
      *p++ = n % 10 + '0';  /* '0' ASCII-Nr. 48 */

  } while ((n = n / 10) > 0);

  /* Anfuegen des Vorzeichens */
  if (sign < 0) *p++ = '-';
  *p-- = '\0';                  /* Stringabschluss */

  while (p > s) {
      x = *p;
      *p-- = *s;
      *s++ = x;
  }
}


/******************************************************************************/
/*                        print_build()                                       */
/******************************************************************************/
void print_build()
{
  printf(" %s %s %s\n",PROG_NAME,PROG_VERSION,COPYRIGHT);
  printf("\n Build %s %s\n\n",__DATE__,__TIME__);
}

/*****************************************************************************/
