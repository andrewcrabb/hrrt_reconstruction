/*! \file stream_buffer.h
    \brief This class implements a data buffer.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments
    \date 2005/03/17 added bufferDeleted() method
 */

#pragma once

#include <cstring>
#include <string>

/*- class definitions -------------------------------------------------------*/

class StreamBuffer
 { private:
    static const unsigned short int RESIZE_INCR;  /*!< size of memory chunks */
    char *buffer;                                       /*!< buffer for data */
    unsigned long int min_buffer_size,              /*!< minimum buffer size */
                      cur_size,                     /*!< current buffer size */
                      write_ptr,               /*!< write pointer for buffer */
                      read_ptr;                 /*!< read pointer for buffer */
    bool big_endian;                        /*!< data in buffer big-endian ? */
   public:
    StreamBuffer();                                     // create 1 byte buffer
    StreamBuffer(const unsigned long int);                     // create buffer
    ~StreamBuffer();                                           // delete buffer
    template <typename T>
     StreamBuffer & operator << (const T);              // write data to buffer
    template <typename T>
     StreamBuffer & operator >> (T const &);           // read data from buffer
    void BigEndian(const bool);                  // data in buffer big-endian ?
    bool BigEndian() const;
    void bufferDeleted();                          // remove buffer from object
    unsigned long int bufferSize() const;  // request number of bytes in buffer
    template <typename T>
     void readData(T * const, const unsigned long int);// read data from buffer
    void resetBuffer();                                         // reset buffer
    void resetReadPtr();                                  // reset read pointer
    void resizeBuffer(const unsigned long int);        // change size of buffer
                                                  // set minimum size of buffer
    void setMinBufferSize(const unsigned long int);
    void setWritePtr(const unsigned long int);             // set write pointer
    char *start() const;                  // request pointer to start of buffer
    template <typename T>                                // test size of buffer
     bool testData(const T * const, const unsigned long int) const;
    template <typename T>                               // write data to buffer
     void writeData(const T * const , const unsigned long int);
 };
