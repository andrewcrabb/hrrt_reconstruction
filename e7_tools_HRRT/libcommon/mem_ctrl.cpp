/*! \class MemCtrl mem_ctrl.h "mem_ctrl.h"
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

    This object maintains a list of 200 memory blocks. The blocks can have any
    size and contain float, char, signed short int or unsigned long int values.
    The application using this class can initialize (create) blocks, release
    (delete) blocks, check blocks out for read/write or read-only access and
    check them back in. When a block is created, the application gets an index
    which is used to address this block.

    The overall amount of RAM that is used by this class is counted in the
    \em free_memory variable. The maximum amount is limited by the physical RAM
    of the computer and the fact that the application may allocate RAM outside
    from this class. Therefore the amount of RAM used by this class is limited
    to 80% of the physical RAM, leaving 20% to be used by normal "new/delete"
    calls in the application, before the swapping mechanism of the operating
    system kicks in (leaving the presence of other application out of the
    equation).

    Operating systems have a limited address space of 2 GByte for Windows,
    4 GByte for Mac OS X or 2, 4 or 36 GByte for Linux - depending on the
    Kernel. This limits the amount of memory an application can handle (in RAM
    and swap file). If this barrier is hit, the application usually aborts with
    an error message. If such a limit is reached when allocating memory with
    this class however, memory blocks are swapped to disk and the application
    continues.

    The described operating systems limitations are regarding the amount of
    data that this class can keep in memory at one point in time, not regarding
    the amount of memory (in RAM and on disk) it can utilize. The overall
    amount of memory is only limited by the number of memory blocks and the
    available disk space, not by the limitations of the operating system. If
    the class has to handle more memory than fits into the managed RAM, the
    class automatically swaps parts of the data to disk. Each block is written
    into its own swap-file with a filename consisting of a string given by the
    application, the block number, the computer name and the time of creation.

    The decision which block to swap-out is made with the following algorithm:
    - if a block is written to file and later read again in "read-only" mode,
      the swap-file is still valid, since the data won't be modified.
      If such a "read-only" block is in memory, but not used, it can be
      deleted. It doesn't need to be written to file, since the file already
      exists.
    - if an application runs it uses the different memory blocks in a fixed
      order which is called a "memory access pattern". This pattern depends on
      the execution path of the program, which can be determined from the
      command line switches the user chooses. Every set of switches leads to a
      distinct "memory access pattern", whereas different sets of switches may
      lead to the same pattern.\\
      A pattern-key can be calculated for each set of switches, that specifies
      the memory pattern resulting from the switches. When an application runs,
      the pattern-key (calculated from the switches) and the memory access
      pattern (recorded during processing) are stored in a file. If the
      application runs again with the same pattern-key, the memory access
      pattern can be looked-up from the file. This way this class knows in
      advance which block will be needed at what time and can make a perfect
      decision on which block to swap-out.\\
      There may be situations when the look-up pattern doesn't match to the
      current processing. This can happen if the pattern-key has not a 1:1
      mapping to a memory access pattern, due to limitations in the
      calculation of the key. In this case the object detects the mismatch and
      tries to "resynchronize" the current pattern with the look-up pattern,
      by shifting the look-up pattern. If this happens more than 4 times, the
      swapping algorithms falls back to a LIFO scheme.
    - If both previous methods fail, a LIFO scheme is used. Here the block
      which was accessed last, is swapped out first.

    The class is implemented as a singleton object. Only one instance of this
    class can exist in a given application. The class is thread safe.
 */

/*! \example example_memctrl.cpp
    Here are some examples of how to use the MemCtrl class.
 */

#define MACOSX 1

#include <iostream>
#include <algorithm>
#include <limits>
#include <fstream>
#include <vector>
#ifdef __MACOSX__
#include <mach/mach.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach-o/arch.h>
#endif
#if defined(__linux__) || defined(__SOLARIS__)
/* ahc */
/* #include <sys/sysinfo.h> */
#endif
#ifdef WIN32
#include <direct.h>
#include <lmcons.h>
#endif
#ifndef _MEM_CTRL_CPP
#define _MEM_CTRL_CPP
#include "mem_ctrl.h"
#endif
#include "e7_common.h"
#include "exception.h"
#ifdef WIN32
#include "global_tmpl.h"
#endif
#include "logging.h"
#include "raw_io.h"
#include "stopwatch.h"
#include "unique_name.h"

#ifndef _MEM_CTRL_TMPL_CPP
/*- constants ---------------------------------------------------------------*/

                                         /*! version number for pattern file */
const unsigned short int MemCtrl::MAPFILE_VERSION=3;
                                         /*! maximum number of memory blocks */
const unsigned short int MemCtrl::MAX_BLOCK=200;
           /*! maximum number of resynchronization trials for pattern lookup */
const unsigned short int MemCtrl::MAX_PATTERN_RESYNC=4;

                                     /*! path for memory access pattern file */
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
const std::string MemCtrl::pattern_path="~";
#endif
#ifdef WIN32
const std::string MemCtrl::pattern_path="C:\\Documents and Settings\\";
#endif

MemCtrl *MemCtrl::instance=NULL;     /*!< pointer to only instance of object */

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \exception REC_HOME_UNKNOWN   environment variable HOME in Unix not set

    Determine the amount of RAM in the host system and initialize data
    structures. Since not all memory used by an application is managed by this
    class, only 80% of the available RAM are used before the swapping mechanism
    of the MemCtrl class starts. If the application uses more RAM than is
    available, the swapping mechanism of the operating system will start. The
    amount of memory an application can used is limited by the address space of
    the operating system. If the application grows larger, it will abort.
 */
/*---------------------------------------------------------------------------*/
MemCtrl::MemCtrl()
 { setMemLimit(0.8f);                      // set memory threshold for swapping
   mem.clear();
   lock=NULL;
   try
   { lock=new Semaphore(1);        // semaphore to control access to block list
     lu_pattern.clear();             // initialize lookup memory access pattern
     pattern.clear();               // initialize current memory access pattern
     pattern_key=std::string();        // key for current memory access pattern
     resync_pattern_attempt=0;          // number of resynchronization attempts
     swapped_in=0;                               // amount of bytes swapped out
     swapped_out=0;                               // amount of bytes swapped in
     swap_path=std::string();
     swap_in_time=0.0f;                          // time used to swap from disk
     swap_out_time=0.0f;                           // time used to swap to disk
                                            // initialize list of memory blocks
     mem.resize(MemCtrl::MAX_BLOCK);
     for (std::vector <tmem>::iterator it=mem.begin(); it != mem.end(); ++it)
      { (*it).fdata=NULL;
        (*it).uidata=NULL;
        (*it).sidata=NULL;
        (*it).cdata=NULL;
        (*it).block_allocated=false;
        (*it).checked_out=0;
        (*it).block_lock=NULL;
        (*it).swap_filename=std::string();
      }
                              // create filename for memory access pattern file
     pattern_filename=pattern_path;
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
     char *ptr;
                                                       // convert '~' into path
     if ((ptr=getenv("HOME")) == NULL)
      throw Exception(REC_HOME_UNKNOWN, "Environment variable HOME not set.");
     pattern_filename.replace(pattern_filename.find("~"), 1, std::string(ptr));
     pattern_filename+="/.";
#endif
#ifdef WIN32
     LPTSTR lpszSystemInfo;
     DWORD cchBuff=256;
     TCHAR tchBuffer[UNLEN+1];

     lpszSystemInfo=tchBuffer;
     GetUserName(lpszSystemInfo, &cchBuff);
     pattern_filename+=std::string(lpszSystemInfo)+"\\";
#endif
     pattern_filename+="ma_pattern.dat";
   }
   catch (...)
    { if (lock != NULL) { delete lock;
                          lock=NULL;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object, release memory and delete swap files.
 */
/*---------------------------------------------------------------------------*/
MemCtrl::~MemCtrl()
 { std::vector <tmem>::iterator it;

   while (mem.size() > 0)
    { it=mem.begin();
                                                              // release memory
      switch ((*it).dt)
       { case DT_FLT:
          if ((*it).fdata != NULL) delete[] (*it).fdata;
          break;
         case DT_UINT:
          if ((*it).uidata != NULL) delete[] (*it).uidata;
          break;
         case DT_SINT:
          if ((*it).sidata != NULL) delete[] (*it).sidata;
          break;
         case DT_CHAR:
          if ((*it).cdata != NULL) delete[] (*it).cdata;
          break;
       }
      unlink((*it).swap_filename.c_str());                  // remove swap file
                                                   // delete semaphore of block
      if ((*it).block_lock != NULL) delete (*it).block_lock;
      mem.erase(it);
    }
   mem.clear();
   if (lock != NULL) delete lock;             // delete semaphore of block list
   UniqueName::close();
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Allocate memory.
    \param[out] ptr        pointer to memory block
    \param[in]  size       size of memory block
    \param[in]  loglevel   level for logging
    \exception REC_OUT_OF_MEMORY   memory exhausted

    Allocate memory. If the memory can't be allocated, a memory block is
    swapped out and the allocation is tried again. This is repeated until
    the memory is allocated successfully or no more memory blocks can be
    swapped out.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void MemCtrl::allocMemory(T **ptr, const unsigned long int size,
                          const unsigned short int loglevel)
 { for (;;)
    { try
      { Logging::flog()->loggingOn(false);
        *ptr=new T[size];
        if (*ptr == NULL)
         throw Exception(REC_OUT_OF_MEMORY, "Memory exhausted.");
        Logging::flog()->loggingOn(true);
        return;
      }
      catch (const Exception r)
       { Logging::flog()->loggingOn(true);
         if (r.errCode() == REC_OUT_OF_MEMORY)
          { if (!swapOut(loglevel)) throw;
          }
          else throw;
       }
    }
 }

#ifndef _MEM_CTRL_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Convert a memory block to float format.
    \param[in] idx        index of memory block
    \param[in] loglevel   level for logging

    Convert a memory block from char or short integer format to float format.
    If all memory is used up, some memory blocks are swapped out.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::convertToFloat(const unsigned short int idx,
                             const unsigned short int loglevel)
 { if (dataType(idx) == DT_FLT) return;               // already float format ?
   try
   { unsigned long int bsize, i;
                   // calculate amount of additional bytes needed for new block
     switch (dataType(idx))
      { case DT_UINT:
         bsize=mem[idx].size*(sizeof(float)-sizeof(uint32));
         break;
        case DT_SINT:
         bsize=mem[idx].size*(sizeof(float)-sizeof(signed short int));
         break;
        case DT_CHAR:
         bsize=mem[idx].size*(sizeof(float)-sizeof(char));
         break;
        default:
         bsize=0;
         break;
      }
                                          // swap-out other blocks if necessary
     while ((signed long int)(bsize/1024) > free_mem)
      if (!swapOut(loglevel)) break;
                                                          // create float block
     allocMemory(&mem[idx].fdata, mem[idx].size, loglevel);
     switch (dataType(idx))                                     // convert data
      { case DT_UINT:                             // unsigned long int to float
         for (i=0; i < mem[idx].size; i++)
          mem[idx].fdata[i]=(float)mem[idx].uidata[i];
         delete[] mem[idx].uidata;
         mem[idx].uidata=NULL;
         freeMem(mem[idx].size*(sizeof(uint32)-sizeof(float)));
         break;
        case DT_SINT:                              // signed short int to float
         for (i=0; i < mem[idx].size; i++)
          mem[idx].fdata[i]=(float)mem[idx].sidata[i];
         delete[] mem[idx].sidata;
         mem[idx].sidata=NULL;
         freeMem(mem[idx].size*(sizeof(signed short int)-sizeof(float)));
         break;
        case DT_CHAR:                                          // char to float
         for (i=0; i < mem[idx].size; i++)
          mem[idx].fdata[i]=(float)mem[idx].cdata[i];
         delete[] mem[idx].cdata;
         mem[idx].cdata=NULL;
         freeMem(mem[idx].size*(sizeof(char)-sizeof(float)));
         break;
        case DT_FLT:                                            // can't happen
         break;
      }
     mem[idx].dt=DT_FLT;
   }
   catch (...)
    { if (mem[idx].fdata != NULL) { delete[] mem[idx].fdata;
                                    mem[idx].fdata=NULL;
                                  }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a memory block in char format and get read/write access.
    \param[in]  size       number of data elements in block
    \param[out] idx        index of memory block
    \param[in]  name       first part of swap-filename
    \param[in]  loglevel   level for logging
    \return pointer to data
    \exception REC_MEMORY_BLOCK   no free memory block

    Create a memory block in char format and get read/write access.
    If all memory is used up, some memory blocks are swapped out.
 */
/*---------------------------------------------------------------------------*/
char *MemCtrl::createChar(const unsigned long int size,
                          unsigned short int * const idx,
                          const std::string name,
                          const unsigned short int loglevel)
 { try
   { *idx=0;
     lock->wait();                 // lock access to block list for this thread
     for (std::vector <tmem>::iterator it=mem.begin(); it != mem.end(); ++it,
          (*idx)++)
      if (!(*it).block_allocated)                        // search unused block
       {                                  // swap-out other blocks if necessary
         while ((signed long int)(size*sizeof(char)/1024) > free_mem)
          if (!swapOut(loglevel)) break;
                                                    // create new block of char
         try
         { (*it).dt=DT_CHAR;
           (*it).size=size;
           allocMemory(&(*it).cdata, (*it).size, loglevel);
           (*it).block_lock=new Semaphore(0);
           (*it).checked_out=1;
           (*it).last_access=time(NULL);
           (*it).block_allocated=true;
           (*it).stored_in_file=false;
           (*it).swap_filename=swappingPath()+name+"_"+
                               UniqueName::un()->getName();
         }
         catch (...)
          { if ((*it).block_lock != NULL) { delete (*it).block_lock;
                                            (*it).block_lock=NULL;
                                          }
            if ((*it).cdata != NULL) { delete[] (*it).cdata;
                                       (*it).cdata=NULL;
                                     }
            lock->signal();
            throw;
          }
         freeMem(-sizeof(char)*(*it).size);    // update counter of free memory
         updatePattern(*idx, loglevel);         // update memory access pattern
         lock->signal();
         return((*it).cdata);
       }
     throw Exception(REC_MEMORY_BLOCK, "No free memory block.");
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a memory block in float format and get read/write access.
    \param[in]  size       number of data elements in block
    \param[out] idx        index of memory block
    \param[in]  name       first part of swap-filename
    \param[in]  loglevel   level for logging
    \return pointer to data
    \exception REC_MEMORY_BLOCK   no free memory block

    Create a memory block in float format and get read/write access.
    If all memory is used up, some memory blocks are swapped out.
 */
/*---------------------------------------------------------------------------*/
float *MemCtrl::createFloat(const unsigned long int size,
                            unsigned short int * const idx,
                            const std::string name,
                            const unsigned short int loglevel)
 { try
   { *idx=0;
     lock->wait();                 // lock access to block list for this thread
     for (std::vector <tmem>::iterator it=mem.begin(); it != mem.end(); ++it,
          (*idx)++)
      if (!(*it).block_allocated)                        // search unused block
       {                                  // swap-out other blocks if necessary
         while ((signed long int)(size*sizeof(float)/1024) > free_mem)
          if (!swapOut(loglevel)) break;
                                                   // create new block of float
         try
         { (*it).dt=DT_FLT;
           (*it).size=size;
           allocMemory(&(*it).fdata, (*it).size, loglevel);
           (*it).block_lock=new Semaphore(0);
           (*it).checked_out=1;
           (*it).last_access=time(NULL);
           (*it).block_allocated=true;
           (*it).stored_in_file=false;
           (*it).swap_filename=swappingPath()+name+"_"+
                               UniqueName::un()->getName();
         }
         catch (...)
          { if ((*it).block_lock != NULL) { delete (*it).block_lock;
                                            (*it).block_lock=NULL;
                                          }
            if ((*it).fdata != NULL) { delete[] (*it).fdata;
                                       (*it).fdata=NULL;
                                     }
            lock->signal();
            throw;
          }
         freeMem(-sizeof(float)*(*it).size);   // update counter of free memory
         updatePattern(*idx, loglevel);         // update memory access pattern
         lock->signal();
         return((*it).fdata);
       }
     throw Exception(REC_MEMORY_BLOCK, "No free memory block.");
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a memory block in signed short int format and get read/write
           access.
    \param[in]  size       number of data elements in block
    \param[out] idx        index of block
    \param[in]  name       first part of swap-filename
    \param[in]  loglevel   level for logging
    \return pointer to data
    \exception REC_MEMORY_BLOCK   no free memory block

    Create a memory block in signed short int format and get read/write access.
    If all memory is used up, some memory blocks are swapped out.
 */
/*---------------------------------------------------------------------------*/
signed short int *MemCtrl::createSInt(const unsigned long int size,
                                      unsigned short int * const idx,
                                      const std::string name,
                                      const unsigned short int loglevel)
 { try
   { *idx=0;
     lock->wait();                 // lock access to block list for this thread
     for (std::vector <tmem>::iterator it=mem.begin(); it != mem.end(); ++it,
          (*idx)++)
      if (!(*it).block_allocated)                        // search unused block
       {                                  // swap-out other blocks if necessary
         while ((signed long int)
                (size*sizeof(signed short int)/1024) > free_mem)
          if (!swapOut(loglevel)) break;
                                        // create new block of signed short int
         try
         { (*it).dt=DT_SINT;
           (*it).size=size;
           allocMemory(&(*it).sidata, (*it).size, loglevel);
           (*it).block_lock=new Semaphore(0);
           (*it).checked_out=1;
           (*it).last_access=time(NULL);
           (*it).block_allocated=true;
           (*it).stored_in_file=false;
           (*it).swap_filename=swappingPath()+name+"_"+
                               UniqueName::un()->getName();
         }
         catch (...)
          { if ((*it).block_lock != NULL) { delete (*it).block_lock;
                                            (*it).block_lock=NULL;
                                          }
            if ((*it).sidata != NULL) { delete[] (*it).sidata;
                                        (*it).sidata=NULL;
                                      }
            lock->signal();
            throw;
          }
                                               // update counter of free memory
         freeMem(-sizeof(signed short int)*(*it).size);
         updatePattern(*idx, loglevel);         // update memory access pattern
         lock->signal();
         return((*it).sidata);
       }
     throw Exception(REC_MEMORY_BLOCK, "No free memory block.");
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a memory block in unsigned long int format and get read/write
           access.
    \param[in]  size       number of data elements in block
    \param[out] idx        index of block
    \param[in]  name       first part of swap-filename
    \param[in]  loglevel   level for logging
    \return pointer to data
    \exception REC_MEMORY_BLOCK   no free memory block

    Create a memory block in unsigned long int format and get read/write
    access. If all memory is used up, some memory blocks are swapped out.
 */
/*---------------------------------------------------------------------------*/
uint32 *MemCtrl::createUInt(const unsigned long int size,
                            unsigned short int * const idx,
                            const std::string name,
                            const unsigned short int loglevel)
 { try
   { *idx=0;
     lock->wait();                 // lock access to block list for this thread
     for (std::vector <tmem>::iterator it=mem.begin(); it != mem.end(); ++it,
          (*idx)++)
      if (!(*it).block_allocated)                        // search unused block
       {                                  // swap-out other blocks if necessary
         while ((signed long int)(size*sizeof(uint32)/1024) > free_mem)
          if (!swapOut(loglevel)) break;
                                        // create new block of signed short int
         try
         { (*it).dt=DT_UINT;
           (*it).size=size;
           allocMemory(&(*it).uidata, (*it).size, loglevel);
           (*it).block_lock=new Semaphore(0);
           (*it).checked_out=1;
           (*it).last_access=time(NULL);
           (*it).block_allocated=true;
           (*it).stored_in_file=false;
           (*it).swap_filename=swappingPath()+name+"_"+
                               UniqueName::un()->getName();
         }
         catch (...)
          { if ((*it).block_lock != NULL) { delete (*it).block_lock;
                                            (*it).block_lock=NULL;
                                          }
            if ((*it).uidata != NULL) { delete[] (*it).uidata;
                                        (*it).uidata=NULL;
                                      }
            lock->signal();
            throw;
          }
                                               // update counter of free memory
         freeMem(-sizeof(uint32)*(*it).size);
         updatePattern(*idx, loglevel);         // update memory access pattern
         lock->signal();
         return((*it).uidata);
       }
     throw Exception(REC_MEMORY_BLOCK, "No free memory block.");
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request data type of memory block.
    \param[in] idx   index of memory block
    \return data type
 */
/*---------------------------------------------------------------------------*/
MemCtrl::tdatatype MemCtrl::dataType(const unsigned short int idx) const
 { return(mem[idx].dt);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete memory block.
    \param[in,out] idx     index of memory block
    \param[in]     force   force deletion ?

    Delete a memory block and its swap file. If the \em force parameter is set,
    the block is deleted, even if the locking semaphore of the block is set.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::delete_block(unsigned short int * const idx, const bool force)
 { if (*idx >= MemCtrl::MAX_BLOCK) return;
   lock->wait();
   if (!mem[*idx].block_allocated) { lock->signal();
                                     return;
                                   }
   if (!force) waitBlock(*idx);
   switch (dataType(*idx))                                    // release memory
    { case DT_FLT:
       if (mem[*idx].fdata != NULL)
        { delete[] mem[*idx].fdata;
          mem[*idx].fdata=NULL;
          freeMem(sizeof(float)*mem[*idx].size);
        }
       break;
      case DT_UINT:
       if (mem[*idx].uidata != NULL)
        { delete[] mem[*idx].uidata;
          mem[*idx].uidata=NULL;
          freeMem(sizeof(uint32)*mem[*idx].size);
        }
       break;
      case DT_SINT:
       if (mem[*idx].sidata != NULL)
        { delete[] mem[*idx].sidata;
          mem[*idx].sidata=NULL;
          freeMem(sizeof(signed short int)*mem[*idx].size);
        }
       break;
      case DT_CHAR:
       if (mem[*idx].cdata != NULL)
        { delete[] mem[*idx].cdata;
          mem[*idx].cdata=NULL;
          freeMem(sizeof(char)*mem[*idx].size);
        }
       break;
    }
   mem[*idx].block_allocated=false;
   unlink(mem[*idx].swap_filename.c_str());                 // remove swap file
   if (!force) signalBlock(*idx);
   delete mem[*idx].block_lock;
   mem[*idx].block_lock=NULL;
   *idx=MemCtrl::MAX_BLOCK;
   lock->signal();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete memory block.
    \param[in,out] idx   index of memory block

    Delete a memory block and its swap file. If the block is still in use, this
    function blocks.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::deleteBlock(unsigned short int * const idx)
 { delete_block(idx, false);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete memory block.
    \param[in,out] idx   index of memory block

    Delete a memory block and its swap file. The block is deleted, even if it
    is still in use. This function should be used only when the program
    terminates anyway.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::deleteBlockForce(unsigned short int * const idx)
 { delete_block(idx, true);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete the only instance of the memory controller.

    Delete the only instance of the memory controller. The memory controller is
    a singleton object.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::deleteMemoryController()
 { if (instance != NULL) { delete instance;
                           instance=NULL;
                         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Modify counter of free memory.
    \param[in] amount   number of bytes to add

    The free_mem counter contains the amount of free memory in KBytes that is
    available to the memory controller. The function is used to
    increment/decrement this counter.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::freeMem(const signed long int amount)
 { free_mem+=amount/1024;
#if 0
   Logging::flog()->logMsg("free memory: #1 MByte", 0)->
    arg((float)free_mem/1024.0f);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read/write access to char memory block.
    \param[in] idx      index of memory block
    \param[in] loglevel level for logging
    \return pointer to data

    Request read/write access to a memory block in char format. If the memory
    block is swapped out, it will be swapped in. If all memory is used up,
    some other memory blocks are swapped out. The function blocks, until the
    memory block becomes available. Only one thread at a time can have
    read/write access to a memory block.
 */
/*---------------------------------------------------------------------------*/
char *MemCtrl::getChar(const unsigned short int idx,
                       const unsigned short int loglevel)
 { if (idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(idx) != DT_CHAR) return(NULL);
   try
   { lock->wait();
     waitBlock(idx);                                   // get read/write access
                                                         // is data in memory ?
     if ((mem[idx].fdata == NULL) && (mem[idx].sidata == NULL) &&
         (mem[idx].cdata == NULL) && (mem[idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)(mem[idx].size*sizeof(char)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
        swapIn(idx, "rw", loglevel);                           // swap-in block
        mem[idx].stored_in_file=false;             // swap file will be invalid
        unlink(mem[idx].swap_filename.c_str());             // delete swap file
      }
                     // increase number of times block is currently checked out
                     // (result is always 1)
     mem[idx].checked_out++;
     mem[idx].checked_out_rw=true;   // block is checked out in read/write mode
     updatePattern(idx, loglevel);              // update memory access pattern
     lock->signal();
     return(mem[idx].cdata);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read-only access to char memory block.
    \param[in] idx      index of memory block
    \param[in] loglevel level for logging
    \return pointer to data

    Request read-only access to a memory block in char format. If the memory
    block is swapped out, it will be swapped in. If all memory is used up,
    some other memory blocks are swapped out. The function blocks, until the
    memory block becomes available. Several threads can have read-only access
    to a memory block at the same time.
 */
/*---------------------------------------------------------------------------*/
char *MemCtrl::getCharRO(const unsigned short int idx,
                         const unsigned short int loglevel)
 { if (idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(idx) != DT_CHAR) return(NULL);
   try
   { lock->wait();
            // is block checked out in read-write mode or not checked out yet ?
     if (mem[idx].checked_out_rw || (mem[idx].checked_out == 0))
      waitBlock(idx);                                             // get access
                                                         // is data in memory ?
     if ((mem[idx].fdata == NULL) && (mem[idx].sidata == NULL) &&
         (mem[idx].cdata == NULL) && (mem[idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)(mem[idx].size*sizeof(char)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
                          // swap-in block and keep swap-file - its still valid
        swapIn(idx, "r", loglevel);
      }
                     // increase number of times block is currently checked out
     mem[idx].checked_out++;
     mem[idx].checked_out_rw=false;   // block is checked out in read-only mode
     updatePattern(idx, loglevel);              // update memory access pattern
     lock->signal();
     return(mem[idx].cdata);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read/write access to float memory block.
    \param[in] idx      index of memory block
    \param[in] loglevel level for logging
    \return pointer to data

    Request read/write access to a memory block in float format. If the memory
    block is swapped out, it will be swapped in. If all memory is used up,
    some other memory blocks are swapped out. The function blocks, until the
    memory block becomes available. Only one thread at a time can have
    read/write access to a memory block. If the memory block is not stored in
    float format, it will be converted.
 */
/*---------------------------------------------------------------------------*/
float *MemCtrl::getFloat(const unsigned short int idx,
                         const unsigned short int loglevel)
 { if (idx >= MemCtrl::MAX_BLOCK) return(NULL);
   try
   { lock->wait();
     waitBlock(idx);                                   // get read/write access
                                                         // is data in memory ?
     if ((mem[idx].fdata == NULL) && (mem[idx].sidata == NULL) &&
         (mem[idx].cdata == NULL) && (mem[idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)(mem[idx].size*sizeof(float)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
        swapIn(idx, "rw", loglevel);                           // swap-in block
        mem[idx].stored_in_file=false;             // swap file will be invalid
        unlink(mem[idx].swap_filename.c_str());             // delete swap file
      }
                     // increase number of times block is currently checked out
                     // (result is always 1)
     mem[idx].checked_out++;
     mem[idx].checked_out_rw=true;   // block is checked out in read/write mode
                                                      // convert data to float
     if (dataType(idx) != DT_FLT) convertToFloat(idx, loglevel);
     updatePattern(idx, loglevel);              // update memory access pattern
     lock->signal();
     return(mem[idx].fdata);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read-only access to float memory block.
    \param[in] idx      index of memory block
    \param[in] loglevel level for logging
    \return pointer to data

    Request read-only access to a memory block in float format. If the memory
    block is swapped out, it will be swapped in. If all memory is used up,
    some other memory blocks are swapped out. The function blocks, until the
    memory block becomes available. Several threads can have read-only access
    to a memory block at the same time. If the memory block is not stored in
    float format, it will be converted.
 */
/*---------------------------------------------------------------------------*/
float *MemCtrl::getFloatRO(const unsigned short int idx,
                           const unsigned short int loglevel)
 { if (idx >= MemCtrl::MAX_BLOCK) return(NULL);
   try
   { lock->wait();
            // is block checked out in read-write mode or not checked out yet ?
     if (mem[idx].checked_out_rw || (mem[idx].checked_out == 0))
      waitBlock(idx);                                             // get access
                                                         // is data in memory ?
     if ((mem[idx].fdata == NULL) && (mem[idx].sidata == NULL) &&
         (mem[idx].cdata == NULL) && (mem[idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)(mem[idx].size*sizeof(float)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
                          // swap-in block and keep swap-file - its still valid
        swapIn(idx, "r", loglevel);
      }
                     // increase number of times block is currently checked out
     mem[idx].checked_out++;
                                     // convert to float if not checked out yet
     if (dataType(idx) != DT_FLT)
      if (mem[idx].checked_out == 1) convertToFloat(idx, loglevel);
       else { mem[idx].checked_out--;
              lock->signal();
              return(NULL);
            }
     mem[idx].checked_out_rw=false;   // block is checked out in read-only mode
     updatePattern(idx, loglevel);              // update memory access pattern
     lock->signal();
     return(mem[idx].fdata);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read/write access to char memory block and remove block
           from memory controller.
    \param[in,out] idx      index of memory block
    \param[in]     loglevel level for logging
    \return pointer to data

    Request read/write access to a memory block in char format and remove the
    block from the memory controller. If the memory block is swapped out, it
    will be swapped in. If all memory is used up, some other memory blocks are
    swapped out. The function blocks, until the memory block becomes available.
    Only one thread at a time can have read/write access to a memory block.
    The caller is responsible for releasing the memory after use.
 */
/*---------------------------------------------------------------------------*/
char *MemCtrl::getRemoveChar(unsigned short int * const idx,
                             const unsigned short int loglevel)
 { if (*idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(*idx) != DT_CHAR) return(NULL);
   try
   { char *cp;

     lock->wait();
     waitBlock(*idx);                                  // get read/write access
                                                         // is data in memory ?
     if ((mem[*idx].fdata == NULL) && (mem[*idx].sidata == NULL) &&
         (mem[*idx].cdata == NULL) && (mem[*idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)(mem[*idx].size*sizeof(char)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
        swapIn(*idx, "rwd", loglevel);                         // swap-in block
      }
     mem[*idx].stored_in_file=false;
     unlink(mem[*idx].swap_filename.c_str());               // delete swap file
     cp=mem[*idx].cdata;
     mem[*idx].cdata=NULL;
     freeMem(sizeof(char)*mem[*idx].size);      // increase free memory counter
     signalBlock(*idx);
                                                                // delete block
     delete mem[*idx].block_lock;
     mem[*idx].block_lock=NULL;
     mem[*idx].block_allocated=false;
     updatePattern(*idx, loglevel);             // update memory access pattern
     lock->signal();
     *idx=MemCtrl::MAX_BLOCK;
     return(cp);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read/write access to float memory block and remove block
           from memory controller.
    \param[in,out] idx      index of memory block
    \param[in]     loglevel level for logging
    \return pointer to data

    Request read/write access to a memory block in float format and remove the
    block from the memory controller. If the memory block is swapped out, it
    will be swapped in. If all memory is used up, some other memory blocks are
    swapped out. The function blocks, until the memory block becomes available.
    Only one thread at a time can have read/write access to a memory block.
    If the memory block is not stored in float format, it will be converted.
    The caller is responsible for releasing the memory after use.
 */
/*---------------------------------------------------------------------------*/
float *MemCtrl::getRemoveFloat(unsigned short int * const idx,
                               const unsigned short int loglevel)
 { if (*idx >= MemCtrl::MAX_BLOCK) return(NULL);
   try
   { float *fp;

     lock->wait();
     waitBlock(*idx);                                  // get read/write access
                                                         // is data in memory ?
     if ((mem[*idx].fdata == NULL) && (mem[*idx].sidata == NULL) &&
         (mem[*idx].cdata == NULL) && (mem[*idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)(mem[*idx].size*sizeof(float)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
        swapIn(*idx, "rwd", loglevel);                         // swap-in block
      }
     mem[*idx].stored_in_file=false;
     unlink(mem[*idx].swap_filename.c_str());               // delete swap file
                                                            // convert to float
     if (dataType(*idx) != DT_FLT) convertToFloat(*idx, loglevel);
     fp=mem[*idx].fdata;
     mem[*idx].fdata=NULL;
     freeMem(sizeof(float)*mem[*idx].size);     // increase free memoru counter
     signalBlock(*idx);
                                                                // delete block
     delete mem[*idx].block_lock;
     mem[*idx].block_lock=NULL;
     mem[*idx].block_allocated=false;
     updatePattern(*idx, loglevel);             // update memory access pattern
     lock->signal();
     *idx=MemCtrl::MAX_BLOCK;
     return(fp);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read/write access to signed short int memory block and
           remove block from memory controller.
    \param[in,out] idx      index of memory block
    \param[in]     loglevel level for logging
    \return pointer to data

    Request read/write access to a memory block in signed short int format and
    remove the block from the memory controller. If the memory block is swapped
    out, it will be swapped in. If all memory is used up, some other memory
    blocks are swapped out. The function blocks, until the memory block becomes
    available. Only one thread at a time can have read/write access to a memory
    block. The caller is responsible for releasing the memory after use.
 */
/*---------------------------------------------------------------------------*/
signed short int *MemCtrl::getRemoveSInt(unsigned short int * const idx,
                                         const unsigned short int loglevel)
 { if (*idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(*idx) != DT_SINT) return(NULL);

   try
   { signed short int *sp;

     lock->wait();
     waitBlock(*idx);                                  // get read/write access
                                                         // is data in memory ?
     if ((mem[*idx].fdata == NULL) && (mem[*idx].sidata == NULL) &&
         (mem[*idx].cdata == NULL) && (mem[*idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)
               (mem[*idx].size*sizeof(signed short int)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
        swapIn(*idx, "rwd", loglevel);                         // swap-in block
      }
     mem[*idx].stored_in_file=false;
     unlink(mem[*idx].swap_filename.c_str());               // delete swap file
     sp=mem[*idx].sidata;
     mem[*idx].sidata=NULL;
                                                // increase free memory counter
     freeMem(sizeof(signed short int)*mem[*idx].size);
     signalBlock(*idx);
                                                                // delete block
     delete mem[*idx].block_lock;
     mem[*idx].block_lock=NULL;
     mem[*idx].block_allocated=false;
     updatePattern(*idx, loglevel);             // update memory access pattern
     lock->signal();
     *idx=MemCtrl::MAX_BLOCK;
     return(sp);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read/write access to unsigned long int memory block and
           remove block from memory controller.
    \param[in,out] idx      index of memory block
    \param[in]     loglevel level for logging
    \return pointer to data

    Request read/write access to a memory block in unsigned long int format and
    remove the block from the memory controller. If the memory block is swapped
    out, it will be swapped in. If all memory is used up, some other memory
    blocks are swapped out. The function blocks, until the memory block becomes
    available. Only one thread at a time can have read/write access to a memory
    block. The caller is responsible for releasing the memory after use.
 */
/*---------------------------------------------------------------------------*/
uint32 *MemCtrl::getRemoveUInt(unsigned short int * const idx,
                               const unsigned short int loglevel)
 { if (*idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(*idx) != DT_UINT) return(NULL);

   try
   { uint32 *sp;

     lock->wait();
     waitBlock(*idx);                                  // get read/write access
                                                         // is data in memory ?
     if ((mem[*idx].fdata == NULL) && (mem[*idx].sidata == NULL) &&
         (mem[*idx].cdata == NULL) && (mem[*idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)
               (mem[*idx].size*sizeof(uint32)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
        swapIn(*idx, "rwd", loglevel);                         // swap-in block
      }
     mem[*idx].stored_in_file=false;
     unlink(mem[*idx].swap_filename.c_str());               // delete swap file
     sp=mem[*idx].uidata;
     mem[*idx].uidata=NULL;
                                                // increase free memory counter
     freeMem(sizeof(uint32)*mem[*idx].size);
     signalBlock(*idx);
                                                                // delete block
     delete mem[*idx].block_lock;
     mem[*idx].block_lock=NULL;
     mem[*idx].block_allocated=false;
     updatePattern(*idx, loglevel);             // update memory access pattern
     lock->signal();
     *idx=MemCtrl::MAX_BLOCK;
     return(sp);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read/write access to signed short int memory block.
    \param[in] idx      index of memory block
    \param[in] loglevel level for logging
    \return pointer to data

    Request read/write access to a memory block in signed short int format. If
    the memory block is swapped out, it will be swapped in. If all memory is
    used up, some other memory blocks are swapped out. The function blocks,
    until the memory block becomes available. Only one thread at a time can
    have read/write access to a memory block.
 */
/*---------------------------------------------------------------------------*/
signed short int *MemCtrl::getSInt(const unsigned short int idx,
                                   const unsigned short int loglevel)
 { if (idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(idx) != DT_SINT) return(NULL);
   try
   { lock->wait();
     waitBlock(idx);                                   // get read/write access
                                                         // is data in memory ?
     if ((mem[idx].fdata == NULL) && (mem[idx].sidata == NULL) &&
         (mem[idx].cdata == NULL) && (mem[idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)
               (mem[idx].size*sizeof(signed short int)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
        swapIn(idx, "rw", loglevel);                           // swap-in block
        mem[idx].stored_in_file=false;             // swap file will be invalid
        unlink(mem[idx].swap_filename.c_str());             // delete swap file
      }
                     // increase number of times block is currently checked out
                     // (result is always 1)
     mem[idx].checked_out++;
     mem[idx].checked_out_rw=true;   // block is checked out in read/write mode
     updatePattern(idx, loglevel);              // update memory access pattern
     lock->signal();
     return(mem[idx].sidata);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read-only access to signed short int memory block.
    \param[in] idx      index of memory block
    \param[in] loglevel level for logging
    \return pointer to data

    Request read-only access to a memory block in signed short int format. If
    the memory block is swapped out, it will be swapped in. If all memory is
    used up, some other memory blocks are swapped out. The function blocks,
    until the memory block becomes available. Several threads can have
    read-only access to a memory block at the same time. If the memory block is
    not stored in float format, it will be converted.
 */
/*---------------------------------------------------------------------------*/
signed short int *MemCtrl::getSIntRO(const unsigned short int idx,
                                     const unsigned short int loglevel)
 { if (idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(idx) != DT_SINT) return(NULL);
   try
   { lock->wait();
            // is block checked out in read-write mode or not checked out yet ?
     if (mem[idx].checked_out_rw || (mem[idx].checked_out == 0))
      waitBlock(idx);                                             // get access
                                                         // is data in memory ?
     if ((mem[idx].fdata == NULL) && (mem[idx].sidata == NULL) &&
         (mem[idx].cdata == NULL) && (mem[idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)
               (mem[idx].size*sizeof(signed short int)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
                          // swap-in block and keep swap-file - its still valid
        swapIn(idx, "r", loglevel);
      }
                     // increase number of times block is currently checked out
     mem[idx].checked_out++;
     mem[idx].checked_out_rw=false;   // block is checked out in read-only mode
     updatePattern(idx, loglevel);              // update memory access pattern
     lock->signal();
     return(mem[idx].sidata);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read/write access to unsigned long int memory block.
    \param[in] idx      index of memory block
    \param[in] loglevel level for logging
    \return pointer to data

    Request read/write access to a memory block in unsigned long int format. If
    the memory block is swapped out, it will be swapped in. If all memory is
    used up, some other memory blocks are swapped out. The function blocks,
    until the memory block becomes available. Only one thread at a time can
    have read/write access to a memory block.
 */
/*---------------------------------------------------------------------------*/
uint32 *MemCtrl::getUInt(const unsigned short int idx,
                         const unsigned short int loglevel)
 { if (idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(idx) != DT_UINT) return(NULL);
   try
   { lock->wait();
     waitBlock(idx);                                   // get read/write access
                                                         // is data in memory ?
     if ((mem[idx].fdata == NULL) && (mem[idx].sidata == NULL) &&
         (mem[idx].cdata == NULL) && (mem[idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)(mem[idx].size*sizeof(uint32)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
        swapIn(idx, "rw", loglevel);                           // swap-in block
        mem[idx].stored_in_file=false;             // swap file will be invalid
        unlink(mem[idx].swap_filename.c_str());             // delete swap file
      }
                     // increase number of times block is currently checked out
                     // (result is always 1)
     mem[idx].checked_out++;
     mem[idx].checked_out_rw=true;   // block is checked out in read/write mode
     updatePattern(idx, loglevel);              // update memory access pattern
     lock->signal();
     return(mem[idx].uidata);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request read-only access to unsigned long int memory block.
    \param[in] idx      index of memory block
    \param[in] loglevel level for logging
    \return pointer to data

    Request read-only access to a memory block in unsigned long int format. If
    the memory block is swapped out, it will be swapped in. If all memory is
    used up, some other memory blocks are swapped out. The function blocks,
    until the memory block becomes available. Several threads can have
    read-only access to a memory block at the same time. If the memory block is
    not stored in float format, it will be converted.
 */
/*---------------------------------------------------------------------------*/
uint32 *MemCtrl::getUIntRO(const unsigned short int idx,
                           const unsigned short int loglevel)
 { if (idx >= MemCtrl::MAX_BLOCK) return(NULL);
   if (dataType(idx) != DT_UINT) return(NULL);
   try
   { lock->wait();
            // is block checked out in read-write mode or not checked out yet ?
     if (mem[idx].checked_out_rw || (mem[idx].checked_out == 0))
      waitBlock(idx);                                             // get access
                                                         // is data in memory ?
     if ((mem[idx].fdata == NULL) && (mem[idx].sidata == NULL) &&
         (mem[idx].cdata == NULL) && (mem[idx].uidata == NULL))
      {                                   // swap-out other blocks if necessary
        while ((signed long int)(mem[idx].size*sizeof(uint32)/1024) > free_mem)
         if (!swapOut(loglevel)) break;
                          // swap-in block and keep swap-file - its still valid
        swapIn(idx, "r", loglevel);
      }
                     // increase number of times block is currently checked out
     mem[idx].checked_out++;
     mem[idx].checked_out_rw=false;   // block is checked out in read-only mode
     updatePattern(idx, loglevel);              // update memory access pattern
     lock->signal();
     return(mem[idx].uidata);
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to instance of memory controller.
    \return pointer to instance of memory controller

    Request pointer to the only instance of the memory controller. The memory
    controller is a singleton object. If it is not initialized, the constructor
    will be called.
 */
/*---------------------------------------------------------------------------*/
MemCtrl *MemCtrl::mc()
 { if (instance == NULL) instance=new MemCtrl();
   return(instance);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write information about swapping activity to log.
    \param[in] loglevel   level for logging

    Write information about swapping activity to log and save the memory access
    pattern in the database file.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::printPattern(const unsigned short int loglevel) const
 { std::string str;

   Logging::flog()->logMsg("swapped out: #1 MBytes (#2)", loglevel)->
    arg((double)swapped_out/1048576.0)->arg(secStr(swap_out_time));
   Logging::flog()->logMsg("swapped in: #1 MBytes (#2)", loglevel)->
    arg((double)swapped_in/1048576.0)->arg(secStr(swap_in_time));
   Logging::flog()->logMsg("memory access pattern: #1 entries", loglevel)->
    arg(pattern.size());
   //str=std::string();
   //for (unsigned short int i=0; i < pattern.size(); i++)
   // str+=toString(pattern[i])+" ";
   //Logging::flog()->logMsg("pattern: #1", loglevel)->arg(str);
   savePattern(loglevel);
   Logging::flog()->logMsg(" ", loglevel);

 }

/*---------------------------------------------------------------------------*/
/*! \brief Return the access rights for memory block.
    \param[in] idx   index of memory block

    Return the access rights for a memory block.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::put(const unsigned short int idx)
 { if (idx >= MemCtrl::MAX_BLOCK) return;
   lock->wait();
   if (mem[idx].checked_out>0) mem[idx].checked_out--;
                                     // is block still checked out by others ?
   if (mem[idx].checked_out == 0) signalBlock(idx);
   mem[idx].checked_out_rw=false;
   mem[idx].last_access=time(NULL);               // store time of last access
   lock->signal();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Put float block into memory controller and give up access rights.
    \param[in,out] fdata      pointer to float block
    \param[in]     size       number of elements in float block
    \param[out]    idx        index of memory block
    \param[in]     name       first part of swap-filename
    \param[in]     loglevel   level for logging
    \exception REC_MEMORY_BLOCK   no free memory block

    Put float block into memory controller and give up access rights.
    If all memory is used up, some memory blocks are swapped out.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::putFloat(float **fdata, const unsigned long int size,
                       unsigned short int * const idx,
                       const std::string name,
                       const unsigned short int loglevel)
 { try
   { *idx=0;
     lock->wait();                 // lock access to block list for this thread
     for (std::vector <tmem>::iterator it=mem.begin(); it != mem.end(); ++it,
          (*idx)++)
      if (!(*it).block_allocated)                        // search unused block
       {                                  // swap-out other blocks if necessary
         while ((signed long int)(size*sizeof(float)/1024) > free_mem)
          if (!swapOut(loglevel)) break;
                                         // initialize data structure for block
         try
         { (*it).dt=DT_FLT;
           (*it).size=size;
           (*it).fdata=*fdata;
           (*it).block_lock=new Semaphore(1);
           (*it).checked_out=0;
           (*it).last_access=time(NULL);
           (*it).block_allocated=true;
           (*it).stored_in_file=false;
           (*it).swap_filename=swappingPath()+name+"_"+
                               UniqueName::un()->getName();
         }
         catch (...)
          { if ((*it).block_lock != NULL) { delete (*it).block_lock;
                                            (*it).block_lock=NULL;
                                          }
            if ((*it).fdata != NULL) { delete[] (*it).fdata;
                                       (*it).fdata=NULL;
                                     }
            delete[] *fdata;
            *fdata=NULL;
            lock->signal();
            throw;
          }
         freeMem(-sizeof(float)*(*it).size);   // update counter of free memory
         lock->signal();
         *fdata=NULL;
         return;
       }
     throw Exception(REC_MEMORY_BLOCK, "No free memory block.");
   }
   catch (...)
    { lock->signal();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save memory access pattern in file.
    \param[in] loglevel   level for logging
    \exception REC_FILE_CREATE   can't create the file

    Save the current memory access pattern together with its key in a file, if
    it doesn't already exist in this file.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::savePattern(const unsigned short int loglevel) const
 { if (pattern_key.empty()) return;
     // does lookup pattern exist (similar to current pattern) ? -> don't store
   if (lu_pattern.size() > 0) return;
   std::ofstream file;
   unsigned short int len;
   bool append;

#ifdef WIN32
                                                            // create directory
   if (!PathExist(pattern_path)) CreateDirectory(pattern_path.c_str(), NULL);
#endif
   append=FileExist(pattern_filename);           // create new file or append ?
   file.open(pattern_filename.c_str(),
             std::ios::out|std::ios::app|std::ios::binary);
   if (!file)
    throw Exception(REC_FILE_CREATE,
                    "Can't create the file '#1'.").arg(pattern_filename);
   if (!append)                        // write copyright information into file
    { char eos=0;
      std::string str="e7_tools (c) 2004 CPS Innovations ";

      file.write((char *)str.c_str(), str.length());
      file.write(&eos, sizeof(char));
      file.write((char *)&MAPFILE_VERSION, sizeof(unsigned short int));
    }
   Logging::flog()->logMsg("store new memory access pattern", loglevel);
                                          // store key of memory access pattern
   len=pattern_key.length();
   file.write((char *)&len, sizeof(unsigned short int));
   file.write((char *)pattern_key.c_str(), len);
                                                 // store memory access pattern
   len=pattern.size();
   file.write((char *)&len, sizeof(unsigned short int));
   for (unsigned short int i=0; i < len; i++)
    file.write((char *)&pattern[i], sizeof(unsigned short int));
   file.close();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Search for a memory access pattern with the given key.
    \param[in] _pattern_key   key of memory access pattern
    \param[in] loglevel       level for logging

    Search in the pattern file for a memory access pattern with the given key.
    If more than one pattern is stored under this key, the last one is used. If
    no matching pattern is found, a simple LIFO strategy will be used.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::searchForPattern(const std::string _pattern_key,
                               const unsigned short int loglevel)
 { if (_pattern_key.empty())
    { lu_pattern.clear();
      Logging::flog()->logMsg("no memory access pattern found -> use LIFO",
                              loglevel);
      return;
    }
   pattern_key=_pattern_key;
   std::ifstream file;

   std::cout << "mem_ctl.cpp:MemCtrl::searchForPattern(" << pattern_key << "): pattern_filename = '" << pattern_filename << "'" << std::endl;

   file.open(pattern_filename.c_str(), std::ios::in|std::ios::binary);
   if (!file)
    { lu_pattern.clear();
      Logging::flog()->logMsg("no memory access pattern found -> use LIFO",
                              loglevel);
      return;
    }
   char *buffer=NULL;

   try
   { unsigned short int version;

     lu_pattern.clear();
     for (;;)                            // read and skip copyright information
      { char eos;

        if (file.eof()) break;
        file.read(&eos, sizeof(char));
        if (eos == 0) break;
      }
     file.read((char *)&version, sizeof(unsigned short int));
     if (version != MAPFILE_VERSION)
      Logging::flog()->logMsg("memory access pattern outdated", loglevel);
      else for (;;)                     // search last matching pattern in file
            { unsigned short int len;
              std::string str;

              if (file.eof()) break;
                                                                    // read key
              file.read((char *)&len, sizeof(unsigned short int));
              buffer=new char[len+1];
              file.read(buffer, len);
              buffer[len]=0;
              str=std::string(buffer);
              delete[] buffer;
              buffer=NULL;
              file.read((char *)&len, sizeof(unsigned short int));
              if (str == pattern_key)                   // matching key found ?
               {                                  // read memory access pattern
                 lu_pattern.resize(len);
                 for (unsigned short int i=0; i < len; i++)
                 file.read((char *)&lu_pattern[i], sizeof(unsigned short int));
                 lu_ptr=0;
               }
               else file.seekg(len*sizeof(unsigned short int), std::ios::cur);
            }
     file.close();
     if (version != MAPFILE_VERSION) unlink(pattern_filename.c_str());
     if (lu_pattern.size() == 0)
      Logging::flog()->logMsg("no memory access pattern found -> use LIFO",
                              loglevel);
      else Logging::flog()->logMsg("memory access pattern found", loglevel);
   }
   catch (...)
    { file.close();
      if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set the memory limit for the swapping.
    \param[in] factor   factor of physical memory that can be used

    Set the memory limit for the swapping. The factor should be between 0 and
    1, but usually smaller than 1 since not all algorithms use the memory
    controller.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::setMemLimit(const float factor)
 { uint64 phys_mem;
                                               // get amount of physical memory
#ifdef __MACOSX__
   {
     mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
     host_basic_info hinfo;

     host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hinfo, &count);
     phys_mem = hinfo.memory_size / 1024;
   }
#endif
#ifdef __SOLARIS__
   phys_mem=1024*1024;
#endif
#ifdef __linux__
   {
     /* ahc */
     /*
     struct sysinfo si;

     sysinfo(&si);
     if (si.totalram > 1024)
       phys_mem = si.totalram / 1024 * si.mem_unit;
     else
       phys_mem = si.mem_unit / 1024 * si.totalram;
     */
   }
#endif
#ifdef WIN32
   { MEMORYSTATUS lp;

     GlobalMemoryStatus(&lp);
     phys_mem=lp.dwTotalPhys/1024;
   }
#endif
                                         // set memory threshold for swapping
   free_mem=(signed long int)(phys_mem*factor);
   Logging::flog()->logMsg("free memory in memory controller: #1 MByte", 0)->
    arg((float)free_mem/1024.0f);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set the path for swap files.
    \param[in] _path   path for swapping
    \exception REC_SWAP_PATH_INVALID   the directory doesn't exist

    Set the path for swap files.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::setSwappingPath(const std::string _path)
 { if (!_path.empty())
    { swap_path=_path+"/";
      if (!PathExist(swap_path))
       throw Exception(REC_SWAP_PATH_INVALID,
                       "The directory '#1' doesn't exist.").arg(swap_path);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Unlock memory block.
    \param[in] idx   index of memory block

    Unlock a memory block.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::signalBlock(const unsigned short int idx) const
 { mem[idx].block_lock->signal();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Swap block into memory.
    \param[in] idx    index of memory block
    \param[in] mode   access mode
    \param[in] loglevel   level for logging

    Swap block into memory. The swap file is not deleted.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::swapIn(const unsigned short int idx, const std::string mode,
                     const unsigned short int loglevel)
 { RawIO <float> *rio_f=NULL;
   RawIO <signed short int> *rio_si=NULL;
   RawIO <char> *rio_c=NULL;
   RawIO <uint32> *rio_ui=NULL;

   try
   { StopWatch sw;
   
     switch (dataType(idx))
      { case DT_FLT:
         Logging::flog()->logMsg("swap-in (#1): #2 (#3 MByte)", loglevel+1)->
          arg(mode)->arg(mem[idx].swap_filename)->
          arg((double)(mem[idx].size*sizeof(float))/1048576.0);
                                                                   // load data
         allocMemory(&mem[idx].fdata, mem[idx].size, loglevel);
         sw.start();
         rio_f=new RawIO <float>(mem[idx].swap_filename, false, false);
         rio_f->read(mem[idx].fdata, mem[idx].size);
         delete rio_f;
         rio_f=NULL;
         swap_in_time+=sw.stop();
                                                // decrease free memory counter
         freeMem(-mem[idx].size*sizeof(float));
         swapped_in+=mem[idx].size*sizeof(float);
         break;
        case DT_UINT:
         Logging::flog()->logMsg("swap-in (#1): #2 (#3 MByte)", loglevel+1)->
          arg(mode)->arg(mem[idx].swap_filename)->
          arg((double)(mem[idx].size*sizeof(unsigned long int))/1048576.0);
                                                                   // load data
         allocMemory(&mem[idx].uidata, mem[idx].size, loglevel);
         sw.start();
         rio_ui=new RawIO <uint32>(mem[idx].swap_filename, false, false);
         rio_ui->read(mem[idx].uidata, mem[idx].size);
         delete rio_ui;
         rio_ui=NULL;
         swap_in_time+=sw.stop();
                                                // decrease free memory counter
         freeMem(-mem[idx].size*sizeof(uint32));
         swapped_in+=mem[idx].size*sizeof(uint32);
         break;
        case DT_SINT:
         Logging::flog()->logMsg("swap-in (#1): #2 (#3 MByte)", loglevel+1)->
          arg(mode)->arg(mem[idx].swap_filename)->
          arg((double)(mem[idx].size*sizeof(signed short int))/1048576.0);
                                                                   // load data
         allocMemory(&mem[idx].sidata, mem[idx].size, loglevel);
         sw.start();
         rio_si=new RawIO <signed short int>(mem[idx].swap_filename, false,
                                             false);
         rio_si->read(mem[idx].sidata, mem[idx].size);
         delete rio_si;
         rio_si=NULL;
         swap_in_time+=sw.stop();
                                                // decrease free memory counter
         freeMem(-mem[idx].size*sizeof(signed short int));
         swapped_in+=mem[idx].size*sizeof(signed short int);
         break;
        case DT_CHAR:
         Logging::flog()->logMsg("swap-in (#1): #2 (#3 MByte)", loglevel+1)->
          arg(mode)->arg(mem[idx].swap_filename)->
          arg((double)(mem[idx].size*sizeof(char))/1048576.0);
                                                                   // load data
         allocMemory(&mem[idx].cdata, mem[idx].size, loglevel);
         sw.start();
         rio_c=new RawIO <char>(mem[idx].swap_filename, false, false);
         rio_c->read(mem[idx].cdata, mem[idx].size);
         delete rio_c;
         rio_c=NULL;
         swap_in_time+=sw.stop();
                                                // decrease free memory counter
         freeMem(-mem[idx].size*sizeof(char));
         swapped_in+=mem[idx].size*sizeof(char);
         break;
      }
   }
   catch (...)
    { if (rio_f != NULL) delete rio_f;
      if (rio_si != NULL) delete rio_si;
      if (rio_ui != NULL) delete rio_ui;
      if (rio_c != NULL) delete rio_c;
      if (mem[idx].fdata != NULL) { delete[] mem[idx].fdata;
                                    mem[idx].fdata=NULL;
                                  }
      if (mem[idx].uidata != NULL) { delete[] mem[idx].uidata;
                                     mem[idx].uidata=NULL;
                                   }
      if (mem[idx].sidata != NULL) { delete[] mem[idx].sidata;
                                     mem[idx].sidata=NULL;
                                   }
      if (mem[idx].cdata != NULL) { delete[] mem[idx].cdata;
                                    mem[idx].cdata=NULL;
                                  }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Swap block out of memory.
    \param[in] loglevel   level for logging
    \return found block to swap-out ?

    Search a block that can be swapped out and do it. The algorithm first
    searches for a read-only block that is in memory but not in use. This swap
    file for this block still exists and the block can be released without
    disk access.

    If no block is found, the memory access pattern is used to decide which
    block is in memory, not in use and will be needed last from now.

    If no block is found or no memory access pattern available, a LIFO strategy
    is used. The block that was used last, is still in memory and not in use
    right now will be used.

    If still no block is found, the function will return without swapping.
 */
/*---------------------------------------------------------------------------*/
bool MemCtrl::swapOut(const unsigned short int loglevel)
 { RawIO <float> *rio_f=NULL;
   RawIO <signed short int> *rio_si=NULL;
   RawIO <char> *rio_c=NULL;
   RawIO <uint32> *rio_ui=NULL;

   try
   { unsigned short int idx=std::numeric_limits <unsigned short int>::max(),
                        i=0;
     StopWatch sw;
                // is a read-only block in memory which is not needed anymore ?
     for (std::vector <tmem>::iterator it=mem.begin();
          it != mem.end();
          ++it,
          i++)
      if (((*it).block_allocated) &&
          (((*it).fdata != NULL) || ((*it).uidata != NULL) ||
           ((*it).sidata != NULL) || ((*it).cdata != NULL)) &&
          ((*it).checked_out == 0))
       if ((*it).stored_in_file) idx=i;
                                                            // no block found ?
     if (idx == std::numeric_limits <unsigned short int>::max())
      { if (lu_pattern.size() > 0)              // lookup memory access pattern
         { unsigned short int distance[MemCtrl::MAX_BLOCK];
           signed short int max_dist=-1;
                             // calculate which block is used how long from now
           for (i=0; i < MemCtrl::MAX_BLOCK; i++)
            distance[i]=std::numeric_limits <unsigned short int>::max();
           for (i=lu_ptr+1; i < lu_pattern.size(); i++)
            if (distance[lu_pattern[i]] ==
                std::numeric_limits <unsigned short int>::max())
             distance[lu_pattern[i]]=i-lu_ptr;
                                       // find block that will be used at last,
                                       // is in memory and not needed anymore
           for (i=0; i < MemCtrl::MAX_BLOCK; i++)
            if (mem[i].block_allocated &&
                ((mem[i].fdata != NULL) || (mem[i].uidata != NULL) ||
                 (mem[i].sidata != NULL) || (mem[i].cdata != NULL)) &&
                (mem[i].checked_out == 0))
             if (distance[i] > max_dist)
              { idx=i;
                max_dist=(signed short int)distance[i];
              }
                                                            // no block found ?
           if (idx == std::numeric_limits <unsigned short int>::max())
            { lu_pattern.clear();
              Logging::flog()->logMsg("memory access pattern unknown -> use "
                                      "LIFO", loglevel);
            }
         }
        if (lu_pattern.size() == 0)                        // use LIFO strategy
         { time_t latest=0;
                                         // find block that was accessed last,
                                         // is in memory and not needed anymore
           i=0;
           for (std::vector <tmem>::iterator it=mem.begin();
                it != mem.end();
                ++it,
                i++)
            if (((*it).block_allocated) &&
                (((*it).fdata != NULL) || ((*it).uidata != NULL) ||
                 ((*it).sidata != NULL) || ((*it).cdata != NULL)) &&
                ((*it).checked_out == 0))
             if ((*it).last_access > latest) { idx=i;
                                               latest=(*it).last_access;
                                             }
           if (idx == std::numeric_limits <unsigned short int>::max())
            return(false);
         }
      }
     waitBlock(idx);                         // get read/write access for block
     switch (dataType(idx))
      { case DT_FLT:
         if (!mem[idx].stored_in_file)                    // write data to file
          { Logging::flog()->logMsg("swap-out: #1 (#2 MByte)", loglevel+1)->
             arg(mem[idx].swap_filename)->
             arg((double)(mem[idx].size*sizeof(float))/1048576.0);
            sw.start();
            rio_f=new RawIO <float>(mem[idx].swap_filename, true, false);
            rio_f->write(mem[idx].fdata, mem[idx].size);
            delete rio_f;
            rio_f=NULL;
            swap_out_time+=sw.stop();
            swapped_out+=mem[idx].size*sizeof(float);
          }
                                                     // delete data from memory
         Logging::flog()->logMsg("delete memory block #1 #2", loglevel+1)->arg(idx)->arg(mem[idx].swap_filename);
         delete[] mem[idx].fdata;
         mem[idx].fdata=NULL;
         freeMem(mem[idx].size*sizeof(float));  // increase free memory counter
         break;
        case DT_UINT:
         if (!mem[idx].stored_in_file)                    // write data to file
          { Logging::flog()->logMsg("swap-out: #1 (#2 MByte)", loglevel+1)->
             arg(mem[idx].swap_filename)->
             arg((double)(mem[idx].size*sizeof(unsigned long int))/1048576.0);
            sw.start();
            rio_ui=new RawIO <uint32>(mem[idx].swap_filename, true, false);
            rio_ui->write(mem[idx].uidata, mem[idx].size);
            delete rio_ui;
            rio_ui=NULL;
            swap_out_time+=sw.stop();
            swapped_out+=mem[idx].size*sizeof(uint32);
          }
                                                     // delete data from memory
         delete[] mem[idx].uidata;
         mem[idx].uidata=NULL;
                                                // increase free memory counter
         freeMem(mem[idx].size*sizeof(uint32));
         break;
        case DT_SINT:
         if (!mem[idx].stored_in_file)                    // write data to file
          { Logging::flog()->logMsg("swap-out: #1 (#2 MByte)", loglevel+1)->
             arg(mem[idx].swap_filename)->
             arg((double)(mem[idx].size*sizeof(signed short int))/1048576.0);
            sw.start();
            rio_si=new RawIO <signed short int>(mem[idx].swap_filename, true,
                                                false);
            rio_si->write(mem[idx].sidata, mem[idx].size);
            delete rio_si;
            rio_si=NULL;
            swap_out_time+=sw.stop();
            swapped_out+=mem[idx].size*sizeof(signed short int);
          }
                                                     // delete data from memory
         delete[] mem[idx].sidata;
         mem[idx].sidata=NULL;
                                                // increase free memory counter
         freeMem(mem[idx].size*sizeof(signed short int));
         break;
        case DT_CHAR:
         if (!mem[idx].stored_in_file)                    // write data to file
          { Logging::flog()->logMsg("swap-out: #1 (#2 MByte)", loglevel+1)->
             arg(mem[idx].swap_filename)->
             arg((double)(mem[idx].size*sizeof(char))/1048576.0);
            sw.start();
            rio_c=new RawIO <char>(mem[idx].swap_filename, true, false);
            rio_c->write(mem[idx].cdata, mem[idx].size);
            delete rio_c;
            rio_c=NULL;
            swap_out_time+=sw.stop();
            swapped_out+=mem[idx].size*sizeof(char);
          }
                                                     // delete data from memory
         delete[] mem[idx].cdata;
         mem[idx].cdata=NULL;
                                                // increase free memory counter
         freeMem(mem[idx].size*sizeof(char));
         break;
      }
     mem[idx].stored_in_file=true;
     signalBlock(idx);
     return(true);
   }
   catch (...)
    { if (rio_f != NULL) delete rio_f;
      if (rio_ui != NULL) delete rio_ui;
      if (rio_si != NULL) delete rio_si;
      if (rio_c != NULL) delete rio_c;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request path for swapping.
    \return path for swapping

    Request path for swapping.
 */
/*---------------------------------------------------------------------------*/
std::string MemCtrl::swappingPath() const
 { return(swap_path);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Update memory access pattern.
    \param[in] idx        index of memory block
    \param[in] loglevel   level for logging

    Update memory access pattern. If the current pattern doesn't match the
    lookup-pattern anymore, the next blocks in the lookup-pattern are skipped
    until the patterns are in-sync again. If the end of the lookup-pattern is
    reached, lookup starts from the beginning. If the resynchronization is not
    successful or is done too many times, the algorithm switches back to a
    LIFO strategy.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::updatePattern(const unsigned short int idx,
                            const unsigned short int loglevel)
 { pattern.push_back(idx);                              // add block to pattern
   if (lu_pattern.size() == 0) return;               // adjust lookup pattern ?
                                  // is pattern "in-sync" with lookup pattern ?
   if (lu_pattern[lu_ptr] == idx)
    { lu_ptr++;
                                        // wrap around at end of lookup pattern
      if (lu_ptr == lu_pattern.size()) lu_ptr=0;
      return;
    }
                                    // pattern "out of sync" -> resynchronize ?
   if (resync_pattern_attempt == MemCtrl::MAX_PATTERN_RESYNC)
    { lu_pattern.clear();
      Logging::flog()->logMsg("memory access pattern unknown -> use LIFO",
                              loglevel);
      return;
    }
   bool restarted=false;

   resync_pattern_attempt++;
              // move pointer into lookup pattern forward until "in-sync" again
   while (lu_pattern[lu_ptr] != idx)
    { lu_ptr++;
      if (lu_ptr == lu_pattern.size())
       if (!restarted) { lu_ptr=0;       // wrap aound at end of lookup pattern
                         restarted=true;
                       }
        else {            // wrapped around already once -> can't resynchronize
               lu_pattern.clear();
               Logging::flog()->logMsg("memory access pattern unknown -> use "
                                       "LIFO", loglevel);
               return;
             }
    }
   Logging::flog()->logMsg("resynchronize memory access pattern", loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Lock memory block.
    \param[in] idx   index of memory block

    Lock a memory block.
 */
/*---------------------------------------------------------------------------*/
void MemCtrl::waitBlock(const unsigned short int idx) const
 { mem[idx].block_lock->wait();
 }
#endif
