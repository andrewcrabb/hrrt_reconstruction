/*! \class Interfile interfile.h "interfile.h"
    \brief This class is used to read, write, create and modify Interfile
           headers.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)

    \date 2003/11/17 initial version
    \date 2004/06/02 added Doxygen style comments
    \date 2004/12/14 added main header keys for normalization code
    \date 2004/12/14 changed unit of tracer activity from MBq to mCi
    \date 2004/12/14 return unit of value when retrieving value
    \date 2004/12/14 changed unit of UTC keys to "sec"
    \date 2005/01/15 cleanup templates
    \date 2005/02/08 support negative number of trues
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class contains code to read, write, create and modify Interfile
    headers and perform syntax and sematic checks. It uses two tables for every
    header:
    one parser table of type std::vector <ttoken *> which stores all keys that
    are allowed in the header, with their types, units and if they require a
    value and one list of KV objects (type std::vector <KV *>) that stores all
    the information found in a given interfile header. The code to get and set
    values of keys is generated from tables at compile time by the
    preprocessor.

    The parameters of the methods ...KeyBeforeKey and ...KeyAfterKey initiate a
    search for two keys. Examples for this are:

     inter->MainKeyBeforeKey(inter->Main_originating_system(),
                             inter->Main_number_of_frames());

    or

     inter->MainKeyBeforeKey(inter->Main_originating_system(395),
                             inter->Main_number_of_frames(7));

    The object stores the last two keys which where used in such search
    operations in the array "last_iterator". These two keys are used to perform
    the task of the method. It is a somewhat "dirty trick", to use such
    side-effect information but it allows the user to insert keys somewhere in
    the list of key/values without increasing the size of this objects
    interface too much.
    It is even possible to insert blank lines into the file by:

     inter->MainKeyBeforeKey(inter->Main_blank_line("this line is empty"),
                             inter->Main_number_of_frames());

    although blank lines don't have a key to search for.

    The method has to be used in the described ways, even if the compiler
    allows different usage, e.g:

     inter->MainKeyBeforeKey(1, 2);

    This would result in "undefined" behaviour. The method would use the key
    information from some previous search operation or would throw an
    exception.
    This method also depends on the evaluation order of the function parameters
    which is undefined in C++. The method evalutationOrder() in the constructor
    detects the evaluation order used by the current compiler. This might not
    work with all compilers.

    Another way to implement this method would be to force the user to specify
    the keys directly, like

     interf->MainKeyBeforeKey("!originating system", "number of time frames");

    This would move a part of the "syntactical responsibility" to the user of
    the library. Also if a key is changed, all software using the library
    would have to change too. Therefore it is a deliberate design decision to
    hide the key strings inside of the library.
 */

/*! \example example_itf.cpp
    Here are some examples of how to use the Interfile class.
 */

#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#ifndef _INTERFILE_CPP
#define _INTERFILE_CPP
#include "interfile.h"
#endif
#include "exception.h"
#include <string.h>

/*- constants ---------------------------------------------------------------*/
#ifndef _INTERFILE_TMPL_CPP
                                /*! extension of interfile main header files */
const std::string Interfile::INTERFILE_MAINHDR_EXTENSION=".mhdr";
                /*! extension of interfile emission sinogram subheader files */
const std::string Interfile::INTERFILE_SSUBHDR_EXTENSION=".s.hdr";
                          /*! extension of interfile emission sinogram files */
const std::string Interfile::INTERFILE_SDATA_EXTENSION=".s";
            /*! extension of interfile transmission sinogram subheader files */
const std::string Interfile::INTERFILE_ASUBHDR_EXTENSION=".a.hdr";
                      /*! extension of interfile transmission sinogram files */
const std::string Interfile::INTERFILE_ADATA_EXTENSION=".a";
                       /*! extension of interfile normalization header files */
const std::string Interfile::INTERFILE_NHDR_EXTENSION=".nhdr";
                         /*! extension of interfile normalization data files */
const std::string Interfile::INTERFILE_NDATA_EXTENSION=".n";
                            /*! extension of interfile image subheader files */
const std::string Interfile::INTERFILE_IHDR_EXTENSION=".v.hdr";
                                 /*! extension of interfile image data files */
const std::string Interfile::INTERFILE_IDATA_EXTENSION=".v";

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
Interfile::Interfile()
 { smh.clear();
   kv_smh.clear();
   ssh.clear();
   kv_ssh.clear();
   nh.clear();
   kv_nh.clear();
   main_headerfile=std::string();
   subheaderfile=std::string();
   norm_headerfile=std::string();
   last_pos[0]=std::numeric_limits <unsigned short int>::max();
   last_pos[1]=std::numeric_limits <unsigned short int>::max();
   initMain();                           // create parser table for main header
   initSub();                              // create parser table for subheader
   initNorm();                           // create parser table for norm header
                                    // determine evaluation order of parameters
   fpf=evaluationOrder(eo_fct(1), eo_fct(2));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
Interfile::~Interfile()
 { deleteKVList(&kv_smh);               // delete key/value list of main header
   deleteParserTable(&smh);              // delete parser table for main header
   deleteKVList(&kv_ssh);                 // delete key/value list of subheader
   deleteParserTable(&ssh);                // delete parser table for subheader
   deleteKVList(&kv_nh);                // delete key/value list of norm header
   deleteParserTable(&nh);               // delete parser table for norm header
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] inf   object to copy
    \return copy of object

    Copy operator.
 */
/*---------------------------------------------------------------------------*/
Interfile& Interfile::operator = (const Interfile &inf)
 { if (this != &inf)
    { std::vector <KV *>::const_iterator it;
      const std::vector <KV *> *ikv_smh=&(inf.kv_smh);
      const std::vector <KV *> *ikv_ssh=&(inf.kv_ssh);
      const std::vector <KV *> *ikv_nh=&(inf.kv_nh);
                                                            // copy main header
      deleteKVList(&kv_smh);
      for (it=ikv_smh->begin(); it != ikv_smh->end(); ++it)
       { KV *kv;

         kv=new KV(std::string());
         *kv=*(*it);
         kv_smh.push_back(kv);
       }
                                                              // copy subheader
      deleteKVList(&kv_ssh);
      for (it=ikv_ssh->begin(); it != ikv_ssh->end(); ++it)
       { KV *kv;

         kv=new KV(std::string());
         *kv=*(*it);
         kv_ssh.push_back(kv);
       }
                                                            // copy norm header
      deleteKVList(&kv_nh);
      for (it=ikv_nh->begin(); it != ikv_nh->end(); ++it)
       { KV *kv;

         kv=new KV(std::string());
         *kv=*(*it);
         kv_nh.push_back(kv);
       }
      main_headerfile=inf.main_headerfile;
      main_headerpath=inf.main_headerpath;
      subheaderfile=inf.subheaderfile;
      subheaderpath=inf.subheaderpath;
      norm_headerfile=inf.norm_headerfile;
      norm_headerpath=inf.norm_headerpath;
      memcpy(last_pos, inf.last_pos, 2*sizeof(unsigned short int));
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate acquisition code.
    \param[in] scan_type   type of scan (emission/transmission)
    \param[in] frame       number of frame (0-99)
    \param[in] gate        number of gate (0-99)
    \param[in] bed         number of bed (0-99)
    \param[in] energy      number of energy window (0-2)
    \param[in] sino_mode   sinogram mode (trues/prompts/randoms)
    \param[in] tof         number of time-of-flight window
    \return acquisition code
    \exception REC_OOR_FRAME  frame out of range
    \exception REC_OOR_GATE   gate out of range
    \exception REC_OOR_BED    bed out of range
    \exception REC_OOR_ENERGY energy window out of range
    \exception REC_OOR_TOF    time-of-flight window out of range

    Calculate acquisition code:
    \f[
        \mbox{scan\_type}\cdot 10^9+\mbox{frame}\cdot 10^7+
        \mbox{gate}\cdot 10^5+\mbox{bed}\cdot 10^3+\mbox{tof}\cdot 10+
        \mbox{energy}\cdot 3+\mbox{sino\_mode}
    \f]
 */
/*---------------------------------------------------------------------------*/
unsigned long int Interfile::acquisitionCode(const tscan_type scan_type,
                                            const unsigned short int frame,
                                            const unsigned short int gate,
                                            const unsigned short int bed,
                                            const unsigned short int energy,
                                            const tsino_mode sino_mode,
                                            const unsigned short int tof) const
 { unsigned short int st, sm=0;

   if (scan_type == EMISSION_TYPE) st=0;
    else st=1;
   if (sino_mode == TRUES_MODE) sm=0;
    else if (sino_mode == PROMPTS_MODE) sm=1;
    else if (sino_mode == RANDOMS_MODE) sm=2;
   if (frame > 99)
    throw Exception(REC_OOR_FRAME,
                    "Number of frame in acuisition code out of range.");
   if (gate > 99)
    throw Exception(REC_OOR_GATE,
                    "Number of gate in acquisition code out of range.");
   if (bed > 99)
    throw Exception(REC_OOR_BED,
                    "Number of bed in acquisition code out of range.");
   if (energy > 2)
    throw Exception(REC_OOR_ENERGY,
                  "Number of energy window in acquisition code out of range.");
   if (tof > 99)
    throw Exception(REC_OOR_TOF,
                    "Number of time-of-flight window in acquisition code of "
                    "ouf range.");
   return((unsigned long int)st*1000000000ul+
          (unsigned long int)frame*10000000ul+
          (unsigned long int)gate*100000ul+
          (unsigned long int)bed*1000ul+
          (unsigned long int)tof*10ul+
          (unsigned long int)energy*3ul+sm);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Decode acquisition code.
    \param[in]  ac          acquisition code
    \param[out] scan_type   type of scan (emission/transmission)
    \param[out] frame       number of frame (0-99)
    \param[out] gate        number of gate (0-99)
    \param[out] bed         number of bed (0-99)
    \param[out] energy      number of energy window (0-2)
    \param[out] sino_mode   sinogram mode (trues/prompts/randoms)
    \param[out] tof         number of time-of-flight window
    \exception REC_OOR_SCAN_TYPE scan type unknown
    \exception REC_OOR_FRAME  frame out of range
    \exception REC_OOR_GATE   gate out of range
    \exception REC_OOR_BED    bed out of range
    \exception REC_OOR_ENERGY energy window out of range
    \exception REC_OOR_TOF    time-of-flight window out of range

    Decode acquisition code.
*/
/*---------------------------------------------------------------------------*/
void Interfile::acquisitionCode(unsigned long int ac,
                                tscan_type * const scan_type,
                                unsigned short int * const frame,
                                unsigned short int * const gate,
                                unsigned short int * const bed,
                                unsigned short int * const energy,
                                tsino_mode * const sino_mode,
                                unsigned short int * const tof) const
 { unsigned long int v;

   if (ac >= 1000000000ul)
    { if (scan_type != NULL)
      if (ac >= 2000000000ul)
       throw Exception(REC_OOR_SCAN_TYPE,
                       "Unknown scan type in acquisition code.");
       else *scan_type=TRANSMISSION_TYPE;
      ac-=1000000000ul;
    }
    else if (scan_type != NULL) *scan_type=EMISSION_TYPE;
   v=ac/10000000ul;
   if (frame != NULL)
    { *frame=(unsigned short int)v;
      if (*frame > 99)
       throw Exception(REC_OOR_FRAME,
                       "Number of frame in acquisition code out of range.");
    }
   ac-=v*10000000ul;
   v=ac/100000ul;
   if (gate != NULL)
    { *gate=(unsigned short int)v;
      if (*gate > 99)
       throw Exception(REC_OOR_GATE,
                       "Number of gate in acquisition code out of range.");
    }
   ac-=v*100000ul;
   v=ac/1000ul;
   if (bed != NULL)
    { *bed=(unsigned short int)v;
      if (*bed > 99)
       throw Exception(REC_OOR_BED,
                       "Number of bed in acquisition code out of range.");
    }
   ac-=v*1000ul;
   v=ac/10ul;
   if (tof != NULL)
    { *tof=(unsigned short int)v;
      if (*tof > 99)
       throw Exception(REC_OOR_TOF,
                       "Number of time-of-flight window in acquisition code of"
                       " ouf range.");
    }
   ac-=v*10ul;
   v=ac/3ul;
   if (energy != NULL)
    { *energy=(unsigned short int)v;
      if (*energy > 2)
       throw Exception(REC_OOR_ENERGY,
                       "Number of energy window in acquisition code out of "
                       "range.");
    }
   ac-=v*3ul;
   if (sino_mode != NULL)
    if (ac == 0) *sino_mode=TRUES_MODE;
     else if (ac == 1) *sino_mode=PROMPTS_MODE;
     else if (ac == 2) *sino_mode=RANDOMS_MODE;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add blank line to key/value list.
    \param[in]     comment   comment for line
    \param[in,out] kv        key/value list

    Add blank line to key/value list.
 */
/*---------------------------------------------------------------------------*/
void Interfile::blankLine(const std::string comment, std::vector <KV *> *kv)
 { kv->push_back(new KV(comment));
   last_pos[0]=last_pos[1];
   last_pos[1]=(unsigned short int)kv->size();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check if unit of key is valid.
    \param[in] pt          parser table
    \param[in] key         name of key
    \param[in] short_key   key without index
    \param[in] unit        unit of key
    \param[in] filename    name of Interfile header
    \exception REC_ITF_HEADER_ERROR_UNIT_WRONG  unit is wrong
    \exception REC_ITF_HEADER_ERROR_KEY_INVALID key is invalid

    Check if unit of key is valid.
 */
/*---------------------------------------------------------------------------*/
void Interfile::checkUnit(std::vector <ttoken *> pt, const std::string key,
                          std::string short_key, std::string unit,
                          const std::string filename) const
 { std::vector <ttoken *>::const_iterator it;

   std::transform(short_key.begin(), short_key.end(), short_key.begin(),
                  tolower);
   std::transform(unit.begin(), unit.end(), unit.begin(), tolower);
   for (it=pt.begin(); it != pt.end(); ++it)      // search key in parser table
    { std::string k;

      k=(*it)->key;
      std::transform(k.begin(), k.end(), k.begin(), tolower);
      if (short_key == k)
       if (!unit.empty())
        { std::vector <std::string>::const_iterator it2;
          std::string unit_lc;

          unit_lc=unit;
          std::transform(unit_lc.begin(), unit_lc.end(), unit_lc.begin(),
                         tolower);
                                  // compare with all allowed units of this key
          for (it2=(*it)->units.begin(); it2 != (*it)->units.end(); ++it2)
           { std::string u;

             u=(*it2);
             std::transform(u.begin(), u.end(), u.begin(), tolower);
             if (u == unit_lc) return;
           }
          throw Exception(REC_ITF_HEADER_ERROR_UNIT_WRONG,
                          "Parsing error in the file '#1'. The unit of the key"
                          " '#2' is wrong.").arg(filename).arg(key);
        }
        else if (!(*it)->units.empty())
              throw Exception(REC_ITF_HEADER_ERROR_UNIT_WRONG,
                              "Parsing error in the file '#1'. The unit of the"
                              " key '#2' is wrong.").arg(filename).arg(key);
              else return;
    }
   throw Exception(REC_ITF_HEADER_ERROR_KEY_INVALID,
                   "Parsing error in the file '#1'. The key '#2' is invalid"
                   ".").arg(filename).arg(key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove all non image main header token.

    Remove all non image main header token. This method can be used to convert
    a sinogram main header into an image main header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::convertToImageMainHeader()
 { std::vector <KV *>::iterator it_kv;
   bool found;

   do
   { found=false;
     for (it_kv=kv_smh.begin(); it_kv != kv_smh.end(); ++it_kv)
      { std::vector <ttoken *>::const_iterator it;

        for (it=smh.begin(); it != smh.end(); ++it)
         if ((*it_kv)->Key() == (*it)->key)
          { tused_where w;

            w=(*it)->where;
            if ((w != IH) && (w != SIH) && (w != ALL))
             { delete *it_kv;
               kv_smh.erase(it_kv);
               found=true;
               break;
             }
          }
        if (found) break;
      }
   } while (found);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove all non image subheader token.

    Remove all non image subheader token. This method can be used to convert a
    sinogram subheader into an image subheader.
 */
/*---------------------------------------------------------------------------*/
void Interfile::convertToImageSubheader()
 { std::vector <KV *>::iterator it_kv;
   bool found;

   do
   { found=false;
     for (it_kv=kv_ssh.begin(); it_kv != kv_ssh.end(); ++it_kv)
      { std::vector <ttoken *>::const_iterator it;

        for (it=ssh.begin(); it != ssh.end(); ++it)
         if ((*it_kv)->Key() == (*it)->key)
          { tused_where w;

            w=(*it)->where;
            if ((w != IH) && (w != SIH) && (w != ALL))
             { delete *it_kv;
               kv_ssh.erase(it_kv);
               found=true;
               break;
             }
          }
        if (found) break;
      }
   } while (found);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove all non siunogram main header token.

    Remove all non sinogram main header token. This method can be used to
    convert an image main header into a sinogram main header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::convertToSinoMainHeader()
 { std::vector <KV *>::iterator it_kv;
   bool found;

   do
   { found=false;
     for (it_kv=kv_smh.begin(); it_kv != kv_smh.end(); ++it_kv)
      { std::vector <ttoken *>::const_iterator it;

        for (it=smh.begin(); it != smh.end(); ++it)
         if ((*it_kv)->Key() == (*it)->key)
          { tused_where w;

            w=(*it)->where;
            if ((w != SH) && (w != SIH) && (w != ALL))
             { delete *it_kv;
               kv_smh.erase(it_kv);
               found=true;
               break;
             }
          }
        if (found) break;
      }
   } while (found);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove all non sinogram subheader token.

    Remove all non sinogram subheader token. This method can be used to
    convert an image subheader into a sinogram subheader.
 */
/*---------------------------------------------------------------------------*/
void Interfile::convertToSinoSubheader()
 { std::vector <KV *>::iterator it_kv;
   bool found;

   do
   { found=false;
     for (it_kv=kv_ssh.begin(); it_kv != kv_ssh.end(); ++it_kv)
      { std::vector <ttoken *>::const_iterator it;

        for (it=ssh.begin(); it != ssh.end(); ++it)
         if ((*it_kv)->Key() == (*it)->key)
          { tused_where w;

            w=(*it)->where;
            if ((w != SH) && (w != SIH) && (w != ALL))
             { delete *it_kv;
               kv_ssh.erase(it_kv);
               found=true;
               break;
             }
          }
        if (found) break;
      }
   } while (found);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create an empty main header.
    \param[in] filename   name of Interfile header

    Create an empty main header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::createMainHeader(const std::string filename)
 { deleteKVList(&kv_smh);
   main_headerfile=filename;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create an empty norm header.
    \param[in] filename   name of Interfile header

    Create an empty norm header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::createNormHeader(const std::string filename)
 { deleteKVList(&kv_nh);
   norm_headerfile=filename;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create an empty subheader.
    \param[in] filename   name of Interfile header

    Create an empty subheader.
 */
/*---------------------------------------------------------------------------*/
void Interfile::createSubheader(const std::string filename)
 { deleteKVList(&kv_ssh);
   subheaderfile=filename;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete key/value pair from list.
    \param[in,out] kv         key/value list
    \param[in]     kv_it      pointer to key/value pair to delete
    \param[in]     filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_KEY_MISSING key not found

    Delete key/value pair from list.
 */
/*---------------------------------------------------------------------------*/
void Interfile::deleteKV(std::vector <KV *> *kv, KV *kv_it,
                         const std::string filename)
 { for (std::vector <KV *>::iterator it=kv->begin(); it != kv->end(); ++it)
    if (*it == kv_it) { delete *it;
                        kv->erase(it);
                        return;
                      }
   throw Exception(REC_ITF_HEADER_ERROR_KEY_MISSING,
                   "Parsing error in the file '#1'. The key '#2' is missing"
                   ".").arg(filename).arg(kv_it->Key());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete key/value list.
    \param[in,out] kv   key/value list

    Delete key/value list.
 */
/*---------------------------------------------------------------------------*/
void Interfile::deleteKVList(std::vector <KV *> *kv)
 { std::vector <KV *>::iterator it_kv;

   while (kv->size() > 0)
    { it_kv=kv->begin();
      delete *it_kv;
      kv->erase(it_kv);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete parser table.
    \param[in,out] pt   parser table

    Delete parser table.
 */
/*---------------------------------------------------------------------------*/
void Interfile::deleteParserTable(std::vector <ttoken *> *pt)
 { std::vector <ttoken *>::iterator it;

   while (pt->size() > 0)
    { it=pt->begin();
      delete *it;
      pt->erase(it);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store the number of this function in a global variable.
    \param[in] num   number of function

    Store the number of this function in a global variable.
 */
/*---------------------------------------------------------------------------*/
unsigned short int Interfile::eo_fct(unsigned short int num)
 { eo=num;
   return(0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Determine evaluation order of parameters.
    \param[in] dummy   first parameter
    \param[in] dummy   second parameter
    \return first parameter is evaluated first

    Determine evaluation order of parameters. This method is used in
    combination with the eo_fct() method:

     \em evaluationOrder(eo_fct(1), eo_fct(2))
 */
/*---------------------------------------------------------------------------*/
bool Interfile::evaluationOrder(unsigned short int, unsigned short int) const
 { return(eo == 2);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Get value of key from header.
    \param[in] key          name of key
    \param[in] kv_hdr       key/value list of header
    \param[in] headerfile   name of Interfile header
    \param[in] headerpath   path of Interfile header
    \return value of key

    Get value of key from header.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
T Interfile::getValue(const std::string key, std::vector <KV *> kv_hdr,
                      const std::string headerfile,
                      const std::string headerpath)
 { T value;
   KV *key_value;

   key_value=searchKey(key, kv_hdr, headerfile);
   value=key_value->getValue<T>();
   if (key_value->Type() == KV::RELFILENAME)
    { std::string filename, fname;

      fname=KV::convertToString(value);
      if ((fname.length() >= 2) && (fname.at(0) != '/') &&
          (fname.substr(0, 2) != "\\\\") &&
          (fname.at(1) != ':')) filename=headerpath+fname;
       else filename=fname;
      return(*(T *)&filename);
    }
   return(value);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Get value of key from header.
    \param[in] key          name of key
    \param[in] a_idx        index of array
    \param[in] kv_hdr       key/value list of header
    \param[in] headerfile   name of Interfile header
    \param[in] headerpath   path of Interfile header
    \return value of key

    Get value of key from header. The key is an array.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
T Interfile::getValue(const std::string key, const unsigned short int a_idx,
                      std::vector <KV *> kv_hdr, const std::string headerfile,
                      const std::string headerpath)
 { T value;
   KV *key_value;

   key_value=searchKey(key, kv_hdr, headerfile);
   value=key_value->getValue<T>(a_idx);
   if (key_value->Type() == KV::DATASET_A)
    { tdataset ds;

      ds=*(tdataset *)&value;
      if ((ds.headerfile->length() >= 2) &&
          (ds.headerfile->at(0) != '/') &&
          (ds.headerfile->substr(0, 2) != "\\\\") &&
          (ds.headerfile->at(1) != ':'))
       *(ds.headerfile)=headerpath+*(ds.headerfile);
      if ((ds.datafile->length() >= 2) &&
          (ds.datafile->at(0) != '/') &&
          (ds.datafile->substr(0, 2) != "\\\\") &&
          (ds.datafile->at(1) != ':'))
       *(ds.datafile)=headerpath+*(ds.datafile);
      return(*(T *)&ds);
    }
   return(value);
 }

#ifndef _INTERFILE_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Create entry for parser table for key with units.
    \param[in] where        in which headers is key used ?
    \param[in] key          name of key
    \param[in] units        list of units allowed for this key
    \param[in] type         type of value
    \param[in] need_value   key needs a value ?
    \return token with key/value pair

    Create entry for parser table for key with units.
 */
/*---------------------------------------------------------------------------*/
Interfile::ttoken *Interfile::initKV(const tused_where where,
                                     const std::string key, std::string units,
                                     const KV::ttoken_type type,
                                     const bool need_value) const
 { ttoken *t=NULL;

   try
   { t=new ttoken;
     t->key=key;
     t->where=where;
     while (!units.empty())
      { std::string::size_type p;
                                    // parse allowed units and fill into vector
        if ((p=units.find(',')) != std::string::npos)
         { t->units.push_back(units.substr(0, p));
           units.erase(0, p+1);
         }
         else { t->units.push_back(units);
                break;
              }
      }
     t->type=type;
     t->need_value=need_value;
     t->max_lindex_key=std::string();
     t->max_index_key=std::string();
     return(t);
   }
   catch (...)
    { if (t != NULL) delete t;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create entry for parser table for key without unit.
    \param[in] where        in which headers is key used ?
    \param[in] key          name of key
    \param[in] type         type of value
    \param[in] need_value   key needs a value ?
    \return token with key/value pair

    Create entry for parser table for key without unit.
 */
/*---------------------------------------------------------------------------*/
Interfile::ttoken *Interfile::initKV(const tused_where where,
                                     const std::string key,
                                     const KV::ttoken_type type,
                                     const bool need_value) const
 { return(initKV(where, key, std::string(), type, need_value));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create entry for parser table for array-key with unit.
    \param[in] where           in which headers is key used ?
    \param[in] key             name of key
    \param[in] units           list of units allowed for this key
    \param[in] type            type of value
    \param[in] max_index_key   name of key which specifies maximum index of the
                               array
    \param[in] need_value      key needs a value ?
    \return token with key/value pair

    Create entry for parser table for array-key with unit.
 */
/*---------------------------------------------------------------------------*/
Interfile::ttoken *Interfile::initKV(const tused_where where,
                                     const std::string key, std::string units,
                                     const KV::ttoken_type type,
                                     const std::string max_index_key,
                                     const bool need_value) const
 { ttoken *t=NULL;

   try
   { t=initKV(where, key, units, type, need_value);
     t->max_index_key=max_index_key;
     return(t);
   }
   catch (...)
    { if (t != NULL) delete t;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create entry for parser table for array-key with unit and list of
           values.
    \param[in] where            in which headers is key used ?
    \param[in] key              name of key
    \param[in] units            list of units allowed for this key
    \param[in] type             type of value
    \param[in] max_lindex_key   name of key which specifies maximum index of
                                the list
    \param[in] max_index_key    name of key which specifies maximum index of
                                the array
    \param[in] need_value       key needs a value ?
    \return token with key/value pair

    Create entry for parser table for array-key with unit and list of values.
 */
/*---------------------------------------------------------------------------*/
Interfile::ttoken *Interfile::initKV(const tused_where where,
                                     const std::string key, std::string units,
                                     const KV::ttoken_type type,
                                     const std::string max_lindex_key,
                                     const std::string max_index_key,
                                     const bool need_value) const
 { ttoken *t=NULL;

   try
   { t=initKV(where, key, units, type, need_value);
     t->max_lindex_key=max_lindex_key;
     t->max_index_key=max_index_key;
     return(t);
   }
   catch (...)
    { if (t != NULL) delete t;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create entry for parser table for array-key without unit.
    \param[in] where           in which headers is key used ?
    \param[in] key             name of key
    \param[in] type            type of value
    \param[in] max_index_key   name of key which specifies maximum index of the
                               array
    \param[in] need_value      key needs a value ?
    \return token with key/value pair

    Create entry for parser table for array-key without unit.
 */
/*---------------------------------------------------------------------------*/
Interfile::ttoken *Interfile::initKV(const tused_where where,
                                     const std::string key,
                                     const KV::ttoken_type type,
                                     const std::string max_index_key,
                                     const bool need_value) const
 { return(initKV(where, key, std::string(), type, max_index_key, need_value));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create entry for parser table for array-key without unit and with
           list of values.
    \param[in] where            in which headers is key used ?
    \param[in] key              name of key
    \param[in] type             type of value
    \param[in] max_lindex_key   name of key which specifies maximum index of
                                the list
    \param[in] max_index_key    name of key which specifies maximum index of
                                the array
    \param[in] need_value       key needs a value ?
    \return token with key/value pair

    Create entry for parser table for array-key without unit and with list of
    values.
 */
/*---------------------------------------------------------------------------*/
Interfile::ttoken *Interfile::initKV(const tused_where where,
                                     const std::string key,
                                     const KV::ttoken_type type,
                                     const std::string max_lindex_key,
                                     const std::string max_index_key,
                                     const bool need_value) const
 { return(initKV(where, key, std::string(), type, max_lindex_key,
                 max_index_key, need_value));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create parser table for sinogram or image main header.

    Create parser table for sinogram or image main header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::initMain()
 { smh.push_back(initKV(SIH, "!INTERFILE", KV::NONE, false));
   smh.push_back(initKV(SIH, "%comment", KV::ASCII, true));
   smh.push_back(initKV(SIH, "!originating system", KV::ASCII, true));
   smh.push_back(initKV(SIH, "%CPS data type", KV::ASCII, true));
   smh.push_back(initKV(SIH, "%CPS version number", KV::ASCII, true));
   smh.push_back(initKV(SIH, "%gantry serial number", KV::ASCII, true));
   smh.push_back(initKV(SIH, "!GENERAL DATA", KV::NONE, false));
   smh.push_back(initKV(SIH, "original institution", KV::ASCII, true));
   smh.push_back(initKV(SIH, "data description", KV::ASCII, false));
   smh.push_back(initKV(SIH, "patient name", KV::ASCII, false));
   smh.push_back(initKV(SIH, "patient ID", KV::ASCII, true));
   smh.push_back(initKV(SIH, "%patient DOB", "yyyy:mm:dd", KV::DATE, false));
   smh.push_back(initKV(SIH, "patient sex", KV::SEX, true));
   smh.push_back(initKV(SIH, "patient height", "cm", KV::UFLOAT, false));
   smh.push_back(initKV(SIH, "patient weight", "kg", KV::UFLOAT, true));
   smh.push_back(initKV(SIH, "study ID", KV::ASCII, true));
   smh.push_back(initKV(SIH, "exam type", KV::EXAMTYPE, true));
   smh.push_back(initKV(SIH, "%study date", "yyyy:mm:dd", KV::DATE, true));
   smh.push_back(initKV(SIH, "%study time", "hh:mm:ss", KV::TIME, true));
   smh.push_back(initKV(SIH, "%study UTC", "sec", KV::ULONG, true));
   smh.push_back(initKV(SIH, "isotope name", KV::ASCII, true));
   smh.push_back(initKV(SIH, "isotope gamma halflife", "sec", KV::UFLOAT,
                        true));
   smh.push_back(initKV(SIH, "isotope branching factor", KV::UFLOAT, true));
   smh.push_back(initKV(SIH, "radiopharmaceutical", KV::ASCII, false));
   smh.push_back(initKV(SIH, "%tracer injection date", "yyyy:mm:dd", KV::DATE,
                        false));
   smh.push_back(initKV(SIH, "%tracer injection time", "hh:mm:ss", KV::TIME,
                        false));
   smh.push_back(initKV(SIH, "%tracer injection UTC", "sec", KV::ULONG,
                        false));
   smh.push_back(initKV(SIH, "relative time of tracer injection", "sec",
                        KV::LONG, true));
   smh.push_back(initKV(SIH, "tracer activity at time of injection", "mCi,MBq",
                        KV::UFLOAT, true));
   smh.push_back(initKV(SIH, "injected volume", "ml", KV::UFLOAT, false));
   smh.push_back(initKV(SIH, "%patient orientation", KV::ORIENTATION, true));
   smh.push_back(initKV(SIH, "%patient scan orientation", KV::ORIENTATION,
                        true));
   smh.push_back(initKV(SIH, "%scan direction", KV::DIRECTION, true));
   smh.push_back(initKV(SIH, "%coincidence window width", "nsec", KV::UFLOAT,
                        false));
   smh.push_back(initKV(SIH, "septa state", KV::SEPTA, true));
   smh.push_back(initKV(SIH, "gantry tilt angle", "degrees", KV::FLOAT, true));
   smh.push_back(initKV(SIH, "%gantry slew", "degrees", KV::FLOAT, true));
   smh.push_back(initKV(SIH, "%type of detector motion", KV::DETMOTION, true));
   smh.push_back(initKV(SIH, "%DATA MATRIX DESCRIPTION", KV::NONE, false));
   smh.push_back(initKV(SIH, "number of time frames", KV::USHRT, true));
   smh.push_back(initKV(SIH, "%number of horizontal bed offsets", KV::USHRT,
                        true));
   smh.push_back(initKV(SIH, "number of time windows", KV::USHRT, true));
   smh.push_back(initKV(SIH, "number of energy windows", KV::USHRT, true));
   smh.push_back(initKV(SIH, "%energy window lower level", "keV", KV::USHRT_A,
                        "number of energy windows", true));
   smh.push_back(initKV(SIH, "%energy window upper level", "keV", KV::USHRT_A,
                        "number of energy windows", true));
   smh.push_back(initKV(SH, "%number of emission data types", KV::USHRT,
                        true));
   smh.push_back(initKV(SH, "%emission data type description", KV::DTDES_A,
                        "%number of emission data types", true));
   smh.push_back(initKV(SH, "%number of transmission data types", KV::USHRT,
                        true));
   smh.push_back(initKV(SH, "%transmission data type description", KV::DTDES_A,
                        "%number of transmission data types", true));
   smh.push_back(initKV(SH, "%TOF bin width", "ns", KV::UFLOAT,
                        false));
   smh.push_back(initKV(SH, "%TOF gaussian fwhm", "ns", KV::UFLOAT, false));
   smh.push_back(initKV(SH, "%number of TOF time bins", KV::USHRT, true));
   smh.push_back(initKV(SIH, "%GLOBAL SCANNER CALIBRATION FACTOR", KV::NONE,
                        false));
   smh.push_back(initKV(SIH, "%scanner quantification factor", "mCi/ml",
                        KV::FLOAT, false));
   smh.push_back(initKV(SIH, "%calibration date", "yyyy:mm:dd", KV::DATE,
                       false));
   smh.push_back(initKV(SIH, "%calibration time",  "hh:mm:ss", KV::TIME,
                        false));
   smh.push_back(initKV(SIH, "%calibration UTC", "sec", KV::ULONG, false));
   smh.push_back(initKV(SIH, "%dead time quantification factor", KV::FLOAT_L,
                        std::string(), false));
   smh.push_back(initKV(SIH, "%DATA SET DESCRIPTION", KV::NONE, false));
   smh.push_back(initKV(SIH, "!total number of data sets", KV::USHRT, true));
   smh.push_back(initKV(SIH, "%data set", KV::DATASET_A,
                        "!total number of data sets", true));
   smh.push_back(initKV(SIH, "%SUPPLEMENTARY ATTRIBUTES", KV::NONE, false));
   smh.push_back(initKV(IH, "!PET scanner type", KV::SCANNER_TYPE, true));
   smh.push_back(initKV(IH, "transaxial FOV diameter", "mm", KV::UFLOAT,
                        false));
   smh.push_back(initKV(SIH, "number of rings", KV::USHRT, true));
   smh.push_back(initKV(IH, "%distance between rings", "mm", KV::UFLOAT,
                        true));
   smh.push_back(initKV(IH, "%bin size", "mm", KV::UFLOAT, true));
   smh.push_back(initKV(SIH, "%source model", KV::ASCII, false));
   smh.push_back(initKV(SIH, "%source serial", KV::ASCII, false));
   smh.push_back(initKV(SIH, "%source shape", KV::ASCII, false));
   smh.push_back(initKV(SIH, "%source radii", KV::USHRT, true));
   smh.push_back(initKV(SIH, "%source radius", "cm,mm", KV::UFLOAT_A,
                        "%source radii", false));
   smh.push_back(initKV(SIH, "%source length", "cm,mm", KV::UFLOAT, false));
   smh.push_back(initKV(SIH, "%source densities", KV::USHRT, true));
   smh.push_back(initKV(SIH, "%source mu", "1/cm", KV::UFLOAT_A,
                        "%source densities", false));
   smh.push_back(initKV(SIH, "%source ratios", KV::USHRT, true));
   smh.push_back(initKV(SIH, "%source ratio", KV::UFLOAT_A, "%source ratios",
                        false));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create parser table for norm header.

    Create parser table for norm header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::initNorm()
 { nh.push_back(initKV(NH, "!INTERFILE", KV::NONE, false));
   nh.push_back(initKV(NH, "%comment", KV::ASCII, false));
   nh.push_back(initKV(NH, "!originating system", KV::ASCII, true));
   nh.push_back(initKV(NH, "%CPS data type", KV::ASCII, true));
   nh.push_back(initKV(NH, "%CPS version number", KV::ASCII, true));
   nh.push_back(initKV(NH, "%gantry serial number", KV::ASCII, true));
   nh.push_back(initKV(NH, "!GENERAL DATA", KV::NONE, false));
   nh.push_back(initKV(NH, "data description", KV::ASCII, false));
   nh.push_back(initKV(NH, "!name of data file", KV::ASCII, true));
   nh.push_back(initKV(NH, "%expiration date", "yyyy:mm:dd", KV::DATE, true));
   nh.push_back(initKV(NH, "%expiration time", "hh:mm:ss", KV::TIME, true));
   nh.push_back(initKV(NH, "%expiration UTC", "sec", KV::ULONG, true));
   nh.push_back(initKV(NH, "!GENERAL IMAGE DATA", KV::NONE, false));
   nh.push_back(initKV(NH, "%study date", "yyyy:mm:dd", KV::DATE, true));
   nh.push_back(initKV(NH, "%study time", "hh:mm:ss", KV::TIME, true));
   nh.push_back(initKV(NH, "%study UTC", "sec", KV::ULONG, true));
   nh.push_back(initKV(NH, "image data byte order", KV::ENDIAN, true));
   nh.push_back(initKV(NH, "!PET data type", KV::PETDT, false));
   nh.push_back(initKV(NH, "data format", KV::ASCII, true));
   nh.push_back(initKV(NH, "number format", KV::NUMFORM, true));
   nh.push_back(initKV(NH, "!number of bytes per pixel", KV::USHRT, true));
   nh.push_back(initKV(NH, "%RAW NORMALIZATION SCANS DESCRIPTION", KV::NONE,
                       false));
   nh.push_back(initKV(NH, "%number of normalization scans", KV::USHRT, true));
   nh.push_back(initKV(NH, "%normalization scan", KV::ASCII_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "isotope name", KV::ASCII_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "radiopharmaceutical", KV::ASCII_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "total prompts", KV::ULONGLONG_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "%total randoms", KV::ULONGLONG_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "%total net trues", KV::LONGLONG_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "image duration", "sec", KV::ULONG_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "%number of bucket-rings", KV::USHRT_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "%total uncorrected singles", KV::ULONGLONG_A,
                       "%number of normalization scans", true));
   nh.push_back(initKV(NH, "%NORMALIZATION COMPONENTS DESCRIPTION", KV::NONE,
                       false));
   nh.push_back(initKV(NH, "%number of normalization components", KV::USHRT,
                       true));
   nh.push_back(initKV(NH, "%normalization component", KV::ASCII_A,
                       "%number of normalization components", true));
   nh.push_back(initKV(NH, "number of dimensions", KV::USHRT_A,
                       "%number of normalization components", true));
   nh.push_back(initKV(NH, "%matrix size", KV::USHRT_AL,
                       "number of dimensions",
                       "%number of normalization components", true));
   nh.push_back(initKV(NH, "%matrix axis label", KV::ASCII_AL,
                       "number of dimensions",
                       "%number of normalization components", true));
   nh.push_back(initKV(NH, "%matrix axis unit", KV::ASCII_AL,
                       "number of dimensions",
                       "%number of normalization components", true));
   nh.push_back(initKV(NH, "%scale factor", KV::UFLOAT_AL,
                       "number of dimensions",
                       "%number of normalization components", true));
   nh.push_back(initKV(NH, "data offset in bytes", KV::ULONGLONG_A,
                       "%number of normalization components", true));
   nh.push_back(initKV(NH, "%axial compression", KV::USHRT, true));
   nh.push_back(initKV(NH, "%maximum ring difference", KV::USHRT, true));
   nh.push_back(initKV(NH, "number of rings", KV::USHRT, true));
   nh.push_back(initKV(NH, "number of energy windows", KV::USHRT, true));
   nh.push_back(initKV(NH, "%energy window lower level", "keV", KV::USHRT_A,
                        "number of energy windows", true));
   nh.push_back(initKV(NH, "%energy window upper level", "keV", KV::USHRT_A,
                        "number of energy windows", true));
   nh.push_back(initKV(NH, "%GLOBAL SCANNER CALIBRATION FACTOR", KV::NONE,
                       false));
   nh.push_back(initKV(NH, "%scanner quantification factor", "mCi/ml",
                       KV::FLOAT, true));
   nh.push_back(initKV(NH, "%calibration date", "yyyy:mm:dd", KV::DATE,
                       false));
   nh.push_back(initKV(NH, "%calibration time",  "hh:mm:ss", KV::TIME, false));
   nh.push_back(initKV(NH, "%calibration UTC", "sec", KV::ULONG, false));
   nh.push_back(initKV(NH, "%dead time quantification factor", KV::FLOAT_L,
                       std::string(), true));
   nh.push_back(initKV(NH, "%DATA SET DESCRIPTION", KV::NONE, false));
   nh.push_back(initKV(NH, "!total number of data sets", KV::USHRT, true));
   nh.push_back(initKV(NH, "%data set", KV::DATASET_A,
                           "!total number of data sets", true));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create parser table for sinogram or image subheader.

    Create parser table for sinogram or image subheader.
 */
/*---------------------------------------------------------------------------*/
void Interfile::initSub()
 { ssh.push_back(initKV(SIH, "!INTERFILE", KV::NONE, false));
   ssh.push_back(initKV(SIH, "%comment", KV::ASCII, true));
   ssh.push_back(initKV(SIH, "!originating system", KV::ASCII, true));
   ssh.push_back(initKV(SIH, "%CPS data type", KV::ASCII, true));
   ssh.push_back(initKV(SIH, "%CPS version number", KV::ASCII, true));
   ssh.push_back(initKV(SIH, "!GENERAL DATA", KV::NONE, false));
   ssh.push_back(initKV(SH, "%name of listmode file", KV::RELFILENAME, false));
   ssh.push_back(initKV(IH, "%name of sinogram file", KV::RELFILENAME, true));
   ssh.push_back(initKV(SIH, "!name of data file", KV::RELFILENAME, true));
   ssh.push_back(initKV(SIH, "!GENERAL IMAGE DATA", KV::NONE, false));
   ssh.push_back(initKV(SIH, "!type of data", KV::ASCII, true));
   ssh.push_back(initKV(SIH, "%study date", "yyyy:mm:dd", KV::DATE, true));
   ssh.push_back(initKV(SIH, "%study time", "hh:mm:ss", KV::TIME, true));
   ssh.push_back(initKV(SIH, "%study UTC", "sec", KV::ULONG, true));
   ssh.push_back(initKV(SIH, "image data byte order", KV::ENDIAN, true));
   ssh.push_back(initKV(SIH, "!PET data type", KV::PETDT, true));
   ssh.push_back(initKV(SH, "data format", KV::ASCII, true));
   ssh.push_back(initKV(SIH, "number format", KV::NUMFORM, true));
   ssh.push_back(initKV(SIH, "!number of bytes per pixel", KV::USHRT, true));
   ssh.push_back(initKV(IH, "!image scale factor", KV::FLOAT, true));
   ssh.push_back(initKV(SIH, "number of dimensions", KV::USHRT, true));
   ssh.push_back(initKV(SIH, "matrix axis label", KV::ASCII_A,
                        "number of dimensions", true));
   ssh.push_back(initKV(SIH, "matrix size", KV::USHRT_A,
                        "number of dimensions", true));
   ssh.push_back(initKV(SIH, "scale factor", "mm/pixel,degree/pixel",
                        KV::UFLOAT_A, "number of dimensions", true));
   ssh.push_back(initKV(SIH, "start horizontal bed position", "mm", KV::FLOAT,
                        true));
   ssh.push_back(initKV(SIH, "start vertical bed position", "mm", KV::FLOAT,
                        true));
   ssh.push_back(initKV(IH, "%reconstruction diameter", "mm", KV::UFLOAT,
                        true));
   ssh.push_back(initKV(IH, "%reconstruction bins", KV::USHRT, true));
   ssh.push_back(initKV(IH, "%reconstruction views", KV::USHRT, true));
   ssh.push_back(initKV(IH, "process status", KV::PROCSTAT, true));
   ssh.push_back(initKV(SIH, "quantification units", KV::ASCII, false));
   ssh.push_back(initKV(SIH, "%decay correction", KV::DECAYCORR, false));
   ssh.push_back(initKV(SIH, "decay correction factor", KV::UFLOAT, false));
   ssh.push_back(initKV(SIH, "%scatter fraction factor", KV::UFLOAT, false));
   ssh.push_back(initKV(SIH, "%dead time factor", KV::FLOAT_L, std::string(),
                        false));
   ssh.push_back(initKV(IH, "slice orientation", KV::SL_ORIENTATION, true));
   ssh.push_back(initKV(IH, "method of reconstruction", KV::ASCII, false));
   ssh.push_back(initKV(IH, "number of iterations", KV::USHRT, false));
   ssh.push_back(initKV(IH, "%number of subsets", KV::USHRT, false));
   ssh.push_back(initKV(IH, "stopping criteria", KV::ASCII, true));
   ssh.push_back(initKV(IH, "filter name", KV::ASCII, false));
   ssh.push_back(initKV(IH, "%xy-filter", "mm", KV::UFLOAT, false));
   ssh.push_back(initKV(IH, "%z-filter", "mm", KV::UFLOAT, false));
   ssh.push_back(initKV(IH, "%image zoom", KV::UFLOAT, true));
   ssh.push_back(initKV(IH, "%x-offset", "mm", KV::UFLOAT, true));
   ssh.push_back(initKV(IH, "%y-offset", "mm", KV::UFLOAT, true));
   ssh.push_back(initKV(SH, "number of scan data types", KV::USHRT, true));
   ssh.push_back(initKV(SH, "scan data type description", KV::DTDES_A,
                        "number of scan data types", true));
   ssh.push_back(initKV(SH, "data offset in bytes", KV::ULONG_A,
                        "number of scan data types", true));
   ssh.push_back(initKV(SH, "angular compression", KV::USHRT, true));
   ssh.push_back(initKV(SIH, "%axial compression", KV::USHRT, true));
   ssh.push_back(initKV(SIH, "%maximum ring difference", KV::USHRT, true));
   ssh.push_back(initKV(SH, "%number of segments", KV::USHRT, true));
   ssh.push_back(initKV(SH, "%segment table", KV::USHRT_L,
                        "%number of segments", true));
   ssh.push_back(initKV(SIH, "applied corrections", KV::ASCII_L, std::string(),
                        false));
   ssh.push_back(initKV(SIH, "method of attenuation correction", KV::ASCII,
                        false));
   ssh.push_back(initKV(IH, "method of scatter correction", KV::ASCII, false));
   ssh.push_back(initKV(IH, "%method of random correction", KV::RANDCORR,
                        false));
   ssh.push_back(initKV(SIH, "!IMAGE DATA DESCRIPTION", KV::NONE, false));
   ssh.push_back(initKV(SIH, "!total number of data sets", KV::USHRT, true));
   ssh.push_back(initKV(SIH, "%acquisition start condition", KV::ACQCOND,
                        true));
   ssh.push_back(initKV(SIH, "%acquisition termination condition", KV::ACQCOND,
                        true));
   ssh.push_back(initKV(SIH, "!image duration", "sec", KV::ULONG, true));
   ssh.push_back(initKV(SIH, "image relative start time", "sec", KV::ULONG,
                        true));
   ssh.push_back(initKV(SIH, "total prompts", KV::ULONGLONG, true));
   ssh.push_back(initKV(SIH, "%total randoms", KV::ULONGLONG, true));
   ssh.push_back(initKV(SIH, "%total net trues", KV::LONGLONG, true));
   ssh.push_back(initKV(SIH, "%total uncorrected singles", KV::ULONGLONG,
                        true));
   ssh.push_back(initKV(SH, "%number of bucket-rings", KV::USHRT, false));
   ssh.push_back(initKV(SH, "%bucket-ring singles", KV::ULONGLONG_A,
                        "%number of bucket-rings", false));
   ssh.push_back(initKV(IH, "%SUPPLEMENTARY ATTRIBUTES", KV::NONE, false));
   ssh.push_back(initKV(IH, "%number of projections", KV::USHRT, false));
   ssh.push_back(initKV(IH, "%number of views", KV::USHRT, false));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change ordering of two keys.
    \param[in,out] kv      key/value list
    \param[in]     after   put key(last_pos[0]) after key(last_pos[1])
                           (otherwise put if before)
    \exception REC_ITF_HEADER_ERROR_KEY_ORDER can't change the ordering of the
                                              keys

    Change ordering of two keys. The positions of the keys in the kev/value
    list are stored in \em last_pos[].
 */
/*---------------------------------------------------------------------------*/
void Interfile::keyResort(std::vector <KV *> *kv, bool after) const
 { KV *tmp;
   unsigned long int idx[2];
                                                      // search indices of keys
   if ((last_pos[0] == std::numeric_limits <unsigned short int>::max()) ||
       (last_pos[1] == std::numeric_limits <unsigned short int>::max()))
    throw Exception(REC_ITF_HEADER_ERROR_KEY_ORDER,
                    "Can't change ordering of keys.");
   idx[0]=last_pos[0]-1;
   idx[1]=last_pos[1]-1;
        // swap indices if evaluation order of parameters is "last param first"
   if (!fpf) std::swap(idx[0], idx[1]);
   tmp=kv->at(idx[0]);
   if (after) idx[1]+=1;   // put key with index idx1 after key with index idx2
                          // put key with index idx1 before key with index idx2
   for (unsigned long int i=idx[0]+1; i < kv->size(); i++)
    { kv->at(i-1)=kv->at(i);
      if (i == idx[1]-1) { kv->at(idx[1]-1)=tmp;
                           return;
                         }
    }
   for (signed long int j=(signed long int)kv->size()-2;
        j >= (signed long int)idx[1];
        j--)
    kv->at(j+1)=kv->at(j);
   kv->at(idx[1])=tmp;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load main header.
    \param[in] filename   name of Interfile header

    Load main header into internal data structures and check for syntactical
    and semantical errors.
 */
/*---------------------------------------------------------------------------*/
void Interfile::loadMainHeader(const std::string filename)
 { std::string::size_type p;

   main_headerfile=filename;
   p=filename.rfind('\\');
   if (p == std::string::npos) p=filename.rfind('/');
   main_headerpath=filename.substr(0, p+1);
   deleteKVList(&kv_smh);
   syntaxParser(main_headerfile, smh, &kv_smh);        // load and parse syntax
   semanticParser(main_headerfile, smh, kv_smh);             // parse semantics
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load norm header.
    \param[in] filename   name of Interfile header

    Load norm header into internal data structures and check for syntactical
    and semantical errors.
 */
/*---------------------------------------------------------------------------*/
void Interfile::loadNormHeader(const std::string filename)
 { std::string::size_type p;

   norm_headerfile=filename;
   p=filename.rfind('\\');
   if (p == std::string::npos) p=filename.rfind('/');
   norm_headerpath=filename.substr(0, p+1);
   deleteKVList(&kv_nh);
   syntaxParser(norm_headerfile, nh, &kv_nh);          // load and parse syntax
   semanticParser(norm_headerfile, nh, kv_nh);               // parse semantics
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load subheader.
    \param[in] filename   name of Interfile header

    Load subheader into internal data structures and check for syntactical and
    semantical errors.
 */
/*---------------------------------------------------------------------------*/
void Interfile::loadSubheader(const std::string filename)
 { std::string::size_type p;

   subheaderfile=filename;
   p=filename.rfind('\\');
   if (p == std::string::npos) p=filename.rfind('/');
   subheaderpath=filename.substr(0, p+1);
   deleteKVList(&kv_ssh);
   syntaxParser(subheaderfile, ssh, &kv_ssh);          // load and parse syntax
   semanticParser(subheaderfile, ssh, kv_ssh);               // parse semantics
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add blank line to key/value list of main header.
    \param[in] comment   comment for line
    \return 0

    Add blank line to key/value list of main header. The function returns an
    unsigned long int, so that it can be used in calls to methods like
    MainKeyAfterKey().
 */
/*---------------------------------------------------------------------------*/
unsigned long int Interfile::Main_blank_line(const std::string comment)
 { blankLine(comment, &kv_smh);
   return(0);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Put first key after second key in main header.
    \param[in] dummy   first key
    \param[in] dummy   second key

    Put first key after second key in main header.
 */
/*---------------------------------------------------------------------------*/
template <typename T, typename U>
void Interfile::MainKeyAfterKey(T, U)
 { keyResort(&kv_smh, true);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Put first key before second key in main header.
    \param[in] dummy   first key
    \param[in] dummy   second key

    Put first key before second key in main header.
 */
/*---------------------------------------------------------------------------*/
template <typename T, typename U>
void Interfile::MainKeyBeforeKey(T, U)
 { keyResort(&kv_smh, false);
 }

#ifndef _INTERFILE_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Add blank line to key/value list of norm header.
    \param[in] comment   comment for line
    \return 0

    Add blank line to key/value list of norm header. The function returns an
    unsigned long int, so that it can be used in calls to methods like
    NormKeyAfterKey().
 */
/*---------------------------------------------------------------------------*/
unsigned long int Interfile::Norm_blank_line(const std::string comment)
 { blankLine(comment, &kv_nh);
   return(0);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Put first key after second key in norm header.
    \param[in] dummy   first key
    \param[in] dummy   second key

    Put first key after second key in norm header.
 */
/*---------------------------------------------------------------------------*/
template <typename T, typename U>
void Interfile::NormKeyAfterKey(T, U)
 { keyResort(&kv_nh, true);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Put first key before second key in norm header.
    \param[in] dummy   first key
    \param[in] dummy   second key

    Put first key before second key in norm header.
 */
/*---------------------------------------------------------------------------*/
template <typename T, typename U>
void Interfile::NormKeyBeforeKey(T, U)
 { keyResort(&kv_nh, false);
 }

#ifndef _INTERFILE_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Save header.
    \param[in] filename   name of Interfile header
    \param[in] sh         parser table of header
    \param[in] kv         key/value list of header
    \exception REC_FILE_CREATE can't create the file

    Save header. The header is checked for semantical errors.
 */
/*---------------------------------------------------------------------------*/
void Interfile::saveHeader(const std::string filename,
                           std::vector <ttoken *> sh,
                           std::vector <KV *> kv)
 { std::ofstream *file=NULL;

   try
   { std::vector <KV *>::const_iterator it_kv;

     semanticParser(filename, sh, kv);             // check for semantic errors
     file=new std::ofstream(filename.c_str());
     if (!*file)
      throw Exception(REC_FILE_CREATE,
                      "Can't create the file '#1'.").arg(filename);
     for (it_kv=kv.begin(); it_kv != kv.end(); it_kv++)
      (*it_kv)->print(file);
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save main header.
    \param[in] filename   name of Interfile header

    Save main header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::saveMainHeader(const std::string filename)
 { main_headerfile=filename;
   saveMainHeader();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save main header.

    Save main header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::saveMainHeader()
 { saveHeader(main_headerfile, smh, kv_smh);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save norm header.
    \param[in] filename   name of Interfile header

    Save norm header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::saveNormHeader(const std::string filename)
 { norm_headerfile=filename;
   saveNormHeader();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save norm header.

    Save norm header.
 */
/*---------------------------------------------------------------------------*/
void Interfile::saveNormHeader()
 { saveHeader(norm_headerfile, nh, kv_nh);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save subheader.
    \param[in] filename   name of Interfile header

    Save subheader.
 */
/*---------------------------------------------------------------------------*/
void Interfile::saveSubheader(const std::string filename)
 { subheaderfile=filename;
   saveSubheader();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save subheader.

    Save subheader.
 */
/*---------------------------------------------------------------------------*/
void Interfile::saveSubheader()
 { saveHeader(subheaderfile, ssh, kv_ssh);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Search key/value list for given key name.
    \param[in] key        name of key to search for
    \param[in] kv         kev/value list
    \param[in] filename   name of Interfile header
    \return key/value pair
    \exception REC_ITF_HEADER_ERROR_KEY_MISSING key no found

    Search key/value list for given key name.
 */
/*---------------------------------------------------------------------------*/
KV *Interfile::searchKey(const std::string key, std::vector <KV *> kv,
                         const std::string filename)
 { unsigned short int pos=1;

   for (std::vector <KV *>::iterator it=kv.begin(); it != kv.end(); ++it,
        pos++)
    if ((*it)->Key() == key) { last_pos[0]=last_pos[1];
                               last_pos[1]=pos;
                               return(*it);
                             }
   throw Exception(REC_ITF_HEADER_ERROR_KEY_MISSING,
                   "Parsing error in the file '#1'. The key '#2' is missing"
                   ".").arg(filename).arg(key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse key/value list for semantic errors.
    \param[in] filename   name of Interfile header
    \param[in] sh         parser table for header
    \param[in] kv         key/value list for header
    \exception REC_ITF_HEADER_ERROR_KEY_INVALID key is not in parser table

    Parse key/value list for semantic errors.
 */
/*--------------------------------------------------------------------------*/
void Interfile::semanticParser(const std::string filename,
                               std::vector <ttoken *> sh,
                               std::vector <KV *> kv)
 { std::vector <KV *>::const_iterator it_kv;
   bool image_header=false, norm_header;

   if (kv == kv_smh)
    image_header=(Main_CPS_data_type() == "image main header");
    else if (kv == kv_ssh)
          image_header=(Sub_CPS_data_type() == "image subheader");
   norm_header=(kv == kv_nh);
   for (it_kv=kv.begin(); it_kv != kv.end(); ++it_kv)
    { unsigned short int max_num;
      std::vector <ttoken *>::const_iterator it;
      tused_where w=ALL;
                                          // get maximum allowed index of array
      switch ((*it_kv)->Type())
       { case KV::ASCII_A:
         case KV::ASCII_L:
         case KV::ASCII_AL:
         case KV::USHRT_L:
         case KV::USHRT_A:
         case KV::USHRT_AL:
         case KV::ULONG_A:
         case KV::ULONG_L:
         case KV::ULONGLONG_A:
         case KV::UFLOAT_L:
         case KV::FLOAT_L:
         case KV::UFLOAT_A:
         case KV::UFLOAT_AL:
         case KV::DATASET_A:
         case KV::DTDES_A:
          if ((*it_kv)->MaxIndexKey().empty())
           max_num=std::numeric_limits <unsigned short int>::max();
           else max_num=searchKey((*it_kv)->MaxIndexKey(), kv,
                                  filename)->getValue<unsigned short int>();
          break;
         default:
          max_num=0;
          break;
       }
                                                        // check key/value pair
      (*it_kv)->semanticCheck(filename, max_num);
      switch ((*it_kv)->Type())
       { case KV::ASCII_AL:
         case KV::USHRT_AL:
         case KV::UFLOAT_AL:
          (*it_kv)->semanticCheckAL(filename,
                                    searchKey((*it_kv)->MaxLIndexKey(), kv,
                                              filename));
          break;
         default:
          break;
       }
      for (it=sh.begin(); it != sh.end(); ++it)
       if ((*it_kv)->Key() == (*it)->key) w=(*it)->where;
      if (norm_header && (w == NH)) continue;
      if (image_header && (w == IH)) continue;
      if (!image_header && (w == SH)) continue;
      if (!norm_header && (w == SIH)) continue;
      if (w == ALL) continue;
      throw Exception(REC_ITF_HEADER_ERROR_KEY_INVALID,
                      "Parsing error in the file '#1'. The key '#2' is "
                      "invalid.").arg(filename).arg((*it_kv)->Key());
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add a separator key to key/value list.
    \param[in]     key       name of key
    \param[in]     comment   comment for key
    \param[in,out] kv        key/value list

    Add a separator key (key without unit and value) to key/value list.
 */
/*---------------------------------------------------------------------------*/
void Interfile::separatorKey(const std::string key, const std::string comment,
                             std::vector <KV *> *kv)
 { kv->push_back(new KV(key, std::string(), std::string(), KV::NONE, comment,
                        std::string()));
   last_pos[0]=last_pos[1];
   last_pos[1]=(unsigned short int)kv->size();
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Add key/value with unit to a header.
    \param[in]     key          name of the key
    \param[in]     value        value of the key
    \param[in]     unit         unit of the value
    \param[in]     comment      comment for the key/value pair
    \param[in]     headerfile   name of headerfile
    \param[in]     err_id       error id if header doesn't exist
    \param[in]     headername   name of header
    \param[in,out] kv_hdr       key/value list of header
    \param[in]     hdr          parser table of header
    \exception err_id  can't set key because header doesn't exist

    Add key/value with unit to a header.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Interfile::setValue(const std::string key, const T value,
                         std::string unit, const std::string comment,
                         const std::string headerfile,
                         const unsigned long int err_id,
                         const std::string headername,
                         std::vector <KV *> *kv_hdr,
                         std::vector <ttoken *> hdr)
 { if (headerfile.empty())
    throw Exception(err_id, "Can't set key '#1' because "+headername+
                            "header doesn't exist.").arg(key);
   std::string unit2;
   unsigned short int pos=1;

   if (typeid(T) == typeid(TIMEDATE::ttime))
    { std::string s, mi;
      TIMEDATE::ttime v;

      v=*(TIMEDATE::ttime *)&value;
      s=" GMT";
      if ((v.gmt_offset_h < 0) || (v.gmt_offset_m < 0)) s+="-";
       else s+="+";
      mi=KV::convertToString(abs(v.gmt_offset_m));
      if (mi.length() < 2) mi="0"+mi;
      unit2=unit+s+KV::convertToString(abs(v.gmt_offset_h))+":"+mi;
    }
    else unit2=unit;
   for (std::vector <KV *>::iterator it=kv_hdr->begin();
        it != kv_hdr->end();
        ++it,
        pos++)
    if ((*it)->Key() == key)
     { checkUnit(hdr, key, key, unit, headerfile);
       last_pos[0]=last_pos[1];
       last_pos[1]=pos;
       (*it)->setValue(value, unit, comment, headerfile);
       return;
     }
   tokenize(key, unit2, std::string(), KV::convertToString(value), comment,
            hdr, kv_hdr, headerfile);
   last_pos[0]=last_pos[1];
   last_pos[1]=(unsigned short int)kv_hdr->size();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add key/value without unit to a header.
    \param[in]     key          name of the key
    \param[in]     value        value of the key
    \param[in]     comment      comment for the key/value pair
    \param[in]     headerfile   name of headerfile
    \param[in]     err_id       error id if header doesn't exist
    \param[in]     headername   name of header
    \param[in,out] kv_hdr       key/value list of header
    \param[in]     hdr          parser table of header
    \exception err_id  can't set key because header doesn't exist

    Add key/value without unit to a header.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Interfile::setValue(const std::string key, const T value,
                         const std::string comment,
                         const std::string headerfile,
                         const unsigned long int err_id,
                         const std::string headername,
                         std::vector <KV *> *kv_hdr,
                         std::vector <ttoken *> hdr)
 { if (headerfile.empty())
    throw Exception(err_id, "Can't set key '#1' because "+headername+
                            "header doesn't exist.").arg(key);
   unsigned short int pos=1;

   for (std::vector <KV *>::iterator it=kv_hdr->begin();
        it != kv_hdr->end();
        ++it,
        pos++)
    if ((*it)->Key() == key)
     { checkUnit(hdr, key, key, std::string(), headerfile);
       last_pos[0]=last_pos[1];
       last_pos[1]=pos;
       (*it)->setValue(value, std::string(), comment, headerfile);
       return;
     }
   tokenize(key, std::string(), std::string(), KV::convertToString(value),
            comment, hdr, kv_hdr, headerfile);
   last_pos[0]=last_pos[1];
   last_pos[1]=(unsigned short int)kv_hdr->size();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add array-key/value with unit to a header.
    \param[in]     key          name of the key
    \param[in]     value        value of the key
    \param[in]     a_idx        index of array
    \param[in]     unit         unit of the value
    \param[in]     comment      comment for the key/value pair
    \param[in]     headerfile   name of headerfile
    \param[in]     err_id       error id if header doesn't exist
    \param[in]     headername   name of header
    \param[in,out] kv_hdr       key/value list of header
    \param[in]     hdr          parser table of header
    \exception err_id  can't set key because header doesn't exist

    Add array-key/value with unit to a header.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Interfile::setValue(const std::string key, const T value,
                         const unsigned short int a_idx,
                         const std::string unit, const std::string comment,
                         const std::string headerfile,
                         const unsigned long int err_id,
                         const std::string headername,
                         std::vector <KV *> *kv_hdr,
                         std::vector <ttoken *> hdr)
 { if (headerfile.empty())
    throw Exception(err_id,
                    "Can't set key '#1' because "+headername+"header doesn't "
                    "exist.").arg(std::string(key)+"["+toString(a_idx+1)+"]");
   unsigned short int pos=1;

   for (std::vector <KV *>::const_iterator it=kv_hdr->begin();
        it != kv_hdr->end();
        ++it,
        pos++)
    if ((*it)->Key() == key)
     { checkUnit(hdr, std::string(key)+"["+toString(a_idx+1)+"]", key, unit,
                 headerfile);
       last_pos[0]=last_pos[1];
       last_pos[1]=pos;
       (*it)->setValue(value, unit, comment, a_idx, headerfile);
       return;
     }
   tokenize(key, unit, "["+toString(a_idx+1)+"]", KV::convertToString(value),
            comment, hdr, kv_hdr, headerfile);
   last_pos[0]=last_pos[1];
   last_pos[1]=(unsigned short int)kv_hdr->size();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add array-key/value without unit to a header.
    \param[in]     key          name of the key
    \param[in]     value        value of the key
    \param[in]     a_idx        index of array
    \param[in]     comment      comment for the key/value pair
    \param[in]     headerfile   name of headerfile
    \param[in]     err_id       error id if header doesn't exist
    \param[in]     headername   name of header
    \param[in,out] kv_hdr       key/value list of header
    \param[in]     hdr          parser table of header
    \exception err_id  can't set key because header doesn't exist

    Add array-key/value without unit to a header.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Interfile::setValue(const std::string key, const T value,
                         const unsigned short int a_idx,
                         const std::string comment,
                         const std::string headerfile,
                         const unsigned long int err_id,
                         const std::string headername,
                         std::vector <KV *> *kv_hdr,
                         std::vector <ttoken *> hdr, const bool list)
 { if (headerfile.empty())
    throw Exception(err_id,
                    "Can't set key '#1' because "+headername+"header doesn't "
                    "exist.").arg(std::string(key)+"["+toString(a_idx+1)+"]");
   unsigned short int pos=1;

   for (std::vector <KV *>::iterator it=kv_hdr->begin();
        it != kv_hdr->end();
        ++it,
        pos++)
    if ((*it)->Key() == key)
     { checkUnit(hdr, std::string(key)+"["+toString(a_idx+1)+"]", key,
                 std::string(), headerfile);
       last_pos[0]=last_pos[1];
       last_pos[1]=pos;
       (*it)->setValue(value, std::string(), comment, a_idx, headerfile);
       return;
     }
   std::string val;

   val=KV::convertToString(value);
   if (list || (typeid(T) == typeid(KV::tdataset))) val="{"+val+"}";
   tokenize(key, std::string(), "["+toString(a_idx+1)+"]", val, comment, hdr,
            kv_hdr, headerfile);
   last_pos[0]=last_pos[1];
   last_pos[1]=(unsigned short int)kv_hdr->size();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add array-key/list-of-values without unit to a header.
    \param[in]     key          name of the key
    \param[in]     value        value of the key
    \param[in]     a_idx        index of array
    \param[in]     l_idx        index of list
    \param[in]     comment      comment for the key/value pair
    \param[in]     headerfile   name of headerfile
    \param[in]     err_id       error id if header doesn't exist
    \param[in]     headername   name of header
    \param[in,out] kv_hdr       key/value list of header
    \param[in]     hdr          parser table of header
    \exception err_id  can't set key because header doesn't exist

    Add array-key/list-of-values without unit to a header.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Interfile::setValue(const std::string key, const T value,
                         const unsigned short int a_idx,
                         const unsigned short int l_idx,
                         const std::string comment,
                         const std::string headerfile,
                         const unsigned long int err_id,
                         const std::string headername,
                         std::vector <KV *> *kv_hdr,
                         std::vector <ttoken *> hdr)
 { if (headerfile.empty())
    throw Exception(err_id,
                    "Can't set key '#1' because "+headername+" header doesn't "
                    "exist.").arg(std::string(key)+"["+toString(a_idx+1)+"]");
   unsigned short int pos=1;

   for (std::vector <KV *>::iterator it=kv_hdr->begin();
        it != kv_hdr->end();
        ++it,
        pos++)
    if ((*it)->Key() == key)
     { checkUnit(hdr, std::string(key)+"["+toString(a_idx+1)+"]", key,
                 std::string(), headerfile);
       last_pos[0]=last_pos[1];
       last_pos[1]=pos;
       (*it)->setValue(value, std::string(), comment, a_idx, l_idx,
                       headerfile);
       return;
     }
   tokenize(key, std::string(), "["+toString(a_idx+1)+"]",
            "{"+KV::convertToString(value)+"}", comment, hdr, kv_hdr,
            headerfile);
   last_pos[0]=last_pos[1];
   last_pos[1]=(unsigned short int)kv_hdr->size();
 }

#ifndef _INTERFILE_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Add blank line to key/value list of subheader.
    \param[in] comment   comment for line
    \return 0

    Add blank line to key/value list of subheader. The function returns an
    unsigned long int, so that it can be used in calls to methods like
    SubKeyAfterKey().
 */
/*---------------------------------------------------------------------------*/
unsigned long int Interfile::Sub_blank_line(const std::string comment)
 { blankLine(comment, &kv_ssh);
   return(0);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Put first key after second key in subheader.
    \param[in] dummy   first key
    \param[in] dummy   second key

    Put first key after second key in subheader.
 */
/*---------------------------------------------------------------------------*/
template <typename T, typename U>
void Interfile::SubKeyAfterKey(T, U)
 { keyResort(&kv_ssh, true);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Put first key before second key in subheader.
    \param[in] dummy   first key
    \param[in] dummy   second key

    Put first key before second key in subheader.
 */
/*---------------------------------------------------------------------------*/
template <typename T, typename U>
void Interfile::SubKeyBeforeKey(T, U)
 { keyResort(&kv_ssh, false);
 }

#ifndef _INTERFILE_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Parse Interfile header and perform syntax checks.
    \param[in]     filename   name of Interfile header
    \param[in]     token      parser table of header
    \param[in,out] kv         kev/value list
    \exception REC_FILE_DOESNT_EXIST                  file doesn't exist
    \exception REC_ITF_HEADER_ERROR_SEPARATOR_MISSING separator ':=' missing
    \exception REC_ITF_HEADER_ERROR_UNITFORMAT        format of unit wrong

    Parse Interfile header and perform syntax checks.
 */
/*---------------------------------------------------------------------------*/
void Interfile::syntaxParser(const std::string filename,
                             std::vector <ttoken *> token,
                             std::vector <KV *> *kv)
 { std::ifstream *file=NULL;

   try
   { file=new std::ifstream(filename.c_str());
     if (!*file) throw Exception(REC_FILE_DOESNT_EXIST,
                                 "The file '#1' doesn't exist.").arg(filename);
     for (;;)
      { std::string line, key, value, comment, short_key, idx, unit;
        std::string::size_type p;
                                   // read line from file and split into peaces
        std::getline(*file, line);
                // eof is only false after trying to read after the end of file
        if (file->eof() && line.empty()) break;
                                         // convert DOS format into Unix format
        while ((p=line.find('\r')) != std::string::npos) line.erase(p, 1);
                                                             // cut-off comment
        if ((p=line.find(';')) != std::string::npos)
         { comment=line.substr(p+1);
           if (comment.at(0) == ' ') comment.erase(0, 1);
           line.erase(p);
         }
                                                                 // remove TABs
        while ((p=line.find('\t')) != std::string::npos) line.at(p)=' ';
                                                         // remove white spaces
        while ((p=line.find("  ")) != std::string::npos) line.erase(p, 1);
        if (line.empty())                                       // line empty ?
         { kv->push_back(new KV(comment));
           continue;
         }
        if (line.at(0) == ' ') line.erase(0, 1);        // remove leading space
        if (line.empty())                                       // line empty ?
         { kv->push_back(new KV(comment));
           continue;
         }
                                                       // remove trailing space
        if (line.at(line.length()-1) == ' ') line.erase(line.length()-1);
        if (line.empty())                                       // line empty ?
         { kv->push_back(new KV(comment));
           continue;
         }
        if ((p=line.find(":=")) == std::string::npos)
         throw Exception(REC_ITF_HEADER_ERROR_SEPARATOR_MISSING,
                         "Parsing error in the file '#1'. ':=' is missing in "
                         "the line '#2'.").arg(filename).arg(line);
         else { key=line.substr(0, p);
                key.erase(key.find_last_not_of(" \t")+1);
                if ((unsigned long int)p+2 < line.length())
                 { value=line.substr(p+2);
                   if (value.at(0) == ' ') value.erase(0, 1);
                 }
                 else value=std::string();
              }
                                                     // remove indices from key
        short_key=key;
        if ((p=short_key.find('[')) != std::string::npos)
         { short_key.erase(p);
                                                       // remove trailing space
           if (short_key.at(short_key.length()-1) == ' ')
            short_key.erase(short_key.length()-1);
           idx=key.substr(p, key.find(']')-p+1);
         }
         else idx=std::string();
                                                        // remove unit from key
        if ((p=short_key.find('(')) != std::string::npos)
         { std::string key_index;

           key_index=short_key;
           if (idx != std::string()) key_index+="["+idx+"]";
           unit=short_key.substr(p+1);
           if (unit.at(unit.length()-1) != ')')
            throw Exception(REC_ITF_HEADER_ERROR_UNITFORMAT,
                            "Parsing error in the file '#1'. The format of the"
                            " unit of the key '#2' is wrong.").arg(filename).
                   arg(key_index);
            else unit.erase(unit.length()-1);
           short_key.erase(p);
                                                       // remove trailing space
           if (short_key.at(short_key.length()-1) == ' ')
            short_key.erase(short_key.length()-1);
         }
         else unit=std::string();
                                       // fill information into internal tables
        tokenize(short_key, unit, idx, value, comment, token, kv, filename);
      }
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert time and date into UTC
    \param[in]  d     date
    \param[in]  t     time
    \param[out] utc   UTC (seconds since 1.1.1970, 00:00:00, GMT-0)

    Convert time and date into UTC (seconds since 1.1.1970, 00:00:00, GMT-0).
 */
/*---------------------------------------------------------------------------*/
void Interfile::time2UTC(TIMEDATE::tdate const d, TIMEDATE::ttime const t,
                         time_t * const utc)
 { tm tf;

   tf.tm_year=d.year-1900;
   tf.tm_mon=d.month-1;
   tf.tm_mday=d.day;
   tf.tm_hour=t.hour;
   tf.tm_min=t.minute;
   tf.tm_sec=t.second;
   tzset();
   tf.tm_gmtoff=t.gmt_offset_h*3600+t.gmt_offset_m*60;
   tf.tm_isdst=-1;
   *utc=mktime(&tf);
   (*utc)-=timezone;
   if (tf.tm_isdst) (*utc)+=3600;
   (*utc)+=t.gmt_offset_h*3600+t.gmt_offset_m*60;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert UTC into local time and date.
    \param[in]  utc   UTC (seconds since 1.1.1970, 00:00:00, GMT-0)
    \param[out] d     local date
    \param[out] t     local time

    Convert UTC into local time and date.
 */
/*---------------------------------------------------------------------------*/
void Interfile::timeUTC2local(time_t utc, TIMEDATE::tdate * const d,
                              TIMEDATE::ttime * const t)
 { tm *tf;

   tf=localtime(&utc);
   d->year=1900+tf->tm_year;
   d->month=tf->tm_mon+1;
   d->day=tf->tm_mday;
   t->hour=tf->tm_hour;
   t->minute=tf->tm_min;
   t->second=tf->tm_sec;
   t->gmt_offset_h=tf->tm_gmtoff/3600;
   t->gmt_offset_m=(tf->tm_gmtoff-t->gmt_offset_h*3600)/60;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create entry in key/value list for line from file.
    \param[in] short_key   name of key without index and unit
    \param[in] unit        unit for key
    \param[in] index       string with index
    \param[in] value       value of key
    \param[in] comment     comment of key
    \param[in] token       parser table for header
    \param[in] kv          key/value list
    \param[in] filename    name of Interfile header
    \exception REC_ITF_HEADER_ERROR_UNITFORMAT      format of unit is wrong
    \exception REC_ITF_HEADER_ERROR_VALUE_MISSING   value missing
    \exception REC_ITF_HEADER_ERROR_INDEX_TOO_SMALL index of array too small
    \exception REC_ITF_HEADER_ERROR_KEY_INVALID     key invalid

    Create entry in key/value list for line from file.
 */
/*---------------------------------------------------------------------------*/
void Interfile::tokenize(std::string short_key, std::string unit,
                         std::string idx, const std::string value,
                         const std::string comment,
                         std::vector <ttoken *> token, std::vector <KV *> *kv,
                         const std::string filename) const
 { std::string offset_str=std::string(), key_index;
   std::vector <ttoken *>::const_iterator it;
                                                  // remove brackets from index
   if (!idx.empty()) { idx=idx.substr(1, idx.length()-2);
                       key_index=short_key+"["+idx+"]";
                     }
    else key_index=short_key;
                                                    // convert key to lowercase
   std::transform(short_key.begin(), short_key.end(), short_key.begin(),
                  tolower);
                                              // special handling for TIME keys
   for (it=token.begin(); it != token.end(); ++it)
    { std::string k;

      k=(*it)->key;
      std::transform(k.begin(), k.end(), k.begin(), tolower);
      if (short_key == k)                                        // found key ?
       if ((*it)->type == KV::TIME)
        { std::string::size_type p;
                                               // separate unit and time offset
          if ((p=unit.find(' ')) != std::string::npos)
           { offset_str=unit.substr(p+1);
             std::transform(offset_str.begin(), offset_str.end(),
                            offset_str.begin(), tolower);
             unit=unit.substr(0, p);
           }
           else throw Exception(REC_ITF_HEADER_ERROR_UNITFORMAT,
                               "Parsing error in the file '#1'. The format of "
                               "the unit of the key '#2' is wrong"
                               ".").arg(filename).arg(key_index);
        }
    }
                                                      // check if unit is valid
   checkUnit(token, key_index, short_key, unit, filename);
                                                  // search key in parser table
   for (it=token.begin(); it != token.end(); ++it)
    { std::string k;

      k=(*it)->key;
      std::transform(k.begin(), k.end(), k.begin(), tolower);
      if (short_key == k)                                        // found key ?
       { if (value.empty())
          if ((*it)->need_value)
           throw Exception(REC_ITF_HEADER_ERROR_VALUE_MISSING,
                        "Parsing error in the file '#1'. The value for the key"
                        " '#2' is missing.").arg(filename).arg(key_index);
         switch ((*it)->type)
          { case KV::ASCII_A:             // key is of array with ascii strings
            case KV::USHRT_A:         // key is of array of unsigned short ints
            case KV::ULONG_A:          // key is of array of unsigned long ints
            case KV::ULONGLONG_A: // key is of array of unsigned long long ints
            case KV::UFLOAT_A:            // key if of array of unsigned floats
            case KV::DATASET_A:                 // key is of array of data sets
            case KV::DTDES_A:      // key is of array of data type descriptions
             { std::vector <KV *>::const_iterator s;
               unsigned long int index;
                                                                   // get index
               KV::parseNumber(&index,
                               std::numeric_limits <unsigned long int>::max(),
                               key_index, idx, filename);
               if (index < 1)
                throw Exception(REC_ITF_HEADER_ERROR_INDEX_TOO_SMALL,
                                "Parsing error in the file '#1'. The index of "
                                "the key '#2' is too small"
                                ".").arg(filename).arg(key_index);
                                            // search in key/value list for key
               for (s=kv->begin(); s != kv->end(); s++)
                if ((*s)->Key() == short_key)
                           // key found -> add new index with value and comment
                 { (*s)->add(unit, (unsigned short int)index, value, comment,
                    filename);
                   return;
                 }
                      // key not found -> add new key, index, value and comment
               kv->push_back(new KV((*it)->key, unit,
                                    (unsigned short int)index,
                                    (*it)->max_index_key, value, (*it)->type,
                                    comment, filename));
               return;
             }
            case KV::ASCII_AL:              // key is of array with ascii lists
            case KV::USHRT_AL: // key is of array with unsigned short int lists
            case KV::UFLOAT_AL:      // key is of array of unsigned float lists
             { std::vector <KV *>::const_iterator s;
               unsigned long int index;
                                                                   // get index
               KV::parseNumber(&index,
                               std::numeric_limits <unsigned long int>::max(),
                               key_index, idx, filename);
               if (index < 1)
                throw Exception(REC_ITF_HEADER_ERROR_INDEX_TOO_SMALL,
                                "Parsing error in the file '#1'. The index of "
                                "the key '#2' is too small"
                                ".").arg(filename).arg(key_index);
                                            // search in key/value list for key
               for (s=kv->begin(); s != kv->end(); s++)
                if ((*s)->Key() == short_key)
                           // key found -> add new index with value and comment
                 { (*s)->add(unit, (unsigned short int)index, value, comment,
                    filename);
                   return;
                 }
                      // key not found -> add new key, index, value and comment
               kv->push_back(new KV((*it)->key, unit,
                                    (unsigned short int)index,
                                    (*it)->max_lindex_key,
                                    (*it)->max_index_key, value, (*it)->type,
                                    comment, filename));
               return;
             }
            case KV::ASCII_L:                    // key is of list with strings
            case KV::USHRT_L:        // key is of list with unsigned short ints
            case KV::ULONG_L:         // key is of list with unsigned long ints
            case KV::UFLOAT_L:           // key is of list with unsigned floats
            case KV::FLOAT_L:              // key is of list with signed floats
             kv->push_back(new KV((*it)->key, unit, (*it)->max_index_key,
                                  value, (*it)->type, comment, filename));
             return;
            case KV::TIME:            // add new ket, value, offset and comment
             kv->push_back(new KV((*it)->key, unit, value, (*it)->type,
                                  offset_str, comment, filename));
             return;
            default:                          // add new key, value and comment
             kv->push_back(new KV((*it)->key, unit, value, (*it)->type,
                                  comment, filename));
             return;
          }
       }
    }
   throw Exception(REC_ITF_HEADER_ERROR_KEY_INVALID,
                   "Parsing error in the file '#1'. The key '#2' is invalid"
                   ".").arg(filename).arg(key_index);
 }

/*---------------------------------------------------------------------------*/
/* Main: request value from main header                                      */
/*       change value in main header or add value to header                  */
/*---------------------------------------------------------------------------*/
#define Main(var, key, type)\
type Interfile::Main_##var(std::string * const unit)\
 { KV *kv;\
\
   kv=searchKey(key, kv_smh, main_headerfile);\
   *unit=kv->getUnit();\
   return(kv->getValue<type>());\
 }\
\
unsigned long int Interfile::Main_##var(const type value,\
                                        std::string unit,\
                                        const std::string comment)\
 { setValue(key, value, unit, comment, main_headerfile,\
            REC_ITF_HEADER_ERROR_MAINHEADER_SET, "main ", &kv_smh, smh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* MainNoUnit: request value from main header                                */
/*             change value in main header or add value to header            */
/*             (for values that don't have a unit)                           */
/*---------------------------------------------------------------------------*/
#define MainNoUnit(var, key, type)\
type Interfile::Main_##var()\
 { return(getValue<type>(key, kv_smh, main_headerfile, main_headerpath));\
 }\
\
unsigned long int Interfile::Main_##var(const type value,\
                                        const std::string comment)\
 { setValue(key, value, comment, main_headerfile,\
            REC_ITF_HEADER_ERROR_MAINHEADER_SET, "main ", &kv_smh, smh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* MainArray: request value from key with array                              */
/*---------------------------------------------------------------------------*/
#define MainArray(var, key, type)\
type Interfile::Main_##var(const unsigned short int a_idx,\
                           std::string * const unit)\
 { KV *kv;\
\
   kv=searchKey(key, kv_smh, main_headerfile);\
   *unit=kv->getUnit();\
   return(kv->getValue<type>(a_idx));\
 }\
\
unsigned long int Interfile::Main_##var(const type value,\
                                        const unsigned short int a_idx,\
                                        const std::string unit,\
                                        const std::string comment)\
 { setValue(key, value, a_idx, unit, comment, main_headerfile,\
            REC_ITF_HEADER_ERROR_MAINHEADER_SET, "main ", &kv_smh, smh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* MainArrayNoUnit: request value from key with array                        */
/*                  (for values that don't have a unit)                      */
/*---------------------------------------------------------------------------*/
#define MainArrayNoUnit(var, key, type)\
type Interfile::Main_##var(const unsigned short int a_idx)\
 { return(getValue<type>(key, a_idx, kv_smh, main_headerfile,\
                         main_headerpath));\
 }\
\
unsigned long int Interfile::Main_##var(const type value,\
                                        const unsigned short int a_idx,\
                                        const std::string comment)\
 { setValue(key, value, a_idx, comment, main_headerfile,\
            REC_ITF_HEADER_ERROR_MAINHEADER_SET, "main ", &kv_smh, smh,\
            false);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* MainArrayRemove: remove array value                                       */
/*---------------------------------------------------------------------------*/
#define MainArrayRemove(var, key)\
unsigned long int Interfile::Main_##var(const unsigned short int a_idx)\
 { if (main_headerfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_MAINHEADER_REMOVE,\
                    "Can't remove key '#1' because main header doesn't exist"\
                    ".").arg(std::string(key)+"["+toString(a_idx+1)+"]");\
   KV *kv_it=NULL;\
\
   try { Logging::flog()->loggingOn(false);\
         kv_it=searchKey(key, kv_smh, main_headerfile);\
       }\
   catch (const Exception r)\
    { Logging::flog()->loggingOn(true);\
      if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING) return(0);\
    }\
   Logging::flog()->loggingOn(true);\
   if (kv_it != NULL)\
    if (kv_it->removeValue(a_idx, main_headerfile))\
     deleteKV(&kv_smh, kv_it, main_headerfile);\
   return(0);\
 }\
\
unsigned long int Interfile::Main_##var()\
 { if (main_headerfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_MAINHEADER_REMOVE,\
                    "Can't remove key '#1' because main header doesn't exist"\
                    ".").arg(key);\
   KV *kv_it=NULL;\
\
   try { Logging::flog()->loggingOn(false);\
         kv_it=searchKey(key, kv_smh, main_headerfile);\
       }\
   catch (const Exception r)\
    { Logging::flog()->loggingOn(true);\
      if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING) return(0);\
    }\
   Logging::flog()->loggingOn(true);\
   if (kv_it != NULL)\
    { kv_it->removeValues();\
      deleteKV(&kv_smh, kv_it, main_headerfile);\
    }\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* MainListNoUnit: request value from key with list of values                */
/*                 (for values that don't have a unit)                       */
/*---------------------------------------------------------------------------*/
#define MainListNoUnit(var, key, type)\
type Interfile::Main_##var(const unsigned short int a_idx)\
 { return(searchKey(key, kv_smh, norm_headerfile)->getValue<type>(a_idx));\
 }\
\
unsigned long int Interfile::Main_##var(const type value,\
                                        const unsigned short int l_idx,\
                                        const std::string comment)\
 { setValue(key, value, l_idx, comment, main_headerfile,\
            REC_ITF_HEADER_ERROR_MAINHEADER_SET, "main", &kv_smh, smh, true);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* MainSep: add separator key to key/value list                              */
/*---------------------------------------------------------------------------*/
#define MainSep(var, key)\
unsigned long int Interfile::Main_##var(const std::string comment)\
 { if (main_headerfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_MAINHEADER_SET,\
                    "Can't set key '#1' because main header doesn't exist"\
                    ".").arg(key);\
   separatorKey(key, comment, &kv_smh);\
   return(0);\
 }
                        // generate code for the following keys of main headers
MainSep(INTERFILE, "!INTERFILE")
MainNoUnit(comment, "%comment", std::string)
MainNoUnit(originating_system, "!originating system", std::string)
MainNoUnit(CPS_data_type, "%CPS data type", std::string)
MainNoUnit(CPS_version_number, "%CPS version number", std::string)
MainNoUnit(gantry_serial_number, "%gantry serial number", std::string)
MainSep(GENERAL_DATA, "!GENERAL DATA")
MainNoUnit(original_institution, "original institution", std::string)
MainNoUnit(data_description, "data description", std::string)
MainNoUnit(patient_name, "patient name", std::string)
MainNoUnit(patient_ID, "patient ID", std::string)
Main(patient_DOB, "%patient DOB", TIMEDATE::tdate)
MainNoUnit(patient_sex, "patient sex", Interfile::tsex)
Main(patient_height, "patient height", float)
Main(patient_weight, "patient weight", float)
MainNoUnit(study_ID, "study ID", std::string)
MainNoUnit(exam_type, "exam type", Interfile::texamtype)
Main(study_date, "%study date", TIMEDATE::tdate)
Main(study_time, "%study time", TIMEDATE::ttime)
Main(study_UTC, "%study UTC", unsigned long int)
MainNoUnit(isotope_name, "isotope name", std::string)
Main(isotope_gamma_halflife, "isotope gamma halflife", float)
MainNoUnit(isotope_branching_factor, "isotope branching factor", float)
MainNoUnit(radiopharmaceutical, "radiopharmaceutical", std::string)
Main(tracer_injection_date, "%tracer injection date", TIMEDATE::tdate)
Main(tracer_injection_time, "%tracer injection time", TIMEDATE::ttime)
Main(tracer_injection_UTC, "%tracer injection UTC", unsigned long int)
Main(relative_time_of_tracer_injection, "relative time of tracer injection",
     signed long int)
Main(tracer_activity_at_time_of_injection,
     "tracer activity at time of injection", float)
Main(injected_volume, "injected volume", float)
MainNoUnit(patient_orientation, "%patient orientation",
           Interfile::tpatient_orientation)
MainNoUnit(patient_scan_orientation, "%patient scan orientation",
           Interfile::tpatient_orientation)
MainNoUnit(scan_direction, "%scan direction", Interfile::tscan_direction)
Main(coincidence_window_width, "%coincidence window width", float)
MainNoUnit(septa_state, "septa state", Interfile::tsepta)
Main(gantry_tilt_angle, "gantry tilt angle", float)
Main(gantry_slew, "%gantry slew", float)
MainNoUnit(type_of_detector_motion, "%type of detector motion",
           Interfile::tdetector_motion)
MainSep(DATA_MATRIX_DESCRIPTION, "%DATA MATRIX DESCRIPTION")
MainNoUnit(number_of_time_frames, "number of time frames", unsigned short int)
MainNoUnit(number_of_horizontal_bed_offsets,
           "%number of horizontal bed offsets", unsigned short int)
MainNoUnit(number_of_time_windows, "number of time windows",
           unsigned short int)
MainNoUnit(number_of_energy_windows, "number of energy windows",
           unsigned short int)
MainArray(energy_window_lower_level, "%energy window lower level",
          unsigned short int)
MainArrayRemove(energy_window_lower_level_remove, "%energy window lower level")
MainArray(energy_window_upper_level, "%energy window upper level",
          unsigned short int)
MainArrayRemove(energy_window_upper_level_remove, "%energy window upper level")
MainNoUnit(number_of_emission_data_types, "%number of emission data types",
           unsigned short int)
MainArrayNoUnit(emission_data_type_description,
                "%emission data type description",
                Interfile::tdata_type_description)
MainArrayRemove(emission_data_type_description_remove,
                "%emission data type description")
MainNoUnit(number_of_transmission_data_types,
           "%number of transmission data types", unsigned short int)
MainArrayNoUnit(transmission_data_type_description,
                "%transmission data type description",
                Interfile::tdata_type_description)
MainArrayRemove(transmission_data_type_description_remove,
                "%transmission data type description")
MainNoUnit(number_of_TOF_time_bins, "%number of TOF time bins",
           unsigned short int)
Main(TOF_bin_width, "%TOF bin width", float)
Main(TOF_gaussian_fwhm, "%TOF gaussian fwhm", float)
MainSep(GLOBAL_SCANNER_CALIBRATION_FACTOR,
        "%GLOBAL SCANNER CALIBRATION FACTOR")
Main(scanner_quantification_factor, "%scanner quantification factor", float)
Main(calibration_date, "%calibration date", TIMEDATE::tdate)
Main(calibration_time, "%calibration time", TIMEDATE::ttime)
Main(calibration_UTC, "%calibration UTC", unsigned long int)
MainListNoUnit(dead_time_quantification_factor,
               "%dead time quantification factor", float)
MainSep(DATA_SET_DESCRIPTION, "%DATA SET DESCRIPTION")
MainNoUnit(total_number_of_data_sets, "!total number of data sets",
           unsigned short int)
MainArrayNoUnit(data_set, "%data set", Interfile::tdataset)
MainArrayRemove(data_set_remove, "%data set")
MainSep(SUPPLEMENTARY_ATTRIBUTES, "%SUPPLEMENTARY ATTRIBUTES")
MainNoUnit(PET_scanner_type, "!PET scanner type", Interfile::tscanner_type)
Main(transaxial_FOV_diameter, "transaxial FOV diameter", float)
MainNoUnit(number_of_rings, "number of rings", unsigned short int)
Main(distance_between_rings, "%distance between rings", float)
Main(bin_size, "%bin size", float)
MainNoUnit(source_model, "%source model", std::string)
MainNoUnit(source_serial, "%source serial", std::string)
MainNoUnit(source_shape, "%source shape", std::string)
MainNoUnit(source_radii, "%source radii", unsigned short int)
MainArray(source_radius, "%source radius", float)
MainArrayRemove(source_radius_remove, "%source radius")
Main(source_length, "%source length", float)
MainNoUnit(source_densities, "%source densities", unsigned short int)
MainArray(source_mu, "%source mu", float)
MainArrayRemove(source_mu_remove, "%source mu")
MainNoUnit(source_ratios, "%source ratios", unsigned short int)
MainArrayNoUnit(source_ratio, "%source ratio", float)
MainArrayRemove(source_ratio_remove, "%source ratio")

/*---------------------------------------------------------------------------*/
/* Norm: request value from norm header                                      */
/*       change value in norm header or add value to header                  */
/*---------------------------------------------------------------------------*/
#define Norm(var, key, type)\
type Interfile::Norm_##var(std::string * const unit)\
 { KV *kv;\
\
   kv=searchKey(key, kv_nh, norm_headerfile);\
   *unit=kv->getUnit();\
   return(kv->getValue<type>());\
 }\
\
unsigned long int Interfile::Norm_##var(const type value,\
                                        std::string unit,\
                                        const std::string comment)\
 { setValue(key, value, unit, comment, norm_headerfile,\
            REC_ITF_HEADER_ERROR_NORMHEADER_SET, "norm ", &kv_nh, nh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* NormNoUnit: request value from norm header                                */
/*             change value in norm header or add value to header            */
/*             (for values that don't have a unit)                           */
/*---------------------------------------------------------------------------*/
#define NormNoUnit(var, key, type)\
type Interfile::Norm_##var()\
 { return(getValue<type>(key, kv_nh, norm_headerfile, norm_headerpath));\
 }\
\
unsigned long int Interfile::Norm_##var(const type value,\
                                        const std::string comment)\
 { setValue(key, value, comment, norm_headerfile,\
            REC_ITF_HEADER_ERROR_NORMHEADER_SET, "norm ", &kv_nh, nh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* NormArray: request value from key with array                              */
/*---------------------------------------------------------------------------*/
#define NormArray(var, key, type)\
type Interfile::Norm_##var(const unsigned short int a_idx,\
                           std::string * const unit)\
 { KV *kv;\
\
   kv=searchKey(key, kv_nh, norm_headerfile);\
   *unit=kv->getUnit();\
   return(kv->getValue<type>(a_idx));\
 }\
\
unsigned long int Interfile::Norm_##var(const type value,\
                                        const unsigned short int a_idx,\
                                        const std::string unit,\
                                        const std::string comment)\
 { setValue(key, value, a_idx, unit, comment, norm_headerfile,\
            REC_ITF_HEADER_ERROR_NORMHEADER_SET, "norm ", &kv_nh, nh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* NormArrayNoUnit: request value from key with array                        */
/*                  (for values that don't have a unit)                      */
/*---------------------------------------------------------------------------*/
#define NormArrayNoUnit(var, key, type)\
type Interfile::Norm_##var(const unsigned short int a_idx)\
 { return(getValue<type>(key, a_idx, kv_nh, norm_headerfile,\
                         norm_headerpath));\
 }\
\
unsigned long int Interfile::Norm_##var(const type value,\
                                        const unsigned short int a_idx,\
                                        const std::string comment)\
 { setValue(key, value, a_idx, comment, norm_headerfile,\
            REC_ITF_HEADER_ERROR_NORMHEADER_SET, "norm ", &kv_nh, nh, false);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* NormArrayListsNoUnit: request value from key with array of lists          */
/*                       (for values that don't have a unit)                 */
/*---------------------------------------------------------------------------*/
#define NormArrayListsNoUnit(var, key, type)\
type Interfile::Norm_##var(const unsigned short int a_idx,\
                           const unsigned short int l_idx)\
 { return(searchKey(key, kv_nh, norm_headerfile)->getValue<type>(a_idx,\
                                                                 l_idx));\
 }\
\
unsigned long int Interfile::Norm_##var(const type value,\
                                        const unsigned short int a_idx,\
                                        const unsigned short int l_idx,\
                                        const std::string comment)\
 { setValue(key, value, a_idx, l_idx, comment, norm_headerfile,\
            REC_ITF_HEADER_ERROR_NORMHEADER_SET, "norm ", &kv_nh, nh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* NormArrayRemove: remove array value                                       */
/*---------------------------------------------------------------------------*/
#define NormArrayRemove(var, key)\
unsigned long int Interfile::Norm_##var(const unsigned short int a_idx)\
 { if (norm_headerfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_NORMHEADER_REMOVE,\
                    "Can't remove key '#1' because norm header doesn't exist"\
                    ".").arg(std::string(key)+"["+toString(a_idx+1)+"]");\
   KV *kv_it=NULL;\
\
   try { Logging::flog()->loggingOn(false);\
         kv_it=searchKey(key, kv_nh, norm_headerfile);\
       }\
   catch (const Exception r)\
    { Logging::flog()->loggingOn(true);\
      if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING) return(0);\
    }\
   Logging::flog()->loggingOn(true);\
   if (kv_it != NULL)\
    if (kv_it->removeValue(a_idx, norm_headerfile))\
     deleteKV(&kv_nh, kv_it, norm_headerfile);\
   return(0);\
 }\
\
unsigned long int Interfile::Norm_##var()\
 { if (norm_headerfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_NORMHEADER_REMOVE,\
                    "Can't remove key '#1' because norm header doesn't exist"\
                    ".").arg(key);\
   KV *kv_it=NULL;\
\
   try { Logging::flog()->loggingOn(false);\
         kv_it=searchKey(key, kv_nh, norm_headerfile);\
       }\
   catch (const Exception r)\
    { Logging::flog()->loggingOn(true);\
      if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING) return(0);\
    }\
   Logging::flog()->loggingOn(true);\
   if (kv_it != NULL)\
    { kv_it->removeValues();\
      deleteKV(&kv_nh, kv_it, norm_headerfile);\
    }\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* NormArrayListsRemove: remove array of lists value                         */
/*---------------------------------------------------------------------------*/
#define NormArrayListsRemove(var, key)\
NormArrayRemove(var, key)\
unsigned long int Interfile::Norm_##var(const unsigned short int a_idx,\
                                        const unsigned short int l_idx)\
 { if (norm_headerfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_NORMHEADER_REMOVE,\
                    "Can't remove key '#1[#2][3]' because norm header doesn't"\
                    " exist.").arg(key).arg(a_idx+1).arg(l_idx+1);\
   KV *kv_it=NULL;\
\
   try { Logging::flog()->loggingOn(false);\
         kv_it=searchKey(key, kv_nh, norm_headerfile);\
       }\
   catch (const Exception r)\
    { Logging::flog()->loggingOn(true);\
      if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING) return(0);\
    }\
   Logging::flog()->loggingOn(true);\
   if (kv_it != NULL)\
    if (kv_it->removeValue(a_idx, l_idx, norm_headerfile))\
     deleteKV(&kv_nh, kv_it, norm_headerfile);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* NormListNoUnit: request value from key with list of values                */
/*                 (for values that don't have a unit)                       */
/*---------------------------------------------------------------------------*/
#define NormListNoUnit(var, key, type)\
type Interfile::Norm_##var(const unsigned short int a_idx)\
 { return(searchKey(key, kv_nh, norm_headerfile)->getValue<type>(a_idx));\
 }\
\
unsigned long int Interfile::Norm_##var(const type value,\
                                        const unsigned short int l_idx,\
                                        const std::string comment)\
 { setValue(key, value, l_idx, comment, norm_headerfile,\
            REC_ITF_HEADER_ERROR_NORMHEADER_SET, "norm", &kv_nh, nh, true);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* NormSep: add separator key to key/value list                              */
/*---------------------------------------------------------------------------*/
#define NormSep(var, key)\
unsigned long int Interfile::Norm_##var(const std::string comment)\
 { if (norm_headerfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_NORMHEADER_SET,\
                    "Can't set key '#1' because norm header doesn't exist"\
                    ".").arg(key);\
   separatorKey(key, comment, &kv_nh);\
   return(0);\
 }
                        // generate code for the following keys of norm headers
NormSep(INTERFILE, "!INTERFILE")
NormNoUnit(comment, "%comment", std::string)
NormNoUnit(originating_system, "!originating system", std::string)
NormNoUnit(CPS_data_type, "%CPS data type", std::string)
NormNoUnit(CPS_version_number, "%CPS version number", std::string)
NormNoUnit(gantry_serial_number, "%gantry serial number", std::string)
NormSep(GENERAL_DATA, "!GENERAL DATA")
NormNoUnit(data_description, "data description", std::string)
NormNoUnit(name_of_data_file, "!name of data file", std::string)
Norm(expiration_date, "%expiration date", TIMEDATE::tdate)
Norm(expiration_time, "%expiration time", TIMEDATE::ttime)
Norm(expiration_UTC, "%expiration UTC", unsigned long int)
NormSep(GENERAL_IMAGE_DATA, "!GENERAL IMAGE DATA")
Norm(study_date, "%study date", TIMEDATE::tdate)
Norm(study_time, "%study time", TIMEDATE::ttime)
Norm(study_UTC, "%study UTC", unsigned long int)
NormNoUnit(image_data_byte_order, "image data byte order", Interfile::tendian)
NormNoUnit(PET_data_type, "!PET data type", Interfile::tpet_data_type)
NormNoUnit(data_format, "data format", std::string)
NormNoUnit(number_format, "number format", Interfile::tnumber_format)
NormNoUnit(number_of_bytes_per_pixel, "!number of bytes per pixel",
           unsigned short int)
NormSep(RAW_NORMALIZATION_SCANS_DESCRIPTION,
        "%RAW NORMALIZATION SCANS DESCRIPTION")
NormNoUnit(number_of_normalization_scans, "%number of normalization scans",
           unsigned short int)
NormArrayNoUnit(normalization_scan, "%normalization scan", std::string)
NormArrayNoUnit(isotope_name, "isotope name", std::string)
NormArrayNoUnit(radiopharmaceutical, "radiopharmaceutical", std::string)
NormArrayNoUnit(total_prompts, "total prompts", uint64)
NormArrayNoUnit(total_randoms, "%total randoms", uint64)
NormArrayNoUnit(total_net_trues, "%total net trues", int64)
NormArray(image_duration, "image duration", unsigned long int)
NormArrayNoUnit(number_of_bucket_rings, "%number of bucket-rings",
                unsigned short int)
NormArrayNoUnit(total_uncorrected_singles, "%total uncorrected singles",
                uint64)
NormSep(NORMALIZATION_COMPONENTS_DESCRIPTION,
        "%NORMALIZATION COMPONENTS DESCRIPTION")
NormNoUnit(number_of_normalization_components,
           "%number of normalization components", unsigned short int)
NormArrayNoUnit(normalization_component, "%normalization component",
                std::string)
NormArrayNoUnit(number_of_dimensions, "number of dimensions",
                unsigned short int)
NormArrayListsNoUnit(matrix_size, "%matrix size", unsigned short int)
NormArrayListsNoUnit(matrix_axis_label, "%matrix axis label", std::string)
NormArrayListsNoUnit(matrix_axis_unit, "%matrix axis unit", std::string)
NormArrayListsNoUnit(scale_factor, "%scale factor", float)
NormArrayNoUnit(data_offset_in_bytes, "data offset in bytes", uint64)
NormNoUnit(axial_compression, "%axial compression", unsigned short int)
NormNoUnit(maximum_ring_difference, "%maximum ring difference",
           unsigned short int)
NormNoUnit(number_of_rings, "number of rings", unsigned short int)
NormNoUnit(number_of_energy_windows, "number of energy windows",
           unsigned short int)
NormArray(energy_window_lower_level, "%energy window lower level",
          unsigned short int)
NormArray(energy_window_upper_level, "%energy window upper level",
          unsigned short int)
NormSep(GLOBAL_SCANNER_CALIBRATION_FACTOR,
        "%GLOBAL SCANNER CALIBRATION FACTOR")
Norm(scanner_quantification_factor, "%scanner quantification factor", float)
Norm(calibration_date, "%calibration date", TIMEDATE::tdate)
Norm(calibration_time, "%calibration time", TIMEDATE::ttime)
Norm(calibration_UTC, "%calibration UTC", unsigned long int)
NormListNoUnit(dead_time_quantification_factor,
               "%dead time quantification factor", float)
NormSep(DATA_SET_DESCRIPTION, "%DATA SET DESCRIPTION")
NormNoUnit(total_number_of_data_sets, "!total number of data sets",
           unsigned short int)
NormArrayNoUnit(data_set, "%data set", Interfile::tdataset)
NormArrayRemove(normalization_scan_remove, "%normalization scan")
NormArrayRemove(isotope_name_remove, "isotope name")
NormArrayRemove(radiopharmaceutical_remove, "radiopharmaceutical")
NormArrayRemove(total_prompts_remove, "total prompts")
NormArrayRemove(total_randoms_remove, "%total randoms")
NormArrayRemove(total_net_trues_remove, "%total net trues")
NormArrayRemove(image_duration_remove, "image duration")
NormArrayRemove(number_of_bucket_rings_remove, "%number of bucket-rings")
NormArrayRemove(total_uncorrected_singles_remove, "%total uncorrected singles")
NormArrayRemove(normalization_component_remove, "%normalization component")
NormArrayRemove(number_of_dimensions_remove, "number of dimensions")
NormArrayListsRemove(matrix_size_remove, "%matrix size")
NormArrayListsRemove(matrix_axis_label_remove, "%matrix axis label")
NormArrayListsRemove(matrix_axis_unit_remove, "%matrix axis unit")
NormArrayListsRemove(scale_factor_remove, "%scale factor")
NormArrayRemove(data_offset_in_bytes_remove, "data offset in bytes")
NormArrayRemove(energy_window_lower_level_remove, "%energy window lower level")
NormArrayRemove(energy_window_upper_level_remove, "%energy window upper level")
NormArrayRemove(data_set_remove, "%data set")

/*---------------------------------------------------------------------------*/
/* Sub: request value from subheader                                         */
/*      change value in subheader or add value to header                     */
/*---------------------------------------------------------------------------*/
#define Sub(var, key, type)\
    type Interfile::Sub_##var(std::string * const unit)\
 { KV *kv;\
\
   kv=searchKey(key, kv_ssh, subheaderfile);\
   *unit=kv->getUnit();\
   return(kv->getValue<type>());\
 }\
\
unsigned long int Interfile::Sub_##var(const type value,\
                                       const std::string unit,\
                                       const std::string comment)\
 { setValue(key, value, unit, comment, subheaderfile,\
            REC_ITF_HEADER_ERROR_SUBHEADER_SET, "sub", &kv_ssh, ssh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* SubNoUnit: request value from subheader                                   */
/*            change value in subheader or add value to header               */
/*            (for values that don't have a unit)                            */
/*---------------------------------------------------------------------------*/
#define SubNoUnit(var, key, type)\
type Interfile::Sub_##var()\
 { return(getValue<type>(key, kv_ssh, subheaderfile, subheaderpath));\
 }\
\
unsigned long int Interfile::Sub_##var(const type value,\
                                       const std::string comment)\
 { setValue(key, value, comment, subheaderfile,\
            REC_ITF_HEADER_ERROR_SUBHEADER_SET, "sub", &kv_ssh, ssh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* SubArray: request value from key with array of values                     */
/*---------------------------------------------------------------------------*/
#define SubArray(var, key, type)\
type Interfile::Sub_##var(const unsigned short int a_idx,\
                          std::string * const unit)\
 { KV *kv;\
\
   kv=searchKey(key, kv_ssh, subheaderfile);\
   *unit=kv->getUnit();\
   return(kv->getValue<type>(a_idx));\
 }\
\
unsigned long int Interfile::Sub_##var(const type value,\
                                       const unsigned short int a_idx,\
                                       const std::string unit,\
                                       const std::string comment)\
 { setValue(key, value, a_idx, unit, comment, subheaderfile,\
            REC_ITF_HEADER_ERROR_SUBHEADER_SET, "sub", &kv_ssh, ssh);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* SubArrayNoUnit: request value from key with array of values               */
/*            (for values that don't have a unit)                            */
/*---------------------------------------------------------------------------*/
#define SubArrayNoUnit(var, key, type)\
type Interfile::Sub_##var(const unsigned short int a_idx)\
 { return(getValue<type>(key, a_idx, kv_ssh, subheaderfile, subheaderpath));\
 }\
\
unsigned long int Interfile::Sub_##var(const type value,\
                                       const unsigned short int a_idx,\
                                       const std::string comment)\
 { setValue(key, value, a_idx, comment, subheaderfile,\
            REC_ITF_HEADER_ERROR_SUBHEADER_SET, "sub", &kv_ssh, ssh, false);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* SubArrayRemove: remove array value                                        */
/*---------------------------------------------------------------------------*/
#define SubArrayRemove(var, key)\
unsigned long int Interfile::Sub_##var(const unsigned short int a_idx)\
 { if (subheaderfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_SUBHEADER_REMOVE,\
                    "Can't remove key '#1' because subheader doesn't exist"\
                    ".").arg(std::string(key)+"["+toString(a_idx+1)+"]");\
   KV *kv_it=NULL;\
\
   try { Logging::flog()->loggingOn(false);\
         kv_it=searchKey(key, kv_ssh, subheaderfile);\
       }\
   catch (const Exception r)\
    { Logging::flog()->loggingOn(true);\
      if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING) return(0);\
    }\
   Logging::flog()->loggingOn(true);\
   if (kv_it != NULL)\
    if (kv_it->removeValue(a_idx, subheaderfile))\
     deleteKV(&kv_ssh, kv_it, subheaderfile);\
   return(0);\
 }\
\
unsigned long int Interfile::Sub_##var()\
 { if (subheaderfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_SUBHEADER_REMOVE,\
                    "Can't remove key '#1' because subheader doesn't exist"\
                    ".").arg(key);\
   KV *kv_it=NULL;\
\
   try { Logging::flog()->loggingOn(false);\
         kv_it=searchKey(key, kv_ssh, subheaderfile);\
       }\
   catch (const Exception r)\
    { Logging::flog()->loggingOn(true);\
      if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING) return(0);\
    }\
   Logging::flog()->loggingOn(true);\
   if (kv_it != NULL)\
    { kv_it->removeValues();\
      deleteKV(&kv_ssh, kv_it, subheaderfile);\
    }\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* SubListNoUnit: request value from key with list of values                 */
/*                (for values that don't have a unit)                        */
/*---------------------------------------------------------------------------*/
#define SubListNoUnit(var, key, type)\
type Interfile::Sub_##var(const unsigned short int a_idx)\
 { return(searchKey(key, kv_ssh, subheaderfile)->getValue<type>(a_idx));\
 }\
\
unsigned long int Interfile::Sub_##var(const type value,\
                                       const unsigned short int l_idx,\
                                       const std::string comment)\
 { setValue(key, value, l_idx, comment, subheaderfile,\
            REC_ITF_HEADER_ERROR_SUBHEADER_SET, "sub", &kv_ssh, ssh, true);\
   return(0);\
 }

/*---------------------------------------------------------------------------*/
/* SubSep: add separator key to key/value list                               */
/*---------------------------------------------------------------------------*/
#define SubSep(var, key)\
unsigned long int Interfile::Sub_##var(const std::string comment)\
 { if (subheaderfile.empty())\
    throw Exception(REC_ITF_HEADER_ERROR_SUBHEADER_SET,\
                    "Can't set key '#1' because subheader doesn't exist"\
                    ".").arg(key);\
   separatorKey(key, comment, &kv_ssh);\
   return(0);\
 }
                          // generate code for the following keys of subheaders
SubSep(INTERFILE, "!INTERFILE")
SubNoUnit(comment, "%comment", std::string)
SubNoUnit(originating_system, "!originating system", std::string)
SubNoUnit(CPS_data_type, "%CPS data type", std::string)
SubNoUnit(CPS_version_number, "%CPS version number", std::string)
SubSep(GENERAL_DATA, "!GENERAL DATA")
SubNoUnit(name_of_listmode_file, "%name of listmode file", std::string)
SubNoUnit(name_of_sinogram_file, "%name of sinogram file", std::string)
SubNoUnit(name_of_data_file, "!name of data file", std::string)
SubSep(GENERAL_IMAGE_DATA, "!GENERAL IMAGE DATA")
SubNoUnit(type_of_data, "!type of data", std::string)
Sub(study_date, "%study date", TIMEDATE::tdate)
Sub(study_time, "%study time", TIMEDATE::ttime)
Sub(study_UTC, "%study UTC", unsigned long int)
SubNoUnit(image_data_byte_order, "image data byte order", Interfile::tendian)
SubNoUnit(PET_data_type, "!PET data type", Interfile::tpet_data_type)
SubNoUnit(data_format, "data format", std::string)
SubNoUnit(number_format, "number format", Interfile::tnumber_format)
SubNoUnit(number_of_bytes_per_pixel, "!number of bytes per pixel",
          unsigned short int)
SubNoUnit(image_scale_factor, "!image scale factor", float)
SubNoUnit(number_of_dimensions, "number of dimensions", unsigned short int)
SubArrayNoUnit(matrix_axis_label, "matrix axis label", std::string)
SubArrayNoUnit(matrix_size, "matrix size", unsigned short int)
SubArray(scale_factor, "scale factor", float)
Sub(start_horizontal_bed_position, "start horizontal bed position", float)
Sub(start_vertical_bed_position, "start vertical bed position", float)
Sub(reconstruction_diameter, "%reconstruction diameter", float)
SubNoUnit(reconstruction_bins, "%reconstruction bins", unsigned short int)
SubNoUnit(reconstruction_views, "%reconstruction views", unsigned short int)
SubNoUnit(process_status, "process status", Interfile::tprocess_status)
SubNoUnit(quantification_units, "quantification units", std::string)
SubNoUnit(decay_correction, "%decay correction", Interfile::tdecay_correction)
SubNoUnit(decay_correction_factor, "decay correction factor", float)
SubNoUnit(scatter_fraction_factor, "%scatter fraction factor", float)
SubListNoUnit(dead_time_factor, "%dead time factor", float)
SubNoUnit(slice_orientation, "slice orientation",
          Interfile::tslice_orientation)
SubNoUnit(method_of_reconstruction, "method of reconstruction", std::string)
SubNoUnit(number_of_iterations, "number of iterations", unsigned short int)
SubNoUnit(number_of_subsets, "%number of subsets", unsigned short int)
SubNoUnit(stopping_criteria, "stopping criteria", std::string)
SubNoUnit(filter_name, "filter name", std::string)
Sub(xy_filter, "%xy-filter", float)
Sub(z_filter, "%z-filter", float)
SubNoUnit(image_zoom, "%image zoom", float)
Sub(x_offset, "%x-offset", float)
Sub(y_offset, "%y-offset", float)
SubNoUnit(number_of_scan_data_types, "number of scan data types",
          unsigned short int)
SubArrayNoUnit(scan_data_type_description, "scan data type description",
               Interfile::tdata_type_description)
SubArrayNoUnit(data_offset_in_bytes, "data offset in bytes", unsigned long int)
SubNoUnit(angular_compression, "angular compression", unsigned short int)
SubNoUnit(axial_compression, "%axial compression", unsigned short int)
SubNoUnit(maximum_ring_difference, "%maximum ring difference",
          unsigned short int)
SubNoUnit(number_of_segments, "%number of segments", unsigned short int)
SubListNoUnit(segment_table, "%segment table", unsigned short int)
SubListNoUnit(applied_corrections, "applied corrections", std::string)
SubNoUnit(method_of_attenuation_correction, "method of attenuation correction",
          std::string)
SubNoUnit(method_of_scatter_correction, "method of scatter correction",
          std::string)
SubNoUnit(method_of_random_correction, "%method of random correction",
          Interfile::trandom_correction)
SubSep(IMAGE_DATA_DESCRIPTION, "!IMAGE DATA DESCRIPTION")
SubNoUnit(total_number_of_data_sets, "!total number of data sets",
          unsigned short int)
SubNoUnit(acquisition_start_condition, "%acquisition start condition",
          Interfile::tacqcondition)
SubNoUnit(acquisition_termination_condition,
          "%acquisition termination condition", Interfile::tacqcondition)
Sub(image_duration, "!image duration", unsigned long int)
Sub(image_relative_start_time, "image relative start time", unsigned long int)
SubNoUnit(total_prompts, "total prompts", uint64)
SubNoUnit(total_randoms, "%total randoms", uint64)
SubNoUnit(total_net_trues, "%total net trues", int64)
SubNoUnit(total_uncorrected_singles, "%total uncorrected singles", uint64)
SubNoUnit(number_of_bucket_rings, "%number of bucket-rings",
          unsigned short int)
SubArrayNoUnit(bucket_ring_singles, "%bucket-ring singles", uint64)
SubSep(SUPPLEMENTARY_ATTRIBUTES, "%SUPPLEMENTARY ATTRIBUTES")
SubNoUnit(number_of_projections, "%number of projections", unsigned short int)
SubNoUnit(number_of_views, "%number of views", unsigned short int)
SubArrayRemove(applied_corrections_remove, "applied corrections")
SubArrayRemove(matrix_axis_label_remove, "matrix axis label")
SubArrayRemove(matrix_size_remove, "matrix size")
SubArrayRemove(scale_factor_remove, "scale factor")
SubArrayRemove(scan_data_type_description_remove, "scan data type description")
SubArrayRemove(data_offset_in_bytes_remove, "data offset in bytes")
SubArrayRemove(segment_table_remove, "%segment table")
SubArrayRemove(bucket_ring_singles_remove, "%bucket-ring singles")
#endif
