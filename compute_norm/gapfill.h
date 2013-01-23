//** file: gapfill.h
//** date: 2002/11/04

//** author: Ziad Burbar
//** © CPS Innovations

//** This class implements the gap filling algorithm for HRRT scanners.

/*- usage examples ------------------------------------------------------------

  GapFill *gf;

  gf=new GapFill(RhoSamples, ThetaSamples, threshold);
  for (p=0; p < planes; p++)
   gf->calcGAPfill(&norm[p*plane_size], &sino[p*plane_size]);
  delete gf;

-----------------------------------------------------------------------------*/

#ifndef _GAPFILL_H
#define _GAPFILL_H

/*- class definitions -------------------------------------------------------*/

class GapFill  
 { private:
    unsigned short int RhoSamples,             // number of bins in projections
                       ThetaSamples;           // number of angles in sinograms
    unsigned long int *m_index,
                      planesize;
    float threshold;
    void calc_indices() const;
    void smooth(float * const, const unsigned long int,
                unsigned long int) const;
   public:
    GapFill(const unsigned short int, const unsigned short int, const float);
    ~GapFill();
    void calcGapFill(const float * const, float * const) const;
    void calcRandomsSmooth(float * const) const;
 };

#endif

