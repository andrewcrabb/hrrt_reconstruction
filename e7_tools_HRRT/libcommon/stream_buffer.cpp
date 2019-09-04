/*! \class StreamBuffer stream_buffer.h "stream_buffer.h"
    \brief This class implements a data buffer.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments
    \date 2005/03/17 added bufferDeleted() method

    This class implements a data buffer. The buffer is expanded as necessary.
    To reduce memory allocations, the buffer has a minimum size at
    initialization and additional memory is allocated in larger chunks.
 */

/*! \example example_stbuf.cpp
    Here are some examples of how to use the StreamBuffer class.
 */

#include <iostream>
#include "stream_buffer.h"
#include "exception.h"

/*- constants ---------------------------------------------------------------*/

                                    /*! allocate memory in chunks of 4 kbyte */
const unsigned short int StreamBuffer::RESIZE_INCR=4096;

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Create buffer.

    Create 1 byte buffer.
 */
/*---------------------------------------------------------------------------*/
StreamBuffer::StreamBuffer()
 { buffer=NULL;
   try
   { setMinBufferSize(1);
     buffer=new char[1];                                   // create new buffer
     cur_size=1;                                      // current size of buffer
     write_ptr=0;                                  // write pointer into buffer
     read_ptr=0;                                    // read pointer into buffer
   }
   catch (...)
    { if (buffer != NULL) { delete[] buffer;
                            buffer=NULL;
                          }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create buffer.
    \param[in] _min_buffer_size   minimum and initial buffer size

    Create buffer.
 */
/*---------------------------------------------------------------------------*/
StreamBuffer::StreamBuffer(const unsigned long int _min_buffer_size)
 { buffer=NULL;
   try
   { setMinBufferSize(_min_buffer_size);
     buffer=new char[min_buffer_size];                     // create new buffer
     cur_size=min_buffer_size;                        // current size of buffer
     write_ptr=0;                                  // write pointer into buffer
     read_ptr=0;                                    // read pointer into buffer
   }
   catch (...)
    { if (buffer != NULL) { delete[] buffer;
                            buffer=NULL;
                          }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy buffer.

    Destroy buffer.
 */
/*---------------------------------------------------------------------------*/
StreamBuffer::~StreamBuffer()
 { if (buffer != NULL) delete[] buffer;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write data into buffer.
    \param[in] data   pointer to data
    \return StreamBuffer object with new data

    Write data into buffer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
StreamBuffer & StreamBuffer::operator << (const T data)
 { writeData(&data, 1);
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Read data from buffer.
    \param[out] data   pointer to data
    \return StreamBuffer object

    Read data from buffer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
StreamBuffer & StreamBuffer::operator >> (T const &data)
 { readData(&data, 1);
   return(*this);
 }

#ifndef _STREAM_BUFFER_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Set data format to big-endian.
    \param[in] big   big-endian format ? (otherwise little-endian)

    Set data format to big-endian.
 */
/*---------------------------------------------------------------------------*/
void StreamBuffer::BigEndian(const bool big)
 { big_endian=big;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Is data in big-endian format ?
    \return data is in big-endian format (otherwise little-endian)

    Is data in big-endian format ?
 */
/*---------------------------------------------------------------------------*/
bool StreamBuffer::BigEndian() const
 { return(big_endian);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove buffer from object.  

    The buffer is removed from the object without deleting it.
 */
/*---------------------------------------------------------------------------*/
void StreamBuffer::bufferDeleted()
 { buffer=NULL;
   cur_size=0;
   write_ptr=0;
   read_ptr=0;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bytes in buffer.
    \return number of bytes

    Request number of bytes in buffer. This is not necessarily the real size of
    the buffer, since part of it may be unused.
 */
/*---------------------------------------------------------------------------*/
unsigned long int StreamBuffer::bufferSize() const
 { return(write_ptr);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Read data from buffer.
    \param[in,out] data   pointer to data
    \param[in]     size   number of data elements to read
    \exception REC_STREAM_EMPTY can't read data from stream buffer

    Read data from buffer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void StreamBuffer::readData(T * const data, const unsigned long int size)
 { if (testData(data, size))                                // data available ?
    { memcpy(data, buffer+read_ptr, size*sizeof(T));                // get data
      read_ptr+=size*sizeof(T);
      return;
    }
   throw Exception(REC_STREAM_EMPTY,
                   "Can't read data from stream buffer. Stream is empty.");
 }

#ifndef _STREAM_BUFFER_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Reset buffer to minimum size and reset read/write-pointers.

    Reset buffer to minimum size and reset read/write-pointers.
 */
/*---------------------------------------------------------------------------*/
void StreamBuffer::resetBuffer()
 { resizeBuffer(min_buffer_size);
   resetReadPtr();
   write_ptr=0;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Reset read pointer.

    Reset read pointer.
 */
/*---------------------------------------------------------------------------*/
void StreamBuffer::resetReadPtr()
 { read_ptr=0;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Resize buffer without destroying its content.
    \param[in] new_size   new size of buffer

    Resize buffer without destroying its content.
 */
/*---------------------------------------------------------------------------*/
void StreamBuffer::resizeBuffer(const unsigned long int new_size)
 { char *buf2=NULL;

   try
   { unsigned long int old_size;
         // If buffer is enlarged, more memory is requested than needed so the
         // next write operations don't need to allocate memory.
         // Shrinking is only done, if new size is smaller than min_buffer_size
     old_size=cur_size;
     if (new_size > cur_size) cur_size=new_size+RESIZE_INCR;
      else if ((new_size < min_buffer_size) &&
               (cur_size != min_buffer_size)) cur_size=min_buffer_size;
            else return;
     buf2=new char[cur_size];
     if (buffer != NULL)
      { if (old_size < cur_size) memcpy(buf2, buffer, old_size*sizeof(char));
         else memcpy(buf2, buffer, cur_size*sizeof(char));
        delete[] buffer;
      }
     buffer=buf2;
     buf2=NULL;
   }
   catch (...)
    { if (buf2 != NULL) delete[] buf2;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set minimum buffer size.
    \param[in] _min_buffer_size   minimum size of buffer

    Set minimum buffer size.
 */
/*---------------------------------------------------------------------------*/
void StreamBuffer::setMinBufferSize(const unsigned long int _min_buffer_size)
 { min_buffer_size=_min_buffer_size;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set write pointer.
    \param[in] _write_ptr   new write pointer

    Set write pointer.
 */
/*---------------------------------------------------------------------------*/
void StreamBuffer::setWritePtr(const unsigned long int _write_ptr)
 { write_ptr=_write_ptr;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to start of buffer.
    \return pointer to start of buffer

    Request pointer to start of buffer.
 */
/*---------------------------------------------------------------------------*/
char *StreamBuffer::start() const
 { return(buffer);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Test if specified number of data elements are left in buffer.
    \param[in] size   number of data elements
    \return requested amount of data is available

    Test if specified number of data elements are left in buffer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
bool StreamBuffer::testData(const T * const,
                            const unsigned long int size) const
 { return(read_ptr+size*sizeof(T) <= write_ptr);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write data to buffer.
    \param[in] data   pointer to data
    \param[in] size   number of data elements to write

    Write data to buffer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void StreamBuffer::writeData(const T * const data,
                             const unsigned long int size)
 { unsigned long int new_write_ptr;

   new_write_ptr=write_ptr+size*sizeof(T);
   resizeBuffer(new_write_ptr);
   memcpy(buffer+write_ptr, data, size*sizeof(T));
   write_ptr=new_write_ptr;
 }
