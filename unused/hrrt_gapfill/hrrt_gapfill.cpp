/*
 * hrrt_gapfill 1.0.0 (c) 2006-2009 by VU University Medical Center
 * originally a part of program: 3drp 0.1.5 (c) 2006-2008 by VU University Medical Center
 *
 * Programmers: Floris van Velden, f.vvelden@vumc.nl
 *
 * Known issues:
 * - Needs a lot of iterations for confosp on very homogeneous objects
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <cstdlib>
#include <windows.h>
#include <ctime>
#include "gapfill.h"
#include "confosthread.h"
#include "header.h"
using namespace std;

/* Program name */
#define PROG_NAME "hrrt_gapfill"

/* Program version */
#define PROG_VERSION "1.0.0"

/* Copyright */
#define COPYRIGHT "(c) 2006-2009 by VU university medical center"

/* Defined functions in main */
void print_usage(void);
void print_build(void);
int print_systeminfo(SYSTEM_INFO info, MEMORYSTATUS mstat);
int checksinogram (char* filename, int sinosize);
int checkfileextension (char* filename, char extension);
int openandwritedata(char* outfile, float* data, int size, int offset);
int openandreaddata(char* infile, float* data, int size, int offset);
int openandreaddata(char* infile, short int* data, int size, int offset);

static char insino[256],
       inatten[256],
       innorm[256],
       inscatter[256],
       inrandoms[256],
       inheader[256],
       outheader[256],
       outsino[256];

int main(int argc, char *argv[])
{
  int i=0, seg=0;
  int length,
      niter,
      extention,
      gapfill,
      rd,
      span,
      nelems,
      nangles,
      nplanes,
      ntotal,
      nrings,
      nsegs,
      nthreads,
      interpolarea,
  	  offset,
      sinosize,
      segsize,
      *segplanes,
      nsegsmax;
  short int *intsino;
  bool verboseMode,
       convertprompts,
       convertrandoms,
       hasatten,
       hasscatter;
  char *str,
       curtime[10];
  float radius,
        *randoms,
        *sinogram,
        *atten,
        *norm,
        *scatter;
  time_t now,
         start;
  struct tm* timeinfo;
  fstream filestream;
  HeaderInfo header;
  SYSTEM_INFO systeminfo;
  MEMORYSTATUS mstat;

/***********************************************************************
 * Set values for some defined variables
 ***********************************************************************/

  // Set variables
  hasatten = false;
  hasscatter = false;
  convertprompts = false;
  convertrandoms = false;
  extention = 1;
  gapfill = 2;
  interpolarea = 4;
  nelems = 256;
  nangles = 288;
  niter = 15;
  nplanes = 207;
  nrings = 104;
  nthreads = 1;
  offset = 0;
  radius = 234.5;
  rd = 67;
  segsize = 0;
  span = 9;
  verboseMode = true;

  for (i = 0; i < 256; i++)
  {
    insino[i] = '\0';
    inscatter[i] = '\0';
    inatten[i] = '\0';
    innorm[i] = '\0';
    inrandoms[i] = '\0';
    outsino[i] = '\0';
    inheader[i] = '\0';
    outheader[i] = '\0';
  }

/***********************************************************************
 * Handle input options
 ***********************************************************************/

  cout << endl << "*** Notification ***" << endl << endl;
  cout << "This program is provided without any warranty. By running this software, you" << endl;
  cout << "agree that this software is provided \'as is\' and is meant for research" << endl;
  cout << "purposes only. Do not use this program for clinical evaluation of patient" << endl;
  cout << "studies. The programmers cannot be held responsible for any bugs or mistakes" << endl;
  cout << "in this program. Do not distribute this program without prior permission by" << endl;
  cout << "the developers of this software. For more details about this software," << endl;
  cout << "please see Van Velden et al. 2008, IEEE Trans Med Imaging 27(7): 934-942." << endl;
  cout << "Good luck!" << endl << endl;
  cout << "*** Notification ***" << endl << endl;

  //Sleep(1000);

  // Output name of program plus version number and copyrights
  cout << PROG_NAME << " " << PROG_VERSION << " " << COPYRIGHT << endl << endl;

  // Get system info
  GetSystemInfo(&systeminfo);

  // Get memory info
  mstat.dwLength = sizeof(mstat);
  GlobalMemoryStatus(&mstat);

  // Check number of threads
  nthreads = systeminfo.dwNumberOfProcessors;
  if (nthreads < 1)
  {
  	cerr << "Error: threads could not be initialized, aborting!" << endl;
  	return 1;
  }

  // Print system details
  if (print_systeminfo(systeminfo, mstat))
  {
  	return 1;
  }

  // Print help if no options have been given
  if (argc == 1)
  {
    print_usage();
    exit(0);
  }

  // Print version number or help if requested
  for(i = 1; i < argc; i++)
  {
    if(strstr(argv[i], "--version"))
    {
      print_build();
      exit(0);
    }
    else if(strstr(argv[i], "--help") ||
            strstr(argv[i], "--h") ||
            strstr(argv[i], "-h"))
    {
      print_usage();
      exit(0);
    }
  }

  start = time(0);                                     // get current time

  cout << "Reading command line parameters:" << endl << endl;

  // Read options from command line
  while (--argc > 0 && (*++argv)[0] == '-')
  {
    for (str = argv[0]+1; *str!= '\0'; str++)
    {
      switch (*str)
      {
        case 'a':
        {
          strcpy(inatten,*(argv+1));
          argc--;
          argv++;
          hasatten = true;
	      if (verboseMode == 1)
	        cout << "  input attenuation sino: " << inatten << endl;
        } break;
        case 'd':
        {
          strcpy(inrandoms,*(argv+1));
          argc--;
          argv++;
	      if (verboseMode == 1)
	        cout << "  input randoms sinogram: " << inrandoms << endl;
        } break;
        case 'g':
        {
          gapfill = atoi(*(argv+1));
          argc--;
          argv++;
	      cout << "  missing data estimation method: ";
	      switch(gapfill)
	      {
	        case 0:
	        {
	          cout << "none" << endl;
	          gapfill = 0;
	        } break;
	        case 1:
	        {
	          cout << "linear interpolation" << endl;
	          gapfill = 1;
	        } break;
	        case 2:
	        {
	          cout << "confosp" << endl;
	          gapfill = 2;
	        } break;
	        default:
	        {
	          cout << "unknown method, aborting..." << endl;
	          return 1;
	        }
	      }
        } break;
        case 'i':
        {
          niter = atoi(*(argv+1));
          argc--;
          argv++;
	      cout << "  number of iterations: " << niter << endl;
        } break;
        case 'n':
        {
          strcpy(innorm,*(argv+1));
          argc--;
          argv++;
	      cout << "  input norm sino: " << innorm << endl;
        } break;
        case 'o':
        {
          strcpy(outsino,*(argv+1));
          argc--;
          argv++;
	      cout << "  output corrected sinogram: " << outsino << endl;
        } break;
        case 'p':
        {
          strcpy(insino,*(argv+1));
          argc--;
          argv++;
	      if (verboseMode == 1)
	        cout << "  input prompts sinogram: " << insino << endl;
        } break;
        case 's':
        {
          strcpy(inscatter,*(argv+1));
          argc--;
          argv++;
          hasscatter = true;
	      if (verboseMode == 1)
	        cout << "  input scatter sino: " << inscatter << endl;
        } break;
        case 'v':
        {
	      cout << "  started at: " << ctime(&start);
          verboseMode = 1;
          cout << "  verbose mode: on" << endl;
        } break;
        default:
        {
          cerr << "Error: illegal option -" << *str << endl;
          argc = 0;
          return 1;
        }
      }
    }
  }

/***********************************************************************
 * Check given options
 ***********************************************************************/

  // Emission sinogram
  switch(checkfileextension(insino, 's'))
  {
  	case 0:
  	{
      // Successful, do nothing!
  	} break;
  	case 1:
  	{
  	  cerr << "Error: wrong file extension for emission sinogram (.s)" << endl;
  	  return 1;
  	} break;
  	case 2:
  	{
  	  cerr << "Error: missing input emission sinogram" << endl;
  	  return 1;
  	} break;
  	default:
  	{
  	  cerr << "Unknown error while checking sinogram extension, aborting program!" << endl;
  	  return 1;
  	}
  }

  // Randoms sinogram
  switch(checkfileextension(inrandoms, 's'))
  {
  	case 0:
  	{
      // Successful, do nothing!
  	} break;
  	case 1:
  	{
  	  cerr << "Error: wrong file extension for randoms sinogram (.s)" << endl;
  	  return 1;
  	} break;
  	case 2:
  	{
  	  cerr << "Error: missing input randoms sinogram" << endl;
  	  return 1;
  	} break;
  	default:
  	{
  	  cerr << "Unknown error while checking sinogram extension, aborting program!" << endl;
  	  return 1;
  	}
  }

  // Normalization sinogram
  switch(checkfileextension(innorm, 'n'))
  {
	case 0:
  	{
      // Successful, do nothing!
  	} break;
  	case 1:
  	{
  	  cerr << "Error: wrong file extension for normalization sinogram (.n)" << endl;
  	  return 1;
  	} break;
  	case 2:
  	{
  	  cerr << "Error: missing input normalization sinogram" << endl;
  	  return 1;
  	} break;
  	default:
  	{
  	  cerr << "Unknown error while checking sinogram extension, aborting program!" << endl;
  	  return 1;
  	}
  }

  if (hasatten)
  {
    // Attenuation sinogram
    switch(checkfileextension(inatten, 'a'))
    {
  	  case 0:
  	  {
  	    // Attenuation exists, sinogram needs to be attenuation corrected!
  	  } break;
  	  case 1:
  	  {
  	    cerr << "Error: wrong file extension for attenuation sinogram (.a)" << endl;
  	    return 1;
  	  } break;
  	  case 2:
  	  {
  	  	cerr << "Error: missing input attenuation sinogram" << endl;
  	  	return 1;
   	  } break;
  	  default:
  	  {
  	    cerr << "Unknown error while checking sinogram extension, aborting program!" << endl;
  	    return 1;
  	  }
    }
  }

  if (hasscatter)
  {
    switch(checkfileextension(inscatter, 's'))
    {
  	  case 0:
  	  {
  	    // Scatter exists, sinogram needs to be scatter corrected!
  	  } break;
  	  case 1:
  	  {
  	    cerr << "Error: wrong file extension for scatter sinogram (.s)" << endl;
  	    return 1;
  	  } break;
  	  case 2:
  	  {
  		cerr << "Error: missing input scatter sinogram" << endl;
  		return 1;
  	  } break;
  	  default:
  	  {
  	    cerr << "Unknown error while checking sinogram extension, aborting program!" << endl;
  	    return 1;
  	  }
    }
  }

/***********************************************************************
 * Construct output files (when not given)
 ***********************************************************************/

  // Construct output file if not given
  if (!strcmp(outsino,""))
  {
    // Get length
  	length = strlen(insino);
    // Copy name without extension .s
  	strncpy(outsino, insino, length-2);
    // Add '_3drp.i' to name
  	strcpy (&(outsino[length-2]), "_corrected.s");
	cout << "  output corrected sinogram: " << outsino << endl;
  }

  // Clear file (if exists)
  filestream.open(outsino, ios::out | ios::binary);
  if (!filestream.is_open())
  {
    cerr << "Error: could not open file " << outsino << endl;  	
    return 1;
  }
  filestream.close();

  // Construct header file
  if (!strcmp(inheader,""))
  {
    // Get length
  	length = strlen(insino);
    // Copy name
  	strncpy(inheader, insino, length);
    // Add '.hdr' to name
  	strcpy (&(inheader[length]), ".hdr");
	cout << "  expected header: " << inheader << endl;
  }

  // Construct output header file if not given
  if (!strcmp(outheader,""))
  {
    // Get length
  	length = strlen(outsino);
    // Copy name
  	strncpy(outheader, outsino, length);
    // Add '.hdr' to name
  	strcpy (&(outheader[length]), ".hdr");
	cout << "  output corrected sinogram hdr: " << outheader << endl;
  }

  // Clear file (if exists)
  filestream.open(outheader, ios::out | ios::binary);
  if (!filestream.is_open())
  {
    cerr << "Error: could not open file " << outheader << endl;  	
    return 1;
  }
  filestream.close();

/***********************************************************************
 * Initialze variables which could not be set earlier
 ***********************************************************************/

  nsegs = (2*rd+1)/span;
  ntotal = nsegs*nplanes - (nsegs-1)*(span*(nsegs-1)/2+1);
  sinosize = nelems*nangles*ntotal;
  nsegsmax = ((nsegs-1)/2) + 1;

  // Create array
  if((segplanes = new int[nsegsmax]) == 0)
  {
    cerr << "Error: memory allocation failed for size " << nsegsmax*sizeof(int) << ", aborting" << endl;
    // Clean up
    return 1;
  }

  segplanes[0] = nplanes;                         // First segment 0
  for (seg = 1; seg < nsegsmax; seg++)        // For all other segments
  {
   	segplanes[seg] = 2*nrings - span*(2*seg-1)-2;
  }

/***********************************************************************
 * Check files
 ***********************************************************************/

  // Emission sinogram
  switch(checksinogram(insino, sinosize))
  {
  	case 0:
    {
  	  cerr << "Error: could not open file " << insino << endl;
  	  return 1;
    } break;
  	case 1:
    {
  	  cerr << "Error: " << insino << " is not a valid 3D sinogram" << endl;
  	  return 1;
    } break;
  	case 2:
    {
      convertprompts = true;
  	  if (verboseMode)
  	    cout << endl << "  emission sinogram in int format and will be converted" << endl;
    } break;
  	case 3:
  	{
  	  if (verboseMode)
  	    cout << endl << "  emission sinogram in float format" << endl;
  	} break;
  	default:
  	{
  	  cerr << "Unknown error while checking sinogram, aborting program!" << endl;
  	  return 1;
  	}
  }

  // Emission sinogram
  switch(checksinogram(inrandoms, sinosize))
  {
  	case 0:
    {
  	  cerr << "Error: could not open file " << inrandoms << endl;
  	  return 1;
    } break;
  	case 1:
    {
  	  cerr << "Error: " << inrandoms << " is not a valid 3D sinogram" << endl;
  	  return 1;
    } break;
  	case 2:
    {
      convertrandoms = true;
  	  if (verboseMode)
  	    cout << endl << "  emission sinogram in int format and will be converted" << endl;
    } break;
  	case 3:
  	{
  	  if (verboseMode)
  	    cout << endl << "  emission sinogram in float format" << endl;
  	} break;
  	default:
  	{
  	  cerr << "Unknown error while checking sinogram, aborting program!" << endl;
  	  return 1;
  	}
  }

  // Normalization sinogram
  switch(checksinogram(innorm, sinosize))
  {
  	case 0:
    {
  	  cerr << "Error: could not open file " << innorm << endl;
  	  return 1;
    } break;
  	case 1:
    {
  	  cerr << "Error: " << innorm << " is not a valid 3D sinogram" << endl;
  	  return 1;
    } break;
  	case 2:
    {
  	  cerr << "Error: normalization sinogram in int format, needs to be float!" << endl;
  	  return 1;
    } break;
  	case 3:
  	{
  	  if (verboseMode)
  	    cout << "  normalization sinogram in float format" << endl;
  	} break;
  	default:
  	{
  	  cerr << "Unknown error while checking sinogram, aborting program!" << endl;
  	  return 1;
  	}
  }

  // Attenuation sinogram
  if (hasatten)
  {
    switch(checksinogram(inatten, sinosize))
    {
  	  case 0:
      {
  	    cerr << "Error: could not open file " << inatten << endl;
  	    return 1;
      } break;
  	  case 1:
      {
  	    cerr << "Error: " << inatten << " is not a valid 3D sinogram" << endl;
  	    return 1;
      } break;
  	  case 2:
      {
  	    cerr << "Error: attenuation sinogram in int format, needs to be float!" << endl;
  	    return 1;
      } break;
  	  case 3:
  	  {
  	    if (verboseMode)
  	      cout << "  attenuation sinogram in float format" << endl;
  	  } break;
  	  default:
  	  {
  	    cerr << "Unknown error while checking sinogram, aborting program!" << endl;
  	    return 1;
  	  }
    }
  }

  // Scatter sinogram
  if (hasscatter)
  {
    switch(checksinogram(inscatter, sinosize))
    {
  	  case 0:
      {
  	    cerr << "Error: could not open file " << inscatter << endl;
  	    return 1;
      } break;
  	  case 1:
      {
  	    cerr << "Error: " << inscatter << " is not a valid 3D sinogram" << endl;
  	    return 1;
      } break;
  	  case 2:
      {
  	    cerr << "Error: scatter sinogram in int format, needs to be float!" << endl;
  	    return 1;
      } break;
  	  case 3:
  	  {
  	    if (verboseMode)
  	      cout << "  scatter sinogram in float format" << endl;
  	  } break;
  	  default:
  	  {
  	    cerr << "Unknown error while checking sinogram, aborting program!" << endl;
  	    return 1;
  	  }
    }
  }

/***********************************************************************
 * Creating image header
 ***********************************************************************/

  // Construct header
  header.correctedname = outsino;
  header.nelems = 256;
  header.nangles = 288;
  header.nplanes = ntotal;
  header.prompt = insino;
  header.delayed = inrandoms;
  header.norm = innorm;
  header.atten = inatten;
  header.scatter = inscatter;
  header.gapfill = gapfill;

  // Output image
  if(constructheader(inheader, outheader, header))
  {
    cerr << "Error: could not make header file, aborting" << endl;
    // Clean up
    return 1;
  }

/***********************************************************************
 * START
 ***********************************************************************/

    cout << endl << "Starting the program:" << endl << endl;

	for (seg = 0; seg < nsegsmax; seg++)
	{
	  now = time(0);                                     // get current time
	  timeinfo = localtime(&now);
	  sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	  cout << curtime << "  Estimate missing data for segment +-" << seg << "..." << endl;

	  if (seg == 0)
	  {
		nplanes = segplanes[seg];
	  }
	  else
	  {
        nplanes = 2*segplanes[seg];
	  }

	  segsize = nelems*nangles*nplanes;

	  // Create array
	  if((sinogram = new float[segsize]) == 0)
	  {
		cerr << "Error: memory allocation failed for size " << segsize*sizeof(float) << ", aborting" << endl;
		// Clean up
		return 1;
	  }

	  for (i = 0; i < segsize; i++)
		sinogram[i] = 0.0;

	  if (convertprompts)
	  {
		now = time(0);                                     // get current time
		timeinfo = localtime(&now);
		sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		cout << curtime << "    Converting emission sinogram to float..." << endl;

		if((intsino = new short int[segsize]) == 0)
		{
		  cerr << "Error: memory allocation failed for size " << segsize*sizeof(short) << ", aborting" << endl;
		  // Clean up
		  delete[] sinogram;
		  delete[] segplanes;
		  return 1;
		}

		for (i = 0; i < segsize; i++)
		  intsino[i] = 0;

		// Open and read data
		if(openandreaddata(insino, intsino, segsize, offset))
		{
		  cerr << "Error: could not read data, aborting!" << endl;
		  delete[] sinogram;
		  delete[] segplanes;
		  delete[] intsino;
		  return 1;
		}

        // Convert to float
        for(i = 0; i < segsize; i++)
          sinogram[i] = float(intsino[i]);

		delete[] intsino;
	  }
	  else
	  {
		// Open and read data
		if(openandreaddata(insino, sinogram, segsize, offset))
		{
		  cerr << "Error: could not read data, aborting!" << endl;
		  delete[] sinogram;
		  delete[] segplanes;
		  return 1;
		}
	  }

	  // Create array
	  if((randoms = new float[segsize]) == 0)
	  {
		cerr << "Error: memory allocation failed for size " << segsize*sizeof(float) << ", aborting" << endl;
		// Clean up
		delete[] sinogram;
		delete[] segplanes;
		return 1;
	  }

	  for (i = 0; i < segsize; i++)
		randoms[i] = 0.0;

	  if (convertrandoms)
	  {
		now = time(0);                                     // get current time
		timeinfo = localtime(&now);
		sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		cout << curtime << "    Converting randoms sinogram to float..." << endl;

		if((intsino = new short int[segsize]) == 0)
		{
		  cerr << "Error: memory allocation failed for size " << segsize*sizeof(short) << ", aborting" << endl;
		  // Clean up
		  delete[] sinogram;
		  delete[] randoms;
		  delete[] segplanes;
		  return 1;
		}

		for (i = 0; i < segsize; i++)
		  intsino[i] = 0;

		// Open and read data
		if(openandreaddata(inrandoms, intsino, segsize, offset))
		{
		  cerr << "Error: could not read data, aborting!" << endl;
		  delete[] sinogram;
		  delete[] segplanes;
		  delete[] randoms;
		  delete[] intsino;
		  return 1;
		}

        // Convert to float
        for(i = 0; i < segsize; i++)
          randoms[i] = float(intsino[i]);

		delete[] intsino;
	  }
	  else
	  {
		// Open and read data
		if(openandreaddata(inrandoms, randoms, segsize, offset))
		{
		  cerr << "Error: could not read data, aborting!" << endl;
		  delete[] sinogram;
		  delete[] randoms;
		  delete[] segplanes;
		  return 1;
		}
	  }

	  now = time(0);                                     // get current time
	  timeinfo = localtime(&now);
	  sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	  cout << curtime << "    Applying corrections (P-R)..." << endl;

	  for (i = 0; i < segsize; i++)
	    sinogram[i] = sinogram[i] - randoms[i];

	  // Clean up
	  delete[] randoms;

	/***********************************************************************
	 * Correct emission sinogram
	 ***********************************************************************/

      if((norm = new float [segsize]) == 0)
	  {
		cerr << "Error: memory allocation failed for size " << segsize*sizeof(float) << ", aborting" << endl;
		// Clean up
		delete[] sinogram;
		delete[] segplanes;
		return 1;
	  }

	  for (i = 0; i < segsize; i++)
		norm[i] = 0.0;

	  // Open and read data
	  if(openandreaddata(innorm, norm, segsize, offset))
	  {
		cerr << "Error: could not read data, aborting!" << endl;
		delete[] sinogram;
		delete[] segplanes;
		delete[] norm;
		return 1;
	  }

	  if (hasatten)
	  {
		now = time(0);                                     // get current time
		timeinfo = localtime(&now);
		sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		cout << curtime << "    Applying corrections (A*N)..." << endl;

		if((atten = new float [segsize]) == 0)
		{
			cerr << "Error: memory allocation failed for size " << segsize*sizeof(float) << ", aborting" << endl;
			// Clean up
			delete[] sinogram;
			delete[] segplanes;
			delete[] norm;
			return 1;
		}

		for (i = 0; i < segsize; i++)
			atten[i] = 0.0;

		// Open and read data
		if(openandreaddata(inatten, atten, segsize, offset))
		{
			cerr << "Error: could not read data, aborting!" << endl;
			delete[] sinogram;
			delete[] segplanes;
			delete[] norm;
			delete[] atten;
			return 1;
		}

		// Correct sinogram
		for (i = 0; i < segsize; i++)
		{
		  sinogram[i] = sinogram[i]*atten[i]*norm[i];
		}
	  }
	  else
	  {
		now = time(0);                                     // get current time
		timeinfo = localtime(&now);
		sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		cout << curtime << "    Applying corrections (*N)..." << endl;

		// Correct sinogram
		for (i = 0; i < segsize; i++)
		{
		  sinogram[i] = sinogram[i]*norm[i];
		}
	  }

	  /***********************************************************************
	   * Start gapfilling depending on method
	   ***********************************************************************/

	  now = time(0);                                     // get current time
	  timeinfo = localtime(&now);
	  sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	  switch (gapfill)
	  {
		// No gapfilling
		case 0:
		{
		  cout << curtime << "    No gapfilling (not recommended)..." << endl;
		} break;
		// Linear interpolation (angles)
		case 1:
		{
		  cout << curtime << "    Gap filling by linear interpolation (angles)..." << endl;
		  if(linearinterpolation_angles(sinogram, norm, nelems, nangles, nplanes, 0))
		  {
			cerr << "Error: could not do interpolation gap filling, aborting!" << endl;
			// Clean up
			delete[] sinogram;
			delete[] segplanes;
			delete[] norm;
			if (hasatten) delete[] atten;
		  }
		} break;
		// Confos
		case 2:
		{
		  cout << curtime << "    Constrained Fourier Space gap filling..." << endl;

		  // Confos gapfill initial estimate
		  if (confos2D_thread (sinogram, norm, extention, niter, nelems, nangles, nplanes, interpolarea, verboseMode, nthreads))
		  {
			cerr << "Error: could not do contrained fourier space gap filling, aborting!" << endl;
			// Clean up
			delete[] sinogram;
			delete[] segplanes;
			delete[] norm;
			if (hasatten) delete[] atten;
		  }
		} break;
		default:
		{
			cerr << "Error: unknown method for gapfilling, aborting program!" << endl;
			// Clean up
			delete[] sinogram;
			delete[] segplanes;
			delete[] norm;
			if (hasatten) delete[] atten;
			return 1;
		}
	  }

	  /***********************************************************************
	   * Destroy norm
	  ***********************************************************************/

	  // Clean up
	  delete[] norm;

	/***********************************************************************
	 * Scatter correct emission sinogram
	 ***********************************************************************/

	  if (hasscatter)
	  {
		now = time(0);                                     // get current time
		timeinfo = localtime(&now);
		sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		if(hasatten)
		{
		  cout << curtime << "    Applying scatter corrections (-S*A)..." << endl;
		}
		else
		{
		  cout << curtime << "    Applying scatter corrections (-S), presuming scatter is precorrected..." << endl;
		}

		if((scatter = new float [segsize]) == 0)
		{
		  cerr << "Error: memory allocation failed for size " << segsize*sizeof(float) << ", aborting" << endl;
		  // Clean up
		  delete[] sinogram;
		  delete[] segplanes;
		  if (hasatten) delete[] atten;
		  return 1;
		}

		for (i = 0; i < segsize; i++)
			scatter[i] = 0.0;

		// Open and read data
		if(openandreaddata(inscatter, scatter, segsize, offset))
		{
			cerr << "Error: could not read data, aborting!" << endl;
			delete[] sinogram;
			delete[] segplanes;
			if (hasatten) delete[] atten;
			delete[] scatter;
			return 1;
		}

		if(hasatten)
		{
		  for (i = 0; i < segsize; i++)
			  scatter[i] *= atten[i];

		  // Clean up
		  delete[] atten;
		}

		// Correct sinogram
		for (i = 0; i < segsize; i++)
          sinogram[i] -= scatter[i];

		// Clean up
		delete[] scatter;
	  }
	  else
	  {
		now = time(0);                                     // get current time
		timeinfo = localtime(&now);
		sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		cout << curtime << "    Sinogram is assumed to be scatter corrected..." << endl;
	  }

	 /***********************************************************************
	  * Write data
	  ***********************************************************************/

	  now = time(0);                                     // get current time
	  timeinfo = localtime(&now);
	  sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	  cout << curtime << "    Writing corrected segment..." << endl;

	  if(openandwritedata(outsino, sinogram, segsize, offset))
	  {
	    cerr << "Error: could not write data, aborting!" << endl;
	    // Clean up
	    delete[] sinogram;
		return 1;
	  }

    /***********************************************************************
	 * Upgrade variables and clean up for next round
	 ***********************************************************************/

	  // Upgrade variables
	  offset += segsize;

	  // Clean up
	  delete[] sinogram;
	}

/***********************************************************************
 * Exit and clean up
 ***********************************************************************/

  now = time(0);                                     // get current time
  cout << endl << PROG_NAME << " version " << PROG_VERSION << " finished in " << now-start << " seconds" << endl;

  return 0;
}

/* Function print_usage(): Print details about usage to screen */
void print_usage(void)
{
  cout << "SYNTAX:" << endl;
  cout << PROG_NAME << endl;
  cout << "      [--h] [--version] [-v]" << endl;
  cout << "      [-e file] [-n file] [-a file] [-c file] [-o file]" << endl;
  cout << "      [-g number] [-i number] [-r number] [-R number]" << endl;
  cout << "      [-x number] [-y number] [-X number] [-Y number]" << endl;
  cout << "      [-z number]" << endl << endl;
  cout << "Corrects and gap fills a 3D HRRT sinogram in span 9." << endl << endl;
  cout << " -a filename     input 3D attenuation sinogram file name (optional)" << endl;
  cout << " -d filename     input 3D random smoothed sinogram file name (required)" << endl;
  cout << " -g number       missing data estimate algorithm (default = 2)" << endl;
  cout << "                   0: no gapfilling" << endl;
  cout << "                   1: linear interpolation" << endl;
  cout << "                   2: constrained Fourier space (confosp)" << endl;
  cout << " --help          print this message and exit (or -h, --h)" << endl;
  cout << " -i number       confosp number of iterations (default = 15)" << endl;
  cout << " -n filename     input 3D normalization sinogram file name (required)" << endl;
  cout << " -o filename     output corrected sinogram file name (optional)" << endl;
  cout << " -p filename     input 3D emission sinogram file name (required)" << endl;
  cout << " -s filename     input scatter sinogram file name (optional)" << endl;
  cout << " -v              verbose mode (print extra information to screen)" << endl;
  cout << " --version       print details of build and version number" << endl;
}

/* Function print_build(): Print build information to screen */
void print_build()
{
  cout << PROG_VERSION << " was build on " <<  __DATE__ << " " << __TIME__ << endl << endl;
  cout << "Version history:" << endl;
  cout << "  0.0.1 August 21 2006 (Floris van Velden, fvv)" << endl;
  cout << "       initial 3DRP program" << endl;
  cout << "  1.0.0 May 7 2009 (fvv)" << endl;
  cout << "       separate program to fill gaps in HRRT sinogram" << endl;
}

int print_systeminfo(SYSTEM_INFO info, MEMORYSTATUS mstat)
{
  cout << "Hardware information:" << endl << endl;
  cout << "  processor: " << info.wProcessorArchitecture << " " << info.dwProcessorType << endl;
  cout << "  number of processors: " << info.dwNumberOfProcessors << endl;
  cout << "  total physical memory: " << mstat.dwTotalPhys << endl;
  cout << "  available physical memory: " << mstat.dwAvailPhys << endl;
  cout << "  page size: " << info.dwPageSize << endl;
  cout << "  minimum application address: " << info.lpMinimumApplicationAddress << endl;
  cout << "  maximum application address: " << info.lpMaximumApplicationAddress << endl;
  cout << endl;

  return 0;
}

/*
 * Return values:
 *
 * 0: could not open file
 * 1: unknown format
 * 2: int format
 * 3: float format
 */

int checksinogram (char* filename, int sinosize)
{
  int begin, end, filesize;
  fstream filestream;

  // Check if file can be opened
  filestream.open(filename, ios::in | ios::binary);
  if (!filestream.is_open())
  {
    return 0;
  }

  // Get filesize
  begin = filestream.tellg();
  filestream.seekg (0, ios::end);
  end = filestream.tellg();
  filesize = end - begin;

  // Close file
  filestream.close();

  // Check type of sinogram
  if ((unsigned) filesize == (sinosize*sizeof(float)))
  {
  	return 3;
  }
  else if ((unsigned) filesize == (sinosize*sizeof(short)))
  {
  	return 2;
  }
  else
  {
  	return 1;
  }

  return 0;
}

/*
 * Return values
 * 0: success
 * 1: wrong file extention
 * 2: no file name given
 */

int checkfileextension (char* filename, char extension)
{
  int length;

  if (strcmp(filename,""))
  {
    // Check file extension
    length = strlen(filename);
    if (length < 2 || !(filename[length-1] == extension && filename[length-2] == '.'))
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
  	return 2;
  }

  return 2;
}

int openandreaddata(char* infile, short int* data, int size, int offset)
{
  fstream instream;

  // Open file
  instream.open(infile, ios::in | ios::binary);

  if (!instream.is_open())
  {
    cerr << "Error: could not open file " << infile << endl;
    return 1;
  }

  // Seek in files
  instream.seekg(offset*sizeof(short int), ios::beg);

  // Read all into sinogram read buffer
  if (instream.read(reinterpret_cast<char *>(data), sizeof(short int) * size).bad())
  {
    cerr << "Error: failed reading file " << infile << endl;
    // Clean up
    instream.close();
    return 1;
  }

  // Close file
  instream.close();

  // Check if closed
  if (instream.is_open())
  {
    cerr << "Error: could not close file " << infile << endl;
    return 1;
  }

  return 0;
}

int openandreaddata(char* infile, float* data, int size, int offset)
{
  fstream instream;

  // Open file
  instream.open(infile, ios::in | ios::binary);

  if (!instream.is_open())
  {
    cerr << "Error: could not open file " << infile << endl;
    return 1;
  }

  // Seek in files
  instream.seekg(offset*sizeof(float), ios::beg);

  // Read all into sinogram read buffer
  if (instream.read(reinterpret_cast<char *>(data), sizeof(float) * size).bad())
  {
    cerr << "Error: failed reading file " << infile << endl;
    // Clean up
    instream.close();
    return 1;
  }

  // Close file
  instream.close();

  // Check if closed
  if (instream.is_open())
  {
    cerr << "Error: could not close file " << infile << endl;
    return 1;
  }

  return 0;
}

int openandwritedata(char* outfile, float* data, int size, int offset)
{
  fstream outstream;

  // Open file
  outstream.open(outfile, ios::out | ios::binary | ios::app);

  if (!outstream.is_open())
  {
    cerr << "Error: could not open file " << outfile << endl;
    return 1;
  }

  // Seek in files
  outstream.seekg(offset*sizeof(float), ios::beg);

  // Read all into sinogram read buffer
  if (outstream.write(reinterpret_cast<char *>(data), sizeof(float) * size).bad())
  {
    cerr << "Error: failed writing file " << outfile << endl;
    // Clean up
    outstream.close();
    return 1;
  }

  // Close file
  outstream.close();

  // Check if closed
  if (outstream.is_open())
  {
    cerr << "Error: could not close file " << outfile << endl;
    return 1;
  }

  return 0;
}
