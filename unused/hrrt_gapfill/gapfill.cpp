#include <iostream>
#include <ctime>
#include <string>
#include "interpolation.h"
#include "gapfill.h"
using namespace std;

int linearinterpolation_angles(float* sinogram2D, float* normsino2D, int nelems, int nangles, int nplanes, bool verboseMode)
{
  time_t now;
  struct tm* timeinfo;
  char curtime[10];
  
/***********************************************************************
 * Start gapfilling for each plane
 ***********************************************************************/

  for (int plane = 0; plane < nplanes; plane++)
  {
  	if (verboseMode)
  	{
      now = time(0);                                     // get current time
      timeinfo = localtime(&now); 
      sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
      cout << curtime << "    <plane " << plane+1 << " out of " << nplanes << ">" << endl;
  	}

    // Do for all bins
    for (int bin = 0; bin < nelems; bin++)
    {
      // Screen all angles
      for (int angle = 0; angle < nangles; angle++)
      {
      	// Interpolate if we have found a gap and we're not on the border (otherwise do nothing)
      	if ((normsino2D[bin + angle*nelems + plane*nelems*nangles] == 0))
      	{
      	  int angleprev = angle-1;
      	  int anglenext = angle+1;
      	  int indexlength = 0;

      	  // Search for next known non zero value
      	  while((normsino2D[bin + (((angleprev+nangles)%nangles)*nelems) + plane*nelems*nangles] == 0) && (((angleprev+nangles)%nangles) != angle))
      	  {
      	    angleprev--;
      	  }
      	  
      	  // Search for next known non zero value
      	  while((normsino2D[bin + ((anglenext%nangles)*nelems) + plane*nelems*nangles] == 0) && ((anglenext%nangles) != angle))
      	  {
      	    anglenext++;
      	  }

      	  // If there is no value which is bigger than zero or if we get out of upper bound, continue to next bin
      	  if ((angleprev >= anglenext) || (anglenext >= nangles))
      	  {
      	  	angle = nangles;
      	  }
      	  // Interpolate
      	  else
      	  {
      	  	// Calculate number of elements to interpolate
      	  	indexlength = anglenext - angleprev;
      	  	      	  	   	
            for (int i = 1; i < indexlength; i++)
            {  
              sinogram2D[bin + ((i + angleprev + nangles)%nangles)*nelems + plane*nelems*nangles] = linearinterpolate(i + angleprev, angleprev, anglenext, sinogram2D[bin + ((angleprev+nangles)%nangles)*nelems + plane*nelems*nangles], sinogram2D[bin + anglenext*nelems + plane*nelems*nangles]);
            }
      	  }
      	}
      }	
    }
  }
  
  return 0;
}

int linearinterpolation_bins(float* sinogram2D, float* normsino2D, int nelems, int nangles, int nplanes, bool verboseMode)
{
  time_t now;
  struct tm* timeinfo;
  char curtime[10];
  
/***********************************************************************
 * Start gapfilling for each plane
 ***********************************************************************/

  for (int plane = 0; plane < nplanes; plane++)
  {
  	if (verboseMode)
  	{
      now = time(0);                                     // get current time
      timeinfo = localtime(&now); 
      sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
      cout << curtime << "    <plane " << plane+1 << " out of " << nplanes << ">" << endl;
  	}

    // Do for all angles
    for (int angle = 0; angle < nangles; angle++)
    {
      // Screen all bins
      for (int bin = 0; bin < nelems; bin++)
      {
      	// Interpolate if we have found a gap and we're not on the border (otherwise do nothing)
      	if ((normsino2D[bin + angle*nelems + plane*nelems*nangles] == 0))
      	{
      	  int binprev = bin-1;
      	  int binnext = bin+1;
      	  int indexlength = 0;

      	  // Search for next known non zero value
      	  while((normsino2D[((binprev+nelems)%nelems) + angle*nelems + plane*nelems*nangles] == 0) && (((binprev+nelems)%nelems) != bin))
      	  {
      	    binprev--;
      	  }
      	  
      	  // Search for next known non zero value
      	  while((normsino2D[(binnext%nelems) + angle*nelems + plane*nelems*nangles] == 0) && ((binnext%nelems) != bin))
      	  {
      	    binnext++;
      	  }

      	  // If there is no value which is bigger than zero or if we get out of upper bound, continue to next angle
      	  if ((binprev >= binnext) || (binnext >= nelems))
      	  {
      	  	bin = nelems;
      	  }
      	  // Interpolate
      	  else
      	  {
      	  	// Calculate number of elements to interpolate
      	  	indexlength = binnext - binprev;
      	  	      	  	   	
            for (int i = 1; i < indexlength; i++)
            {
              sinogram2D[((i + binprev + nelems)%nelems) + angle*nelems + plane*nelems*nangles] = linearinterpolate(i + binprev, binprev, binnext, sinogram2D[(binprev+nelems)%nelems + angle*nelems + plane*nelems*nangles], sinogram2D[binnext + angle*nelems + plane*nelems*nangles]);
            }
      	  }
      	}
      }	
    }
  }
  
  return 0;
}

int bilinearinterpolation(float* sinogram2D, float* normsino2D, int nelems, int nangles, int nplanes, bool verboseMode)
{
  time_t now;
  struct tm* timeinfo;
  char curtime[10];
  float* sinograminterpol;
  
/***********************************************************************
 * Start gapfilling for each plane
 ***********************************************************************/

  for (int plane = 0; plane < nplanes; plane++)
  {
  	if (verboseMode)
  	{
      now = time(0);                                     // get current time
      timeinfo = localtime(&now); 
      sprintf(curtime, "(%02d:%02d:%02d)", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
      cout << curtime << "    <plane " << plane+1 << " out of " << nplanes << ">" << endl;
  	}

    // Create array
    if((sinograminterpol = new float[nelems*nangles]) == 0) 
    {
      cerr << "Error: memory allocation failed for size " << nelems*nangles*sizeof(float) << ", aborting" << endl;
	  return 1;
    }
    
    for (int i = 0; i < nelems*nangles; i++)
      sinograminterpol[i] = 0.0;

    // Do for all bins
    for (int bin = 0; bin < nelems; bin++)
    {
      // Screen all angles
      for (int angle = 0; angle < nangles; angle++)
      {
      	// Interpolate if we have found a gap and we're not on the border (otherwise do nothing)
      	if ((normsino2D[bin + angle*nelems + plane*nelems*nangles] == 0))
      	{
      	  int angleprev = angle-1;
      	  int anglenext = angle+1;
      	  int indexlength = 0;

      	  // Search for next known non zero value
      	  while((normsino2D[bin + (((angleprev+nangles)%nangles)*nelems) + plane*nelems*nangles] == 0) && (((angleprev+nangles)%nangles) != angle))
      	  {
      	    angleprev--;
      	  }
      	  
      	  // Search for next known non zero value
      	  while((normsino2D[bin + ((anglenext%nangles)*nelems) + plane*nelems*nangles] == 0) && ((anglenext%nangles) != angle))
      	  {
      	    anglenext++;
      	  }

      	  // If there is no value which is bigger than zero or if we get out of upper bound, continue to next bin
      	  if ((angleprev >= anglenext) || (anglenext >= nangles))
      	  {
      	  	angle = nangles;
      	  }
      	  // Interpolate
      	  else
      	  {
      	  	// Calculate number of elements to interpolate
      	  	indexlength = anglenext - angleprev;
      	  	      	  	   	
            for (int i = 1; i < indexlength; i++)
            {  
              sinograminterpol[bin + ((i + angleprev + nangles)%nangles)*nelems] = linearinterpolate(i + angleprev, angleprev, anglenext, sinogram2D[bin + ((angleprev+nangles)%nangles)*nelems + plane*nelems*nangles], sinogram2D[bin + anglenext*nelems + plane*nelems*nangles]);
            }
      	  }
      	}
      }	
    }

    // Do for all angles
    for (int angle = 0; angle < nangles; angle++)
    {
      // Screen all bins
      for (int bin = 0; bin < nelems; bin++)
      {
      	// Interpolate if we have found a gap and we're not on the border (otherwise do nothing)
      	if ((normsino2D[bin + angle*nelems + plane*nelems*nangles] == 0))
      	{
      	  int binprev = bin-1;
      	  int binnext = bin+1;
      	  int indexlength = 0;

      	  // Search for next known non zero value
      	  while((normsino2D[((binprev+nelems)%nelems) + angle*nelems + plane*nelems*nangles] == 0) && (((binprev+nelems)%nelems) != bin))
      	  {
      	    binprev--;
      	  }
      	  
      	  // Search for next known non zero value
      	  while((normsino2D[(binnext%nelems) + angle*nelems + plane*nelems*nangles] == 0) && ((binnext%nelems) != bin))
      	  {
      	    binnext++;
      	  }

      	  // If there is no value which is bigger than zero or if we get out of upper bound, continue to next angle
      	  if ((binprev >= binnext) || (binnext >= nelems))
      	  {
      	  	bin = nelems;
      	  }
      	  // Interpolate
      	  else
      	  {
      	  	// Calculate number of elements to interpolate
      	  	indexlength = binnext - binprev;
      	  	      	  	   	
            for (int i = 1; i < indexlength; i++)
            {
              sinogram2D[((i + binprev + nelems)%nelems) + angle*nelems + plane*nelems*nangles] = (sinograminterpol[((i + binprev + nelems)%nelems) + angle*nelems] + linearinterpolate(i + binprev, binprev, binnext, sinogram2D[(binprev+nelems)%nelems + angle*nelems + plane*nelems*nangles], sinogram2D[binnext + angle*nelems + plane*nelems*nangles]))/2.0;
            }
      	  }
      	}
      }	
    }
    
    delete[] sinograminterpol;
  }
  
  return 0;
}

int useestimateddata(float* sinogram, float* estimate, float* normsino, int nelems, int nangles, int nplanes, bool verboseMode)
{
  int sinosize = nelems*nangles*nplanes;
  
  for (int i = 0; i < sinosize; i++)
  {
  	// If gap found, then copy data from estimate
  	if (normsino[i] == 0)
  	{
  	  sinogram[i] = estimate[i];
  	}
  }
  
  return 0;
}
