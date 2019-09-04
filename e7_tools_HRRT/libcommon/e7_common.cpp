/*! \file e7_common.cpp
    \brief This module provides some common functions which are used in the
           e7-Tools.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/02 added Doxygen style comments
    \date 2004/10/09 added gantryModel() function
    \date 2004/11/19 added fireCOMEvent() function
    \date 2005/02/08 added deleteDirectory() function
    \date 2005/02/11 added deleteFiles() function
    \date 2005/02/12 integrate error group in fireCOMEvent()

    This module provides some common functions which are used in the e7-Tools.
 */

#include <cstdlib>
#include <fstream>
#include <dirent.h>
#include <sys/utsname.h>

#include <sys/types.h>
#include <ctime>
#include "e7_common.h"
#include "ecat7_global.h"
#include "exception.h"
#include "image_io.h"
#include "interfile.h"
#include "logging.h"
#include "raw_io.h"
#include "sinogram_io.h"
#include "str_tmpl.h"

/*---------------------------------------------------------------------------*/
/*! \brief Get bed position in mm for a given ECAT7 matrix.
    \param[in] e7    ECAT7 file
    \param[in] mnr   matrix number
    \return bed position in mm

    Get bed position in mm for a given ECAT7 matrix.
 */
/*---------------------------------------------------------------------------*/
float bedPosition(ECAT7 * const e7, const unsigned short int mnr)
 { float pos;
   unsigned short int bed;

   bed=e7->Dir_bed(mnr);
   pos=e7->Main_init_bed_position()*10.0f;
   if (bed > 0) return(pos+e7->Main_bed_position(bed-1)*10.0f);
   return(pos);
 }


/*---------------------------------------------------------------------------*/
/*! \brief Calculate decay and frame-length correction factor.
    \param[in]  fs              frame start time in seconds
    \param[in]  fd              frame duration in seconds
    \param[in]  gd              gate duration in seconds
    \param[in]  halflife        halflife of isotope in seconds
    \param[in]  decay_corr      calculate decay correction factor ?
    \param[in]  framelen_corr   calculate frame-length correction factor ?
    \param[out] scale_factor    scaling factor for decay and frame-length
                                correction
    \param[in]  loglevel        level for logging
    \return decay correction factor

    Calculate the decay correction factor if the \em decay_corr parameter is
    set:
    \f[
       \lambda=\frac{ln(2)}{\mbox{halflife}}
    \f]
    \f[
       \mbox{decay\_factor}=\frac{\exp(\lambda\cdot\mbox{fs})\cdot\mbox{fd}
                                  \cdot\lambda}{1-\exp(-\lambda\cdot\mbox{fd})}
    \f]
    Calculate the frame-length correction factor if the \em framelen_corr
    parameter is set:
    \f[
       \mbox{fl\_factor}=\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{1}{\mbox{gd}} & \mbox{gd}>0 \wedge \mbox{gd}\le\mbox{fd} \\
         \frac{1}{\mbox{fd}} & \mbox{gd}=0 \vee \mbox{gd}>\mbox{fd}
        \end{array}\right.
    \f]
    The scale factor is calculated by:
    \f[
        \mbox{scale\_factor}=\mbox{decay\_factor}\cdot\mbox{fl\_factor}
    \f]
 */
/*---------------------------------------------------------------------------*/
float calcDecayFactor(const float fs, const float fd, const float gd,
                      const float halflife, const bool decay_corr,
                      const bool framelen_corr, float * const scale_factor,
                      const unsigned short int loglevel)
 { float decay_factor;
   std::string str;

   str="calculate ";
   if (decay_corr) { str+="decay ";
                     if (framelen_corr) str+="and ";
                   }
   if (framelen_corr) str+="frame-length ";
   Logging::flog()->logMsg(str+"correction", loglevel);
                                           // calculate decay correction factor
   if ((halflife > 0.0f) && decay_corr)
    { double lambda, divisor;

      lambda=logf(2.0f)/halflife;                             // decay constant
      divisor=1.0f-expf(-lambda*fd);
      if (divisor != 0.0) decay_factor=expf(lambda*fs)*fd*lambda/divisor;
       else decay_factor=1.0f;
    }
    else decay_factor=1.0f;
                                           // calculate frame-length correction
   if (framelen_corr)
    if ((gd > 0) && (gd <= fd)) *scale_factor=decay_factor/gd;
     else *scale_factor=decay_factor/fd;
    else *scale_factor=decay_factor;
   return(decay_factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate for each plane of a sinogram, which ring-pairs are
           contributing to this plane.
    \param[in] rings   number of crystal rings in gantry
    \param[in] span    span of sinogram
    \param[in] mrd     maximum ring difference
    \return array of ring pairs for all planes

    Calculate for each plane of a sinogram, which ring-pairs are contributing
    to this plane.
 */
/*---------------------------------------------------------------------------*/
std::vector <std::vector<unsigned short int> > calcRingNumbers(
                                          const unsigned short int rings,
                                          const unsigned short int span,
                                          const unsigned short int mrd)
 { unsigned short int *zsize=NULL, *zindex=NULL;

   try
   { unsigned short int dmax, nmax;
     std::vector <std::vector<unsigned short int> > ringnums;

     dmax=span/2+1;           // maximum number of cross planes in a projection
     nmax=(mrd+1)/span;                     // maximum angular projection index

     { unsigned short int as;

       as=2*rings-1;
       for (unsigned short int axis=1; axis < (mrd-(span-1)/2)/span+1; axis++)
        as+=2*(2*rings-1-2*(axis*span-(span-1)/2));
       ringnums.resize(as);
     }
                // create array for z-offset of the beginning of the projection
     zsize=new unsigned short int[nmax+1];
      // create array for the index of the beginning of the positive projection
     zindex=new unsigned short int[nmax+1];
     zsize[0]=0;
     zindex[0]=0;
     zsize[1]=dmax;
     zindex[1]=rings*2-1;
                             // calculate plane offsets at the beginning of the
                             // projections and the size of the projections
     for (unsigned short int i=2; i <= nmax; i++)
      { zindex[i]=zindex[i-1]+2*(rings*2-1-2*zsize[i-1]);
        zsize[i]=(2*i-1)*dmax-i+1;
      }
                  // calculate and store plane number for each pair of crystals
     for (unsigned short int r1=0; r1 < rings; r1++)
      for (unsigned short int r2=0; r2 < rings; r2++)
       { unsigned short int nabs;

         nabs=(abs(r1-r2)+dmax-1)/span;
         if (nabs <= nmax)
          { signed short int plane;

            plane=(r1+r2)-zsize[nabs]+zindex[nabs];
            if ((r1-r2 < 0) && (nabs > 0)) plane+=rings*2-1-2*zsize[nabs];
            ringnums[plane].push_back(r1);
            ringnums[plane].push_back(r2);
          }
       }

     delete[] zindex;
     zindex=NULL;
     delete[] zsize;
     zsize=NULL;
     return(ringnums);
   }
   catch (...)
    { if (zsize != NULL) delete[] zsize;
      if (zindex != NULL) delete[] zindex;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check filename extension and cut off.
    \param[in,out] str   filename
    \param[in]     s     extension
    \return cut off ?
 
    Check filename extension and cut off.
 */
/*---------------------------------------------------------------------------*/
bool checkExtension(std::string * const str, const std::string s)
 { if (str->length() < s.length()) return(false);
   if (str->substr(str->length()-s.length(), s.length()) == s)
    { *str=str->substr(0, str->length()-s.length());
      return(true);
    }
   return(false);
 }


/*---------------------------------------------------------------------------*/
/*! \brief Check if a file exists.
    \param[in] filename   name of file
    \return does file exist ?

    Check if a file exists.
 */
/*---------------------------------------------------------------------------*/
bool FileExist(const std::string filename)
 { std::ifstream *file=NULL;

   try
   { bool ret=true;

     file=new std::ifstream(filename.c_str());
     if (!*file) ret=false;
     delete file;
     file=NULL;
     return(ret);
   }
   catch (...)
    { if (file != NULL) delete file;
      return(false);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Find a matrix number in an ECAT7 file or Interfile for a given bed
           position.
    \param[in]  filename   name of ECAT7 file
    \param[in]  transmission_only   search a transsmision frame
    \param[in]  bed_pos             bed position in mm
    \param[out] found_bed_pos       position of the found matrix in mm
    \return number of the matrix
    \exception REC_BED_NOT_FOUND bed not found in Interfile

    Find a matrix number in an ECAT7 file or Interfile for a given bed
    position.
 */
/*---------------------------------------------------------------------------*/
unsigned short int findMNR(const std::string filename,
                           const bool transmission_only, const float bed_pos,
                           float * const found_bed_pos)
 { bool ecat7_format;

   ecat7_format=isECAT7file(filename);
   if (!ecat7_format && !isInterfile(filename))
    { *found_bed_pos=0.0f;
      return(0);
    }
   ECAT7 *e7=NULL;
   Interfile *interf=NULL;

   try
   { unsigned short int a_mnr=0;
     float min_diff=std::numeric_limits <float>::max();

     if (ecat7_format)
      { e7=new ECAT7();
        e7->LoadFile(filename);
                            // search bed that is closest to given bed position
        for (unsigned short int i=0; i < e7->NumberOfMatrices(); i++)
         { float diff;

           diff=fabsf(bedPosition(e7, i)-bed_pos);
           if (diff < min_diff) { min_diff=diff;
                                  a_mnr=i;
                                }
         }
        *found_bed_pos=bedPosition(e7, a_mnr);
        delete e7;
        e7=NULL;
      }
      else { *found_bed_pos=0.0f;
             interf=new Interfile();
             interf->loadMainHeader(filename);
                            // search bed that is closest to given bed position
             for (unsigned short int i=0;
                  i < interf->Main_number_of_horizontal_bed_offsets();
                  i++)
              { Interfile::tdataset ds;
                float diff;
                bool found_bed=false;
                std::string unit;

                for (unsigned short int j=0;
                     j < interf->Main_total_number_of_data_sets();
                     j++)
                 { unsigned short int bed;
                   Interfile::tscan_type sc;

                   ds=interf->Main_data_set(j);
                   interf->acquisitionCode(ds.acquisition_number, &sc, NULL,
                                            NULL, &bed, NULL, NULL, NULL);
                   if ((bed == i) &&
                       (!transmission_only ||
                        (sc == Interfile::TRANSMISSION_TYPE)))
                    { interf->loadSubheader(*(ds.headerfile));
                      found_bed=true;
                      break;
                    }
                 }
                if (!found_bed)
                 throw Exception(REC_BED_NOT_FOUND,
                                 "The main header file '#1' doesn't contain a "
                                 "link to bed #2.").arg(filename).arg(i);
                diff=fabsf(interf->Sub_start_horizontal_bed_position(&unit)-
                           bed_pos);
                if (diff < min_diff)
                 { min_diff=diff;
                   *found_bed_pos=interf->Sub_start_horizontal_bed_position(
                                                                        &unit);
                   a_mnr=i;
                 }
              }
             delete interf;
             interf=NULL;
           }
     return(a_mnr);
   }
   catch (...)
    { if (e7 != NULL) delete e7;
      if (interf != NULL) delete interf;
      throw;
    }
 }


/*---------------------------------------------------------------------------*/
/*! \brief Request the gantry model number from a file.
    \param[in] filename   name of file
    \return gantry model number

    Request the gantry model number from a file. This is derived from the ECAT7
    or Interfile header. For flat files the return value is 0.
 */
/*---------------------------------------------------------------------------*/
unsigned short int gantryModel(const std::string filename)
 { ECAT7 *e7=NULL;
   Interfile *inf=NULL;

   try
   { if (isECAT7file(filename))
      { unsigned short int num;

        e7=new ECAT7();
        e7->LoadFile(filename);
        num=e7->Main_system_type();
        delete e7;
        e7=NULL;
        return(num);
      }
     if (isInterfile(filename))
      { unsigned short int num;

        inf=new Interfile();
        inf->loadMainHeader(filename);
        num=atoi(inf->Main_originating_system().c_str());
        delete inf;
        inf=NULL;
        return(num);
      }
     return(0);
   }
   catch (...)
    { if (e7 != NULL) delete e7;
      if (inf != NULL) delete inf;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check if file has ECAT7 format or doesn't exist.
    \param[in] filename   name of file
    \return is file in ECAT7 format or doesn't file exist ?

    Check if file has ECAT7 format or doesn't exist. A file is assumed to be in
    ECAT7 format if the first 7 bytes of the file are "MATRIX7".
 */
/*---------------------------------------------------------------------------*/
bool isECAT7file(const std::string filename)
 { RawIO <unsigned char> *rio=NULL;

   try
   { unsigned char buffer[7];

     Logging::flog()->loggingOn(false);
     rio=new RawIO <unsigned char>(filename, false, false);
     Logging::flog()->loggingOn(true);
     rio->read(buffer, 7);
     delete rio;
     rio=NULL;
     return(strncmp((char *)buffer, "MATRIX7", 7) == 0);
   }
   catch (...)
    { Logging::flog()->loggingOn(true);
      if (rio != NULL) delete rio;
      return(true);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check if file has Interfile format or doesn't exist.
    \param[in] filename   name of file
    \return is file in Interfile format or doesn't file exist ?

    Check if file has Interfile format or doesn't exist. A file is assumed to
    be in Interfile format if the first 10 bytes of the file are "!INTERFILE".
 */
/*---------------------------------------------------------------------------*/
bool isInterfile(const std::string filename)
 { RawIO <unsigned char> *rio=NULL;

   try
   { unsigned char buffer[10];

     Logging::flog()->loggingOn(false);
     rio=new RawIO <unsigned char>(filename, false, false);
     Logging::flog()->loggingOn(true);
     rio->read(buffer, 10);
     delete rio;
     rio=NULL;
     return(strncmp((char *)buffer, "!INTERFILE", 10) == 0);
   }
   catch (...)
    { Logging::flog()->loggingOn(true);
      if (rio != NULL) delete rio;
      return(true);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the number of matrices in a file.
    \param[in] filename   name of file
    \return number of matrices in file

    Request the number of matrices in a file. This is derived from the ECAT7 or
    Interfile header. Flat files are assumed to contain only 1 matrix.
 */
/*---------------------------------------------------------------------------*/
unsigned short int numberOfMatrices(const std::string filename)
 { ECAT7 *e7=NULL;
   Interfile *inf=NULL;

   try
   { if (isECAT7file(filename))
      { unsigned short int num;

        e7=new ECAT7();
        e7->LoadFile(filename);
        num=e7->NumberOfMatrices();
        delete e7;
        e7=NULL;
        return(num);
      }
     if (isInterfile(filename))
      { unsigned short int num;

        inf=new Interfile();
        inf->loadMainHeader(filename);
        num=std::max(inf->Main_number_of_time_frames(),
                     inf->Main_number_of_horizontal_bed_offsets());
        delete inf;
        inf=NULL;
        return(num);
      }
     return(1);
   }
   catch (...)
    { if (e7 != NULL) delete e7;
      if (inf != NULL) delete inf;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check if path exists
    \param[in] path   name of path
    \return does path exist ?

    Check if path exists.
 */
/*---------------------------------------------------------------------------*/
bool PathExist(const std::string path) {
   return(opendir(path.c_str()) != NULL);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert number of seconds into string with hours,minutes and
           seconds.
    \param[in] seconds   number of seconds
    \return string in format "00:00:00 (hh:mm:ss)"

    Convert number of seconds into string with hours,minutes and seconds.
 */
/*---------------------------------------------------------------------------*/
std::string secStr(float seconds)
 { std::string str, s;

   str=toStringZero((unsigned long int)(seconds/3600.0f), 2);
   seconds-=3600.0f*(unsigned long int)(seconds/3600.0f);
   str+=":"+toStringZero((unsigned short int)(seconds/60.0f), 2);
   seconds-=60.0f*(unsigned short int)(seconds/60.0f);
   s=toString(seconds);
   while (s.length() < 2) s="0"+s;
   while ((s[0] == '.') || (s[1] == '.')) s="0"+s;
   return(str+":"+s+" (hh:mm:ss)");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert internal number of segment into normal segment number.
    \param[in] segment   number of segment
    \return string with segment number

    Convert internal number of segment into normal segment number. The internal
    segment numbers are 0,1,2,3,4,5,... while normal segment numbers are 0,+1,
    -1,+2,-2,+3,...
 */
/*---------------------------------------------------------------------------*/
std::string segStr(const unsigned short int segment)
 { if (segment == 0) return("0");
   if (segment & 0x1) return("+"+toString((segment+1)/2));
   return("-"+toString((segment+1)/2));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert STL string into COM BSTR.
    \param[in] str   STL string
    \return COM BSTR

    Convert STL string into COM BSTR. The COM BSTR is allocated.
 */
/*---------------------------------------------------------------------------*/
std::string string2BSTR(std::string str) { 
  return(str);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove path from filename.
    \param[in] str   filename
    \return filename without path

    Remove path from filename.
 */
/*---------------------------------------------------------------------------*/
std::string stripPath(std::string str)
 { std::string::size_type p;

   if ((p=str.rfind("/")) == std::string::npos) p=str.rfind("\\");
   if (p == std::string::npos) return(str);
   return(str.substr(p+1));
 }
