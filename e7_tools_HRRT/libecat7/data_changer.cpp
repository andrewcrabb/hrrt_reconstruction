/*! \class DataChanger data_changer.h "data_changer.h"
    \brief This class implements system independent code to read from big- or
           little-endian files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    The data is copied from file into an internal buffer that uses the same
    data representation. When the user retrieves the values, the data is
    converted into the format of the host. If he puts values into the buffer,
    the data is converted into the format used in the file.
 */

/*! \example example_datachanger.cpp
    Here is an example of how to use the DataChanger class.
 */

#include <cstdlib>
#include "data_changer.h"
#include "exception.h"
#include "swap_tmpl.h"
#include <string.h>

/*! \brief Create buffer for data.
    \param[in] ibuffer_size        size of buffer in bytes
    \param[in] ivax_format         store(d) data in VAX format ?
    \param[in] big_endian_format   store(d) data in big endian format ?

    Create buffer for data.
 */
// ahc ivax_format was always false, big_endian_format was always true.
// DataChanger::DataChanger(const unsigned long int ibuffer_size,  const bool ivax_format,  const bool big_endian_format) { 
DataChanger::DataChanger(long int ibuffer_size) {
  // swap_data=(big_endian_format != BigEndianMachine());
  swap_data = !BigEndianMachine();
   // vax_format=ivax_format;
   buffer_size=ibuffer_size;
   buffer=new unsigned char[buffer_size]; // buffer for system independent data
   ResetBufferPtr();                      // pointer to the start of the buffer
   bufend=buffer+buffer_size;               // pointer to the end of the buffer
 }

/*! \brief Destroy buffer.

    Destroy buffer.
 */
DataChanger::~DataChanger()
 { delete[] buffer;
 }

/*! \brief Load buffer content from file.
    \param[in,out] source   file handle
    \exception REC_FILE_TOO_SMALL   file doesn't contain enough data

    Load buffer content from file.
 */
void DataChanger::LoadBuffer(std::ifstream * const source)
 { unsigned long int size;

   size=buffer_size*sizeof(unsigned char);
   memset(buffer, 0, size*sizeof(char));
   source->read((char *)buffer, size);
   if ((unsigned long int)source->gcount() != size)
    throw Exception(REC_FILE_TOO_SMALL, "File doesn't contain enough data.");
 }

/*! \brief Reset pointer to start of buffer.

    Reset pointer to start of buffer.
 */
void DataChanger::ResetBufferPtr()
 { bufptr=buffer;
 }

/*! \brief Store buffer content in file.
    \param[in,out] dest   file handle

    Store buffer content in file.
 */
void DataChanger::SaveBuffer(std::ofstream * const dest) const
 { dest->write((char *)buffer, sizeof(unsigned char)*buffer_size);
 }

/*! \brief Read "unsigned char" value from buffer.
    \param[out] dest   "unsigned char" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Read "unsigned char" value from buffer.
 */
void DataChanger::Value(unsigned char * const dest)
 { if (bufptr >= bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   *dest=(unsigned char)*bufptr++;
 }

/*! \brief Write "unsigned char" value to buffer.
    \param[in] source   "unsigned char" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Write "unsigned char" value to buffer.
 */
void DataChanger::Value(const unsigned char source)
 { if (bufptr >= bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   *bufptr++=source;
 }

/*! \brief Read "signed short int" value from buffer.
    \param[out] dest   "signed short int" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Read "signed short int" value from buffer.
 */
void DataChanger::Value(signed short int * const dest)
 { if (bufptr+2 > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   memcpy(dest, bufptr, sizeof(signed short int));
   bufptr += sizeof(signed short int);
   // if (swap_data ^ vax_format) Swap(dest);
   if (swap_data) 
    Swap(dest);
 }

/*! \brief Write "signed short int" value to buffer.
    \param[in] source   "signed short int" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Write "signed short int" value to buffer.
 */
void DataChanger::Value(const signed short int source)
 { if (bufptr+2 > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   // if (swap_data ^ vax_format) Swap(&source);
   if (swap_data) 
    Swap(&source);
   memcpy(bufptr, &source, sizeof(signed short int));
   bufptr += sizeof(signed short int);
 }

/*! \brief Read "unsigned short int" value from buffer.
    \param[out] dest   "unsigned short int" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Read "unsigned short int" value from buffer.
 */
void DataChanger::Value(unsigned short int * const dest)
 { Value((signed short int *)dest);
 }

/*! \brief Write "unsigned short int" value to buffer.
    \param[in] source   "unsigned short int" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Write "unsigned short int" value to buffer.
 */
void DataChanger::Value(const unsigned short int source)
 { Value((signed short int)source);
 }

/*! \brief Read "signed long int" value from buffer.
    \param[out] dest   "signed long int" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Read "signed long int" value from buffer.
 */
void DataChanger::Value(signed long int * const dest)
 { if (bufptr+sizeof(signed long int) > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   memcpy(dest, bufptr, sizeof(signed long int));
   bufptr += sizeof(signed long int);
   // if (swap_data ^ vax_format) Swap(dest);
   if (swap_data) 
    Swap(dest);
 }

/*! \brief Write "signed long int" value to buffer.
    \param[in] source   "signed long int" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Write "signed long int" value to buffer.
 */
void DataChanger::Value(const signed long int source)
 { if (bufptr+sizeof(signed long int) > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   // if (swap_data ^ vax_format) Swap(&source);
   if (swap_data) 
    Swap(dest);
   memcpy(bufptr, &source, sizeof(signed long int));
   bufptr += sizeof(signed long int);
 }

/*! \brief Read "unsigned long int" value from buffer.
    \param[out] dest   "unsigned long int" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Read "unsigned long int" value from buffer.
 */
void DataChanger::Value(unsigned long int * const dest)
 { Value((signed long int *)dest);
 }

/*! \brief Write "unsigned long int" value to buffer.
    \param[in] source   "unsigned long int" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Write "unsigned long int" value to buffer.
 */
void DataChanger::Value(const unsigned long int source)
 { Value((signed long int)source);
 }

/*! \brief Read "float" value from buffer.
    \param[out] dest   "float" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Read "float" value from buffer.
 */
void DataChanger::Value(float * const dest)
 { if (bufptr+sizeof(float) > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   memcpy(dest, bufptr, sizeof(float));
   bufptr += sizeof(float);
   // if (vax_format) { unsigned char *cp;
   //                   unsigned short int *sp, stmp, sign;
   //                                       // transform IEEE float into VAX float
   //                   cp=(unsigned char *)dest;
   //                   sp=(unsigned short int *)dest;
   //                   Swap(dest);
   //                   stmp=*sp; *sp=*(sp+1); *(sp+1)=stmp;
   //                   sign=*sp & 0x8000;
   //                   *sp<<=1;
   //                   if (*cp > 1) *cp-=2;
   //                    else *cp=0;
   //                   *sp>>=1;
   //                   *sp|=sign;
   //                 }
   // if (swap_data ^ vax_format) Swap(dest);
   if (swap_data) 
    Swap(dest);
 }

/*! \brief Write "float" value to buffer.
    \param[in] source   "float" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Write "float" value to buffer.
 */
void DataChanger::Value(const float source)
 { if (bufptr+sizeof(float) > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   if (swap_data) Swap(&source);
   memcpy(bufptr, &source, sizeof(float));
   // if (vax_format) { unsigned char *cp;
   //                   unsigned short int *sp, stmp, sign;
   //                                       // transform VAX float into IEEE float
   //                   cp=(unsigned char *)bufptr;
   //                   sp=(unsigned short int *)bufptr;
   //                   sign=*sp & 0x8000;
   //                   *sp<<=1;
   //                   if (*cp < 0xfe) *cp+=2;
   //                    else *cp=0xff;
   //                   *sp>>=1;
   //                   *sp|=sign;
   //                   stmp=*sp; *sp=*(sp+1); *(sp+1)=stmp;
   //                   Swap((float *)bufptr);
   //                 }
   bufptr += sizeof(float);
 }

/*! \brief Read "double" value from buffer.
    \param[out] dest   "double" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Read "double" value from buffer.
 */
void DataChanger::Value(double * const dest)
 { if (bufptr+sizeof(double) > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   memcpy(dest, bufptr, sizeof(double));
   bufptr += sizeof(double);
   // if (vax_format)
   //  throw Exception(REC_VAX_DOUBLE, "VAX double format not implemented.");
   if (swap_data) Swap(dest);
 }

/*! \brief Write "double" value to buffer.
    \param[in] source   "double" value
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Write "double" value to buffer.
 */
void DataChanger::Value(const double source)
 { if (bufptr+sizeof(double) > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   if (swap_data) Swap(&source);
   memcpy(bufptr, &source, sizeof(double));
   // if (vax_format)
   //  throw Exception(REC_VAX_DOUBLE, "VAX double format not implemented.");
   bufptr += sizeof(double);
 }

/*! \brief Read "unsigned char" block from buffer.
    \param[in,out] dest   buffer for data
    \param[in]     len    number of bytes to read
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Read "unsigned char" block from buffer.
 */
void DataChanger::Value(unsigned char * const dest, const unsigned short int len) { 
  if (bufptr + len > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   memcpy(dest, bufptr, len);
   bufptr += len;
 }

/*! \brief Write "unsigned char" block to buffer.
    \param[in]     len    number of bytes to read
    \param[in,out] dest   buffer for data
    \exception REC_BUFFER_OVERRUN   data access outside of buffer

    Write "unsigned char" block to buffer.
 */
void DataChanger::Value(const unsigned short int len, const unsigned char * const source) { 
  if (bufptr + len > bufend)
    throw Exception(REC_BUFFER_OVERRUN, "Data access outside of buffer.");
   memcpy(bufptr, source, len);
   bufptr += len;
 }
