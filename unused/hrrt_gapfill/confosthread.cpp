#include <iostream>
#include <ctime>
#include <string>
#include <fstream>
#include <process.h>
#include <windows.h>
#include "confosthread.h"
#include "interpolation.h"
#include "fft.h"
using namespace std;

#define TIMEOUTTHREAD 40000
#define maxn 1024
#define maxdc 256

int createMask(float* mask, int nelems, int nangles)
{
  int i=0, j=0, size = nangles*nelems;	
	
  // Reset all to value 1
  for (i = 0; i < size; i++)
  {
  	mask[i] = 1.0;
  }

  // Generate wedge-shaped mask (rotated wrt Karp due to location of low freq in corners)
  for (i = 0; i < (nelems/2); i++)
  {
  	for (j = 0; j < (nangles/2); j++)
  	{
  	  if (i < j*0.5)
  	  { 
  	  	mask[i + j*nelems] = 0.0;
  	  	mask[i + (2*(nelems)-1 - j)*nelems] = 0.0;
  	  }
  	  if (i > j*0.5)
  	  { 
  	  	mask[(nelems/2) + i + (nelems-1-j)*nelems] = 0.0;
  	  	mask[(nelems/2) + i + (nelems-1+j)*nelems] = 0.0; 	  	
  	  }
  	}
  }

  return 0;
}

unsigned __stdcall confosp (void *ptarg) 
{ 
  int i,bin, angle;
  ConFoSp *arg = (ConFoSp *) ptarg;
  int nelems = (*arg).nelems;
  int nangles = (*arg).nangles;
  int nanglesbig = (*arg).nanglesbig;
  int planesend = (*arg).nplanestoprocess + (*arg).planestart;
  int angleoffset,
      bigsinosize,
      planearraysize; 
  float *bigsino, 
        *normsino,
        *fftsino,
        *mask;

  planearraysize = nelems*nangles;

  // Set some variables  
  angleoffset = ((nanglesbig-nangles)/2);
  bigsinosize = nelems*nanglesbig;

/***********************************************************************
 * Create mask
 ***********************************************************************/

  // Create array
  if((mask = new float[bigsinosize]) == 0) 
  {
    cerr << "Error: memory allocation failed for size " << bigsinosize << ", aborting" << endl;
    // Clean up
	return 1;
  }

  for (i = 0; i < bigsinosize; i++)
  {
    mask[i] = 0.0;
  }

  // Create mask
  createMask(mask, nelems, nanglesbig);
  
/***********************************************************************
 * Start gapfilling for each plane
 ***********************************************************************/

  for (int plane = (*arg).planestart; plane < planesend; plane++)
  {
  	int planeplanearraysize = plane*planearraysize;

  /***********************************************************************
   * Init bigger array for sinogram and norm sinogram
   ***********************************************************************/
  
    // Create array
    if((bigsino = new float[bigsinosize]) == 0) 
    {
      cerr << "Error: memory allocation failed for size " << bigsinosize*sizeof(float) << ", aborting" << endl;
      // Clean up
      delete[] mask;
  	  return 1;
    }

    // Create array
    if((normsino = new float[bigsinosize]) == 0) 
    {
      cerr << "Error: memory allocation failed for size " << bigsinosize*sizeof(float) << ", aborting" << endl;
      // Clean up
      delete[] bigsino;
      delete[] mask;
	  return 1;
    }
    
    for (i = 0; i < bigsinosize; i++)
    {
      normsino[i] = 0.0;
      bigsino[i] = 0.0;
    }

    // Transform to bigger sinogram
    for (bin = 0; bin < nelems; bin++)
    {
      // fill middle interior of bigsino2D with sinogram2D
      for (angle = 0; angle < nangles; angle++)
      {
      	int anglenelems = angle*nelems;
      	int angleangleoffsetnelems = (angle + angleoffset)*nelems;
      	
        bigsino[bin + angleangleoffsetnelems] = (*arg).sinogram[bin + anglenelems + planeplanearraysize];
        normsino[bin + angleangleoffsetnelems] = (*arg).norm[bin + anglenelems + plane*nangles*nelems];
      }
    
      // fill remainder parts of bigsino2D with sinogram2D
      for (angle = 0; angle < angleoffset; angle++)
      {
      	int anglenelems = angle*nelems;
      	int angleoffsetnelems = (angle + angleoffset + nangles)*nelems;
      	
        switch((*arg).extention)
        {
      	  case 0:
      	  {
            bigsino[bin + anglenelems] = 0.0;
            bigsino[bin + angleoffsetnelems] = 0.0;      		
            normsino[bin + angle*nelems] = 0.0;
            normsino[bin + (angle + angleoffset + nangles)*nelems] = 0.0;
      	  } break;
      	  case 1:
      	  {
      	  	int binnew = (nelems - 1 - bin);
      	  	int anglenew = (nangles - angleoffset + angle)*nelems;
      	  	
            bigsino[bin + anglenelems] = (*arg).sinogram[binnew + anglenew + planeplanearraysize];
            bigsino[bin + angleoffsetnelems] = (*arg).sinogram[binnew + anglenelems + planeplanearraysize];
            normsino[bin + anglenelems] = (*arg).norm[binnew + anglenew + planeplanearraysize];
            normsino[bin + (angle + angleoffset + nangles)*nelems] = (*arg).norm[binnew + anglenelems + planeplanearraysize];
      	  } break;
      	  default:
      	  {
      	    cerr << "Error: unknown method for sinogram extention, aborting" << endl;
            delete[] mask;
      	    return 1;	
      	  }
        }
      }  
    }
  
  /***********************************************************************
   * CFS gap filling for one plane
   ***********************************************************************/

    // Create array
    if((fftsino = new float[bigsinosize]) == 0) 
    {
      cerr << "Error: memory allocation failed for size " << bigsinosize << ", aborting" << endl;
      // Clean up
      delete[] normsino;
      delete[] bigsino;
      delete[] mask;
	  return 1;
    }

    // Copy original sinogram to temp sinogram
    for (i = 0; i < bigsinosize; i++)
    {
      fftsino[i] = bigsino[i];
    }  

    // Do for all iterations
    for(int iteration = 0; iteration < (*arg).niter; iteration++)
    {
      // Fast fourier transform data
      FFTa_real_direct(fftsino, nanglesbig, bigsinosize, nelems);
	  FFTa_real_direct(fftsino, nelems, bigsinosize, 1);

      // Apply wedge mask in Fourier space
      for (i = 0; i < bigsinosize; i++)
      {
        fftsino[i] *= mask[i];
      }

      // Inverse fast fourier transform masked data
      FFTa_real_inverse(fftsino, nanglesbig, bigsinosize, nelems);
      FFTa_real_inverse(fftsino, nelems, bigsinosize, 1);
   
      // Put back original data in FFT where there are no gaps
      // Interpolate the borders to remove artifacts
      for (bin = 0; bin < nelems; bin++)
      {
        for (int angle = 0; angle < nanglesbig; angle++)
        {
          int curangle = angle*nelems;
          int prevangle = ((angle - 1 + nanglesbig)%nanglesbig)*nelems;
          int nextangle = ((angle + 1)%nanglesbig)*nelems;

          // Detect no gaps
          if (normsino[bin + curangle] != 0)
          {
        	// Fill with old data
            fftsino[bin + curangle] = bigsino[bin + curangle];
          }
          // Possible edge to interpolate
          else
          {
      	    // Correct/interpolate edge (edge found if next of prev is not zero, but cur is)
            if (normsino[bin + nextangle] != 0)
            {
              fftsino[bin + curangle] = (fftsino[bin + prevangle] + bigsino[bin + nextangle])/2.0;
            }
            else 
            {
              if (normsino[bin + prevangle] != 0)
              {
                fftsino[bin + curangle] = (fftsino[bin + nextangle] + bigsino[bin + prevangle])/2.0;            	
              }
            }
          }
      	}
      }

      // Interpolate 'strange points'
      for (bin = 0; bin < nelems; bin++)
      {
        for (int angle = 0; angle < nanglesbig; angle++)
        {
          int prev = bin + ((angle-1+nanglesbig)%nanglesbig)*nelems;
          int cur  = bin + angle*nelems;
          int next = bin + ((angle+1)%nanglesbig)*nelems;
       
          // Remove 'strange points'
          if ((normsino[prev] == 0) && (normsino[cur] != 0) && (normsino[next] == 0))
          {
            // Bilinear interpolate
            for (int x = -(*arg).interpolarea; x <= (*arg).interpolarea; x++)
            {
          	  int bincur = (bin+x+nelems)%nelems;
          	  int binleft = (bin-(*arg).interpolarea-1+nelems)%nelems;
          	  int binright = (bin+(*arg).interpolarea+1+nelems)%nelems;
          	          	
          	  for (int y = -(*arg).interpolarea; y <= (*arg).interpolarea; y++)
          	  {
                int anglecur  = ((angle + y + nanglesbig)%nanglesbig)*nelems;

          	    if (normsino[bincur + anglecur] == 0)
          	    {
                  int angletop = ((angle - (*arg).interpolarea - 1 + nanglesbig)%nanglesbig)*nelems;
                  int anglebot = ((angle + (*arg).interpolarea + 1 + nanglesbig)%nanglesbig)*nelems;

                  // Interpolate
          	      fftsino[bincur + anglecur] = (linearinterpolate(x, -(*arg).interpolarea-1, (*arg).interpolarea+1, fftsino[binleft + anglecur], fftsino[binright + anglecur]) + 
          	                                    linearinterpolate(y, -(*arg).interpolarea-1, (*arg).interpolarea+1, fftsino[bincur + angletop], fftsino[bincur + anglebot]))/2.0;
          	    }
          	  }
            }
          }
          // Otherwise do nothing
        }
      }            
    } 

    // Clean up
    delete[] bigsino;

  /***********************************************************************
   * Trunc data
   ***********************************************************************/
   
    // Put data back in sinogram
    for (int bin = 0; bin < nelems; bin++)
    {
      for (int angle = 0; angle < nangles; angle++)
      {
        int anglecur  = angle*nelems;
                   
      	if (((*arg).norm[bin + anglecur + planeplanearraysize] == 0))
      	{ 
          (*arg).sinogram[bin + anglecur + planeplanearraysize] = fftsino[bin + (angle + angleoffset)*nelems];
      	}
      }
    }
        
    // clean up before starting new loop
    delete[] fftsino;
    delete[] normsino;
  }
  
  delete[] mask;
  
  _endthreadex(0);

  return 0;
}

int confos2D_thread (float* sinogram, float* norm, int extention, int niter, int nelems, int nangles, int nplanes, int interpolarea, bool verboseMode, int nthreads)
{
  int i;
  HANDLE* thread;
  unsigned tid;
  int nplanestoprocess = nplanes/nthreads;
  int remainder = nplanes%nthreads;
  ConFoSp* processes;

  // Create array
  if((thread = new HANDLE[nthreads]) == 0) 
  {
    cerr << "Error: memory allocation failed for size " << nthreads*sizeof(HANDLE) << ", aborting" << endl;
    // Clean up
    return 1;
  }

  // Create array
  if((processes = new ConFoSp[nthreads]) == 0) 
  {
    cerr << "Error: memory allocation failed for size " << nthreads*sizeof(ConFoSp) << ", aborting" << endl;
    // Clean up
    return 1;
  }
   
  // Fill filter structures 
  for(i = 0; i < nthreads; i++)
  {
    processes[i].sinogram = sinogram; 
    processes[i].norm = norm;   
    processes[i].nelems = nelems;
    processes[i].nangles = nangles;
    processes[i].planestart = i*nplanestoprocess;
    if (i == nthreads-1)
    {
      processes[i].nplanestoprocess = nplanestoprocess + remainder;
    }
    else
    {
      processes[i].nplanestoprocess = nplanestoprocess;
    }
    processes[i].niter = niter;
    processes[i].extention = extention;
    processes[i].interpolarea = interpolarea;
    processes[i].nanglesbig = nelems*2;
  }

  for (i = 0; i < nthreads; i++)
  {
    thread[i] = (HANDLE) _beginthreadex(NULL, 0, &confosp, &(processes[i]), 0, (unsigned *) &tid);
  }

  WaitForMultipleObjects(nthreads, thread, TRUE, INFINITE);
  
  // Rememeber to destroy Mutex!
  
  delete[] thread;
  delete[] processes;
    
  return 0;
}

int confos3D_thread (float* sinogram, float* norm, int extention, int niter, int nelems, int nangles, int* segplanes, int nsegs, int interpolarea, bool verboseMode, int nthreads)
{
  int i, seg;
  HANDLE* thread;
  unsigned tid;
  int nplanestoprocess;
  int offset = 0;
  int remainder;
  ConFoSp* processes;

  // Create array
  if((thread = new HANDLE[nthreads]) == 0) 
  {
    cerr << "Error: memory allocation failed for size " << nthreads*sizeof(HANDLE) << ", aborting" << endl;
    // Clean up
    return 1;
  }

  // Create array
  if((processes = new ConFoSp[nthreads]) == 0) 
  {
    cerr << "Error: memory allocation failed for size " << nthreads*sizeof(ConFoSp) << ", aborting" << endl;
    // Clean up
    return 1;
  }

  // Do for each segment
  for (seg = 0; seg < nsegs; seg++)
  { 
    nplanestoprocess = segplanes[seg]/nthreads;
    remainder = segplanes[seg]%nthreads;  	
  	
    // Fill filter structures 
    for(i = 0; i < nthreads; i++)
    {
      processes[i].sinogram = &(sinogram[offset]);
      processes[i].norm = &(norm[offset]);   
      processes[i].nelems = nelems;
      processes[i].nangles = nangles;
      processes[i].planestart = i*nplanestoprocess;
      if (i == nthreads-1)
      {
        processes[i].nplanestoprocess = nplanestoprocess + remainder;
      }
      else
      {
        processes[i].nplanestoprocess = nplanestoprocess;
      }
      processes[i].niter = niter;
      processes[i].extention = extention;
      processes[i].interpolarea = interpolarea;
      processes[i].nanglesbig = 2*nelems;      
    }

    for (i = 0; i < nthreads; i++)
    {
      thread[i] = (HANDLE) _beginthreadex(NULL, 0, &confosp, &(processes[i]), 0, (unsigned *) &tid);
    }

    WaitForMultipleObjects(nthreads, thread, TRUE, INFINITE);
    
    offset += segplanes[seg]*nelems*nangles;
  }
  
  // Rememeber to destroy Mutex!
  
  delete[] thread;
  delete[] processes;
    
  return 0;
}
