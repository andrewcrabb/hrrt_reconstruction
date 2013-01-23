/*! \file mem_ctrl.h
    \brief This class is used for memory management.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/03/15 initial version
    \date 2004/05/03 added Doxygen style comments
    \date 2004/05/21 added version number to pattern file
    \date 2004/07/14 store the pattern file in the Windows user directory
    \date 2004/07/21 export MAX_BLOCK as public constant
    \date 2004/10/08 added unsigned long int memory blocks
    \date 2004/11/09 added "setMemLimit" method
    \date 2005/03/29 write access mode to log file in swapIn()
    \date 2005/03/29 log swapping times
 */

#ifndef _MEM_CTRL_H
#define _MEM_CTRL_H

#include <string>
#include <vector>
#include <ctime>
#include "semaphore_al.h"
#include "types.h"

/*- class definitions -------------------------------------------------------*/

class MemCtrl
 { public:
                                  /*! supported data types for memory blocks */
    typedef enum { DT_FLT,                                   /*!< float data */
                   DT_UINT,                      /*!< unsigned long int data */
                   DT_SINT,                       /*!< signed short int data */
                   DT_CHAR                             /*!< signed char data */
                 } tdatatype;
   private:
                                     /*! path for memory access pattern file */
    static const std::string pattern_path;
                                         /*! version number for pattern file */
    static const unsigned short int MAPFILE_VERSION;
           /*! maximum number of resynchronisation trials for pattern lookup */
    static const unsigned short int MAX_PATTERN_RESYNC;
                                        /*! data structure for memory blocks */
    typedef struct { float *fdata;                /*!< pointer to float data */
                                   /*! pointer to unsigned long integer data */
                     uint32 *uidata;
                                   /*! pointer to signed short interger data */
                     signed short int *sidata;
                     char *cdata;                  /*!< pointer to char data */
                     tdatatype dt;       /*!< data type stored in this block */
                          /*! number of times block is currently checked out */
                     unsigned short int checked_out;
                                      /*! number of elements in memory block */
                     unsigned long int size;
                               /*! is block checked out in read/write mode ? */
                     bool checked_out_rw,
                          block_allocated,            /*!< is block in use ? */
                                    /*! is data stored in file (or memory) ? */
                          stored_in_file;
                                /*! semaphore to lock access to memory block */
                     Semaphore *block_lock;
                     time_t last_access;            /*!< time of last access */
                                             /*! name of swap file for block */
                     std::string swap_filename;
                   } tmem;
    static MemCtrl *instance;    /*!< pointer to only instance of this class */
    signed long int free_mem;       /*!< amount of available memory in KByte */
                                                     // statistical information
    uint64 swapped_in,                       /*!< amount of bytes swapped in */
           swapped_out;                     /*!< amount of bytes swapped out */
    std::vector <tmem> mem;                       /*!< list of memory blocks */
                                           /*! current memory access pattern */
    std::vector <unsigned short int> pattern,
         /*! memory access pattern from previous run that is used for lookup */
                                     lu_pattern;
    unsigned short int lu_ptr,              /*!< pointer into lookup pattern */
                                    /*! number of resynchronization attempts */
                       resync_pattern_attempt;
    std::string swap_path,                          /*!< path for swap files */
                pattern_key,      /*!< key for current memory access pattern */
                pattern_filename;    /*!< filename for memory access pattern */
    Semaphore *lock;  /*!< semaphore to lock access to list of memory blocks */
    float swap_out_time,                      /*!< time used to swap to disk */
          swap_in_time;                     /*!< time used to swap from disk */
    template <typename T>                                    // allocate memory
     void allocMemory(T **, const unsigned long int,
                      const unsigned short int);
                                        // convert memory block to float format
    void convertToFloat(const unsigned short int, const unsigned short int);
                                                         // delete memory block
    void delete_block(unsigned short int * const, const bool);
    void freeMem(const signed long int);       // modify counter of free memory
                                          // save current memory access pattern
    void savePattern(const unsigned short int) const;
                                                      // swap block into memory
    void swapIn(const unsigned short int, const std::string,
                const unsigned short int);
    bool swapOut(const unsigned short int);         // swap block out of memory
    void signalBlock(const unsigned short int) const;    // unlock memory block
                                        // update current memory access pattern
    void updatePattern(const unsigned short int, const unsigned short int);
    void waitBlock(const unsigned short int) const;        // lock memory block
   protected:
    MemCtrl();                                                   // constructor
    ~MemCtrl();                                                   // destructor
   public:
                                         /*! maximum number of memory blocks */
    static const unsigned short int MAX_BLOCK;
                              // create block of char and get read/write access
    char *createChar(const unsigned long int, unsigned short int * const,
                     const std::string, const unsigned short int loglevel);
                             // create block of float and get read/write access
    float *createFloat(const unsigned long int, unsigned short int * const,
                       const std::string, const unsigned short int loglevel);
                  // create block of signed short int and get read/write access
    signed short int *createSInt(const unsigned long int,
                                 unsigned short int * const, const std::string,
                                 const unsigned short int);
                 // create block of unsigned long int and get read/write access
    uint32 *createUInt(const unsigned long int, unsigned short int * const,
                       const std::string, const unsigned short int);
                                                  // request data type of block
    tdatatype dataType(const unsigned short int) const;
    void deleteBlock(unsigned short int * const);      // delete a memory block
                                       // delete a memory block and ignore lock
    void deleteBlockForce(unsigned short int * const);
    static void deleteMemoryController();           // delete instace of object
                                      // get read/write access to block of char
    char *getChar(const unsigned short int, const unsigned short int);
                                       // get read-only access to block of char
    char *getCharRO(const unsigned short int, const unsigned short int);
                                     // get read/write access to block of float
    float *getFloat(const unsigned short int, const unsigned short int);
                                      // get read-only access to block of float
    float *getFloatRO(const unsigned short int, const unsigned short int);
                                               // remove char block from system
    char *getRemoveChar(unsigned short int * const, const unsigned short int);
                                              // remove float block from system
    float *getRemoveFloat(unsigned short int * const,
                          const unsigned short int);
                                   // remove signed short int block from system
    signed short int *getRemoveSInt(unsigned short int * const,
                                    const unsigned short int);
                                  // remove unsigned long int block from system
    uint32 *getRemoveUInt(unsigned short int * const,
                          const unsigned short int);
                             // get read/write access to signed short int block
    signed short int *getSInt(const unsigned short int,
                              const unsigned short int);
                              // get read-only access to signed short int block
    signed short int *getSIntRO(const unsigned short int,
                                const unsigned short int);
                            // get read/write access to unsigned long int block
    uint32 *getUInt(const unsigned short int, const unsigned short int);
                             // get read-only access to unsigned long int block
    uint32 *getUIntRO(const unsigned short int, const unsigned short int);
    static MemCtrl *mc();       // get pointer to instance of memory controller
                                 // write information about memory usage to log
    void printPattern(const unsigned short int) const;
    void put(const unsigned short int);       // return access rights for block
                       // put float block into system and give up access rights
    void putFloat(float **, const unsigned long int,
                  unsigned short int * const, const std::string,
                  const unsigned short int);
                           // search for matching memory access pattern in file
    void searchForPattern(const std::string, const unsigned short int);
    void setMemLimit(const float);             // set memory limit for swapping
    void setSwappingPath(const std::string);      // change path for swap files
    std::string swappingPath() const;            // request path for swap files
 };

#ifndef _MEM_CTRL_CPP
#define _MEM_CTRL_TMPL_CPP
#include "mem_ctrl.cpp"
#endif

#endif
