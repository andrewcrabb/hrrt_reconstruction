/*! \file raw_io.h
    \brief This template implements an object used for parallel file-I/O.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/01/26 added function to determine the size of a file
    \date 2004/05/28 added Doxygen style comments
    \date 2004/10/15 added uint32 datatype
    \date 2005/01/15 simplified multi-threading of template
    \date 2005/01/15 added double datatype
 */

#ifndef _RAW_IO_H
#define _RAW_IO_H

#include <cstdio>
#include <string>
#ifdef XEON_HYPERTHREADING_BUG
#include "e7_tools_const.h"
#endif
#include "semaphore_al.h"
#include "thread_wrapper.h"
#include "types.h"

/*- template definitions ----------------------------------------------------*/

template <typename T> class RawIO
 {                            // declare wrapper functions as friends and allow
                              // them to call private methods of the class
    friend void *executeThread_ReadFileOffs(void *);
    friend void *executeThread_WriteFile(void *);
   protected:
                            /*! parameters for thread that reads/writes data */
    typedef struct { std::string type_name;       /*!< type name of template */
                     RawIO <T> *object;         /*!< pointer to RawIO object */
                     T *data;                           /*!< pointer to data */
                     uint64 size, /*!< number of data elements to read/write */
                            offset;  /*!< number of data elements to skip at
                                          beginning of file while reading
                                      */
                     bool use_offset;       /*!</ use offset while reading ? */
                                 /*! number of blocks to split read/write in */
                     unsigned short int blocks;
#ifdef XEON_HYPERTHREADING_BUG
                     unsigned short int thread_number; /*!< number of thread */
                                  /*! padding bytes to avoid cache conflicts */
                     char padding[2*CACHE_LINE_SIZE-
                                  2*sizeof(unsigned short int)-sizeof(bool)-
                                  2*sizeof(uint64)-sizeof(RawIO <T> *)-
                                  sizeof(T *)];
#endif
                   } tthread_params;
   private:
    FILE *file;                              /*!< pointer to read/write file */
    bool write_object;                   /*!< use object for writing files ? */
    tthread_id tid;                               /*!< ID of file-I/O-thread */
    tthread_params tp;        /*!< parameters of thread to read/write values */
    std::string filename;                                  /*!< name of file */
                               /*! semaphore used to signal "block finished" */
    Semaphore *block_finished_sem;
    bool thread_running,                            /*!< is thread running ? */
         swap_data;                 /*!< swap data when reading or writing ? */
                      // read data from file in blocks starting at given offset
    void readFile(const uint64, const bool, T *, const uint64,
                  const unsigned short int);
                      // read data from file by thread starting at given offset
    void readPar(const uint64, const bool, T * const, const uint64,
                 const unsigned short int);
                                                // write data to file by thread
    void writeFile(T *, const uint64, const unsigned short int);
   public:
    RawIO(const std::string, const bool, const bool);            // constructor
    ~RawIO();                                                     // destructor
    void read(T * const, const uint64);                  // read data from file
                                      // read data from file starting at offset
    void read(const uint64, T * const, const uint64);
    void readPar(T * const, const uint64);     // read data from file by thread
                                     // read data from file in blocks by thread
    void readPar(T * const, const uint64, const unsigned short int);
                  // read data from file by thread in blocks starting at offset
    void readPar(const uint64, T * const, const uint64);
                            // read data from file by thread starting at offset
    void readPar(const uint64, T * const, const uint64,
                 const unsigned short int);
                                         // seek relative from current position
    void seekr(const signed long long int) const;
    uint64 size() const;                              // determine size of file
    bool tryWait() const;                    // check if next block is finished
    void wait() const;                     // wait until next block is finished
    void write(T * const, const uint64);                  // write data to file
                                                // write data to file by thread
    void writePar(T * const, const uint64);
                                      // write data to file in blocks by thread
    void writePar(T * const, const uint64, const unsigned short int);
    /*
	PMB
    */
    void *executeThread_ReadFileOffs(void *);
    void *executeThread_WriteFile(void *);
 };

#ifndef _RAW_IO_CPP
#define _RAW_IO_TMPL_CPP
#include "raw_io.cpp"
#endif

#endif
