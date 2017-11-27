/*! \class RawIO raw_io.h "raw_io.h"
    \brief This template implements an object used for parallel file-I/O.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/01/26 added function to determine the size of a file
    \date 2004/05/28 added Doxygen style comments
    \date 2004/10/15 added uint32 datatype
    \date 2005/01/15 simplified multi-threading of template
    \date 2005/01/15 added double datatype

    The class can be used to read/write files blocking or non-blocking
    (parallel to the caller process). Only the formats double, float,
    signed short int, uint64, uint32 and char are supported for asynchronous
    operation.

    The parallel reading and writing of files is implemented in the methods
    readPar() and writePar(). They create one thread that reads/writes the data
    and return immediately to the caller - therefore this methods are
    asynchronous. The read/write operation in the thread is split into blocks.
    After each block of data a semaphore signals. The user can use the wait()
    method (synchronous) or tryWait() (asynchronous) to check for these
    signals. The destructor of the object is synchronous, i.e. waits until the
    read/write operation is finished. The object can be used to read xor write
    files - mixing read and write operations is not supported.

    The class takes care of the conversion between big- and little-endian.
 */

/*! \example example_rawio.cpp
    Here are some examples of how to use the RawIO template.
 */

#include <iostream>
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <unistd.h>
#endif
#include <typeinfo>
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <alloca.h>
#endif
#ifdef WIN32
#include <malloc.h>
#endif
#endif
#ifndef _RAW_IO_CPP
#define _RAW_IO_CPP
#include "raw_io.h"
#endif
#include "exception.h"
#ifdef WIN32
#include "global_tmpl.h"
#endif
#include "swap_tmpl.h"

/*- constants ---------------------------------------------------------------*/

const uint64 MBYTE=1024*512;         // write data in chuncks of upto 512 KByte

/*- local functions ---------------------------------------------------------*/

 // The thread API is a C-API so C linkage and calling conventions have to be
 // used, when creating a thread. To use a method as thread, a wrapper function
 // in C is needed, that calls the method.

 // The object is implemented as a template to be able to use it for different
 // data types. The C-API of thread doesn't allow templates, therefore the
 // type of the template is also passed into the wrapper.

#ifndef _RAW_IO_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Start thread to read data from file starting at offset.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_PAR_IO_DATATYPE file-I/O template doesn't support this data
                                   type
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to read data from file starting at offset.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_ReadFileOffs(void *param)
 { try
   { std::string type_name;
     { RawIO <float>::tthread_params *tp;

       tp=(RawIO <float>::tthread_params *)param;
       type_name=tp->type_name;
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
       alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
       _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     }
     if (type_name == typeid(double).name())
      { RawIO <double>::tthread_params *tp;

        tp=(RawIO <double>::tthread_params *)param;
        tp->object->readFile(tp->offset, tp->use_offset, tp->data, tp->size,
                             tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(float).name())
      { RawIO <float>::tthread_params *tp;

        tp=(RawIO <float>::tthread_params *)param;
        tp->object->readFile(tp->offset, tp->use_offset, tp->data, tp->size,
                             tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(signed short int).name())
      { RawIO <signed short int>::tthread_params *tp;
 
        tp=(RawIO <signed short int>::tthread_params *)param;
        tp->object->readFile(tp->offset, tp->use_offset, tp->data, tp->size,
                             tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(uint64).name())
      { RawIO <uint64>::tthread_params *tp;
 
        tp=(RawIO <uint64>::tthread_params *)param;
        tp->object->readFile(tp->offset, tp->use_offset, tp->data, tp->size,
                             tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(uint32).name())
      { RawIO <uint32>::tthread_params *tp;
 
        tp=(RawIO <uint32>::tthread_params *)param;
        tp->object->readFile(tp->offset, tp->use_offset, tp->data, tp->size,
                             tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(char).name())
      { RawIO <char>::tthread_params *tp;
 
        tp=(RawIO <char>::tthread_params *)param;
        tp->object->readFile(tp->offset, tp->use_offset, tp->data, tp->size,
                             tp->blocks);
        return(NULL);
      }
     throw Exception(REC_PAR_IO_DATATYPE,
                     "File-I/O template doesn't support this data type.");
   }
   catch (const Exception r)
    { return(new Exception(r.errCode(), r.errStr()));
    }
   catch (...)
    { return(new Exception(REC_UNKNOWN_EXCEPTION,
                           "Unknown exception generated."));
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to write data into file.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_PAR_IO_DATATYPE file-I/O template doesn't support this data
                                   type
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to write data into file.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_WriteFile(void *param)
 { try
   { std::string type_name;
     { RawIO <float>::tthread_params *tp;

       tp=(RawIO <float>::tthread_params *)param;
       type_name=tp->type_name;
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
       alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
       _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     }
     if (type_name == typeid(double).name())
      { RawIO <double>::tthread_params *tp;

        tp=(RawIO <double>::tthread_params *)param;
        tp->object->writeFile(tp->data, tp->size, tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(float).name())
      { RawIO <float>::tthread_params *tp;

        tp=(RawIO <float>::tthread_params *)param;
        tp->object->writeFile(tp->data, tp->size, tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(signed short int).name())
      { RawIO <signed short int>::tthread_params *tp;
 
        tp=(RawIO <signed short int>::tthread_params *)param;
        tp->object->writeFile(tp->data, tp->size, tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(uint64).name())
      { RawIO <uint64>::tthread_params *tp;
 
        tp=(RawIO <uint64>::tthread_params *)param;
        tp->object->writeFile(tp->data, tp->size, tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(uint32).name())
      { RawIO <uint32>::tthread_params *tp;
 
        tp=(RawIO <uint32>::tthread_params *)param;
        tp->object->writeFile(tp->data, tp->size, tp->blocks);
        return(NULL);
      }
     if (type_name == typeid(char).name())
      { RawIO <char>::tthread_params *tp;
 
        tp=(RawIO <char>::tthread_params *)param;
        tp->object->writeFile(tp->data, tp->size, tp->blocks);
        return(NULL);
      }
     throw Exception(REC_PAR_IO_DATATYPE,
                     "File-I/O template doesn't support this data type.");
   }
   catch (const Exception r)
    { return(new Exception(r.errCode(), r.errStr()));
    }
   catch (...)
    { return(new Exception(REC_UNKNOWN_EXCEPTION,
                           "Unknown exception generated."));
    }
 }
#endif

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object and open file.
    \param[in] _filename    name of file
    \param[in] write        write file ? (otherwise read file)
    \param[in] big_endian   data in file in big-endian format ?
    \exception REC_PAR_IO_DATATYPE     file-I/O template doesn't support this
                                       data type
    \exception REC_FILE_CREATE         can't create the file
    \exception REC_FILE_DOESNT_EXIST   the file doesn't exist

    Initialize object and open file.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
RawIO <T>::RawIO(const std::string _filename, const bool _write,
                 const bool big_endian)
 {     // The reading and writing of larger files can be done in blocks.
       // This semaphore is used to signal the caller that a block is finished.
   filename=_filename;
   block_finished_sem=NULL;
   thread_running=false;                          // there is no thread running
   swap_data=(big_endian ^ BigEndianMachine());
   write_object=_write;
                                                                   // open file
   if (_write) {
     file=fopen(filename.c_str(), "wb");
   } else {
      file=fopen(filename.c_str(), "rb");
   }
   if (file == NULL) {
     if (_write) {
       throw Exception(REC_FILE_CREATE, "Can't create the file '#1'.").arg(filename);
     } else {
       throw Exception(REC_FILE_DOESNT_EXIST, "The file '#1' doesn't exist.").arg(filename);
     }
   }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Clean up and destroy object.

    Clean up and destroy object.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
RawIO <T>::~RawIO()
 { void *thread_result=NULL;
                                              // wait until I/O-thread finishes
   if (thread_running) ThreadJoin(tid, &thread_result);
                                                            // delete semaphore
   if (block_finished_sem != NULL) delete block_finished_sem;
   if (file != NULL) fclose(file);                                // close file
   // ahc 11/27/17 C++11 warns about throw in destructors
   // if (thread_result != NULL) throw (Exception *)thread_result;
   if (thread_result != NULL) 
    std::cerr << "ERROR: thread_result not NULL!" << std::endl;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Read data from file.
    \param[in,out] data    pointer to buffer for data
    \param[in]     _size   number of data elements to read
    \exception REC_FILE_READ   the file doesn't contain enough data

    Read data from file into a buffer. If a thread is running, the method
    blocks until the thread is finished reading. The data is converted between
    little- and big-endian as necessary.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::read(T * const data, const uint64 _size)
 { try
   { void *thread_result;

     try
     {                            // wait until last reading-thread is finished
       if (thread_running)
        { ThreadJoin(tid, &thread_result);
          thread_running=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
                                                            // delete semaphore
       if (block_finished_sem != NULL) { delete block_finished_sem;
                                         block_finished_sem=NULL;
                                       }
       if ((file != NULL) && !write_object)
        { uint64 bytes;
          char *dp;

          bytes=_size*sizeof(T);
          dp=(char *)data;
          do { uint64 read_bytes, s;
 
               if (bytes < MBYTE) s=bytes;
                else s=MBYTE;
               read_bytes=fread(dp, 1, (size_t)s, file);
               if (read_bytes > 0) { bytes-=read_bytes;
                                     dp+=read_bytes;
                                   }

                else throw Exception(REC_FILE_READ,
                                     "The file '#1' doesn't contain enough "
                                     "data.").arg(filename);
             } while (bytes > 0);
        }
       if (swap_data)
        for (uint64 i=0; i < _size; i++)
         Swap(&data[i]);
     }
     catch (...)
      { thread_result=NULL;
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
        if (thread_running)
         { ThreadJoin(tid, &thread_result);
           thread_running=false;
         }
        if (block_finished_sem != NULL) { delete block_finished_sem;
                                          block_finished_sem=NULL;
                                        }
                    // We might catch an exception of the terminating thread.
                    // In this case we throw the new exception from the thread,
                    // not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Read data from file starting at offset.
    \param[in]     offset   offset into file
    \param[in,out] data     pointer to buffer for data
    \param[in]     _size    number of data elements to read
    \exception REC_FILE_READ   the file doesn't contain enough data

    Read data from file starting at an offset into a buffer. If a thread is
    running, the method blocks until the thread is finished reading. The data
    is converted between little- and big-endian as necessary.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::read(const uint64 offset, T * const data, const uint64 _size)
 { try
   { void *thread_result;

     try
     {                            // wait until last reading-thread is finished
       if (thread_running)
        { ThreadJoin(tid, &thread_result);
          thread_running=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
                                                            // delete semaphore
       if (block_finished_sem != NULL) { delete block_finished_sem;
                                         block_finished_sem=NULL;
                                       }
       if ((file != NULL) && !write_object)
        { uint64 bytes;
          char *dp;

          fseek(file, offset*sizeof(T), SEEK_SET);

          bytes=_size*sizeof(T);
          dp=(char *)data;
          do { uint64 read_bytes;

               read_bytes=fread(dp, 1, std::min(bytes, MBYTE), file);
               if (read_bytes > 0) { bytes-=read_bytes;
                                     dp+=read_bytes;
                                   }

                else throw Exception(REC_FILE_READ,
                                     "The file '#1' doesn't contain enough "
                                     "data.").arg(filename);
             } while (bytes > 0);
        }
       if (swap_data)
        for (uint64 i=0; i < _size; i++)
         Swap(&data[i]);
     }
     catch (...)
      { thread_result=NULL;
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
        if (thread_running)
         { ThreadJoin(tid, &thread_result);
           thread_running=false;
         }
        if (block_finished_sem != NULL) { delete block_finished_sem;
                                          block_finished_sem=NULL;
                                        }
                    // We might catch an exception of the terminating thread.
                    // In this case we throw the new exception from the thread,
                    // not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Read data from file starting at offset in blocks.
    \param[in]     offset       offset into file
    \param[in]     use_offset   use the offset ? (otherwise read from current
                                                  position)
    \param[in,out] data         pointer to buffer for data
    \param[in]     _size        number of data elements to read
    \param[in]     blocks       split reading in this number of blocks
    \exception REC_FILE_READ   the file doesn't contain enough data

    Read data from file starting at an offset into a buffer. The amount of
    data to be read is split into a given number of blocks. After a block is
    read, a signal is sent. The data is converted between little- and
    big-endian as necessary.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::readFile(const uint64 offset, const bool use_offset, T *data,
                         const uint64 _size, const unsigned short int blocks)
 { if (use_offset)
    if (file != NULL) fseek(file, offset*sizeof(T), SEEK_SET);
   uint64 blocksize;
                                             // number of elements in one block
   blocksize=_size/(uint64)blocks;
   for (unsigned short int i=0; i < blocks; i++,
        data+=blocksize)
    {                                                             // read block
      if ((file != NULL) && !write_object)
       { uint64 bytes;
          char *dp;

          bytes=blocksize*sizeof(T);
          dp=(char *)data;
          do { uint64 read_bytes;

               read_bytes=fread(dp, 1, std::min(bytes, MBYTE), file);
               if (read_bytes > 0) { bytes-=read_bytes;
                                     dp+=read_bytes;
                                   }

                else throw Exception(REC_FILE_READ,
                                     "The file '#1' doesn't contain enough "
                                     "data.").arg(filename);
             } while (bytes > 0);
       }
      if (swap_data)
       for (uint64 j=0; j < blocksize; j++)
        Swap(&data[j]);
      block_finished_sem->signal();           // signal that block is in memory
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a thread to read data from a file.
    \param[in,out] data    pointer to buffer for data
    \param[in]     _size   number of data elements to read

    Create a thread to read data from a file. If a thread is already running,
    the method blocks until the thread is finished reading.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::readPar(T * const data, const uint64 _size)
 { readPar(0, false, data, _size, 1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a thread to read data from a file in blocks.
    \param[in,out] data     pointer to buffer for data
    \param[in]     _size    number of data elements to read
    \param[in]     blocks   split reading in this number of blocks

    Create a thread to read data from a file. The amount of data to be read is
    split into a given number of blocks. After a block is read, a signal is
    sent. If a thread is already running, the method blocks until the thread
    is finished reading.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::readPar(T * const data, const uint64 _size,
                        const unsigned short int blocks)
 { readPar(0, false, data, _size, blocks);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a thread to read data from a file starting at offset.
    \param[in]     offset   offset into file
    \param[in,out] data     pointer to buffer for data
    \param[in]     _size    number of data elements to read

    Create a thread to read data from a file starting at offset. If a thread
    is already running, the method blocks until the thread is finished reading.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::readPar(const uint64 offset, T * const data,
                        const uint64 _size)
 { readPar(offset, true, data, _size, 1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a thread to read data from a file in blocks starting at
           offset.
    \param[in]     offset   offset into file
    \param[in,out] data     pointer to buffer for data
    \param[in]     _size    number of data elements to read
    \param[in]     blocks   split reading in this number of blocks

    Create a thread to read data from a file starting at offset. The amount of
    data to be read is split into a given number of blocks. After a block is
    read, a signal is sent. If a thread is already running, the method blocks
    until the thread is finished reading.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::readPar(const uint64 offset, T * const data,
                        const uint64 _size, const unsigned short int blocks)
 { readPar(offset, true, data, _size, blocks);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a thread to read data from a file in blocks starting at
           offset.
    \param[in]     offset       offset into file
    \param[in]     use_offset   use the offset ? (otherwise read from current
                                                  position)
    \param[in,out] data         pointer to buffer for data
    \param[in]     _size        number of data elements to read
    \param[in]     blocks       split reading in this number of blocks

    Create a thread to read data from a file starting at offset. The amount of
    data to be read is split into a given number of blocks. After a block is
    read, a signal is sent. If a thread is already running, the method blocks
    until the thread is finished reading.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::readPar(const uint64 offset, const bool use_offset,
                        T * const data, const uint64 _size,
                        const unsigned short int blocks)
 { if ((file == NULL) || write_object) return;
   try
   { void *thread_result;

     try
     {                            // wait until last reading-thread is finished
       if (thread_running)
        { ThreadJoin(tid, &thread_result);
          thread_running=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
                                                            // delete semaphore
       if (block_finished_sem != NULL) delete block_finished_sem;
       block_finished_sem=new Semaphore(0);         // ... and create a new one
                        // fill parameter structure for thread and start thread
       tp.type_name=typeid(T).name();
       tp.object=this;
       tp.data=data;
       tp.size=_size;
       tp.blocks=blocks;
       tp.offset=offset;
       tp.use_offset=use_offset;
#ifdef XEON_HYPERTHREADING_BUG
       tp.thread_number=0;
#endif
       thread_running=true;
       // ahc kind of a guess 11/20/14
       // ThreadCreate(&tid, executeThread_ReadFileOffs, &tp);
       ThreadCreate(&tid, executeThread_ReadFileOffs(), &tp);
     }
     catch (...)
      { thread_result=NULL;
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
        if (thread_running)
         { ThreadJoin(tid, &thread_result);
           thread_running=false;
         }
        if (block_finished_sem != NULL) { delete block_finished_sem;
                                          block_finished_sem=NULL;
                                        }
                    // We might catch an exception of the terminating thread.
                    // In this case we throw the new exception from the thread,
                    // not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Move file pointer relative to current position.
    \param[in] offset   number of data elements to skip

    Move file pointer relative to current position.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::seekr(const signed long long int offset) const
 { if (file != NULL) fseek(file, offset*sizeof(T), SEEK_CUR);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request size of file.
    \return size of file in bytes

    Request size of file in bytes.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
uint64 RawIO <T>::size() const
 { if (file != NULL)
    { uint64 _size, pos;

      pos=ftell(file);
      fseek(file, 0, SEEK_END);
      _size=ftell(file);
      fseek(file, pos, SEEK_SET);
      return(_size);
    }
   return(0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check if next block is finished.
    \return is next block finished ?

    Check if next block is finished reading or writing.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
bool RawIO <T>::tryWait() const
 { if (block_finished_sem == NULL) return(true);
   return(block_finished_sem->tryWait());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Wait until next block is finished.

    Wait until next block is finished reading or writing.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::wait() const
 { if (block_finished_sem != NULL) block_finished_sem->wait();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write data to file.
    \param[in,out] data    pointer to data
    \param[in]     _size   number of data elements to write
    \exception REC_FILE_WRITE   can't store data in file

    Write data to file. If a thread is running, the method blocks until the
    thread is finished writing. The data is converted between little- and
    big-endian as necessary.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::write(T * const data, const uint64 _size)
 { try
   { void *thread_result;

     try
     {                            // wait until last reading-thread is finished
       if (thread_running)
        { ThreadJoin(tid, &thread_result);
          thread_running=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
                                                            // delete semaphore
       if (block_finished_sem != NULL) { delete block_finished_sem;
                                         block_finished_sem=NULL;
                                       }
       if (swap_data)
        for (uint64 i=0; i < _size; i++)
         Swap(&data[i]);
       if ((file != NULL) && write_object)
        { uint64 bytes;
          char *dp;

          bytes=_size*sizeof(T);
          dp=(char *)data;
          do { uint64 written, s;

               if (bytes < MBYTE) s=bytes;
                else s=MBYTE;
               written=fwrite(dp, 1, (size_t)s, file);
               if (written > 0) { bytes-=written;
                                  dp+=written;
                                }
                else throw Exception(REC_FILE_WRITE,
                                     "Can't store data in the file '#1'. File "
                                     "system full ?").arg(filename);
             } while (bytes > 0);
        }
       if (swap_data)
        for (uint64 i=0; i < _size; i++)
         Swap(&data[i]);
     }
     catch (...)
      { thread_result=NULL;
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
        if (thread_running)
         { ThreadJoin(tid, &thread_result);
           thread_running=false;
         }
        if (block_finished_sem != NULL) { delete block_finished_sem;
                                          block_finished_sem=NULL;
                                        }
                    // We might catch an exception of the terminating thread.
                    // In this case we throw the new exception from the thread,
                    // not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write data to file in blocks.
    \param[in,out] data     pointer to buffer for data
    \param[in]     _size    number of data elements to read
    \param[in]     blocks   split writing in this number of blocks
    \exception REC_FILE_WRITE   can't store data in file

    Write data to file in blocks. The amount of data to be written is split
    into a given number of blocks. After a block is written, a signal is sent.
    The data is converted between little- and big-endian as necessary.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::writeFile(T *data, const uint64 _size,
                          const unsigned short int blocks)
 { uint64 blocksize;
                                             // number of elements in one block
   blocksize=_size/(uint64)blocks;
   for (unsigned short int i=0; i < blocks; i++,
        data+=blocksize)
    { if (swap_data)
       for (uint64 j=0; j < blocksize; j++)
        Swap(&data[j]);
                                                                 // write block
      if ((file != NULL) && write_object)
       { uint64 bytes;
         char *dp;

         bytes=blocksize*sizeof(T);
         dp=(char *)data;
         do { uint64 written;

              written=fwrite(dp, 1, std::min(bytes, MBYTE), file);
              if (written > 0) { bytes-=written;
                                 dp+=written;
                               }
               else throw Exception(REC_FILE_WRITE,
                                    "Can't store data in the file '#1'. File "
                                    "system full ?").arg(filename);
            } while (bytes > 0);
       }
      if (swap_data)
       for (uint64 j=0; j < blocksize; j++)
        Swap(&data[j]);
      block_finished_sem->signal();                // signal that block is done
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a thread to write data to a file.
    \param[in] data    pointer to buffer for data
    \param[in] _size   number of data elements to write

    Create a thread to write data from to file. If a thread is already running,
    the method blocks until the thread is finished writing.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::writePar(T * const data, const uint64 _size)
 { writePar(data, _size, 1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a thread to write data to a file in blocks.
    \param[in] data     pointer to buffer for data
    \param[in] _size    number of data elements to write
    \param[in] blocks   split writing in this number of blocks

    Create a thread to write data to a file starting at offset. The amount of
    data to be written is split into a given number of blocks. After a block is
    written, a signal is sent. If a thread is already running, the method
    blocks until the thread is finished writing.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RawIO <T>::writePar(T * const data, const uint64 _size,
                         const unsigned short int blocks)
 { if ((file == NULL) || !write_object) return;
   try
   { void *thread_result;

     try
     {                            // wait until last writing thread is finished
       if (thread_running)
        { ThreadJoin(tid, &thread_result);
          thread_running=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
                                                            // delete semaphore
       if (block_finished_sem != NULL) delete block_finished_sem;
       block_finished_sem=new Semaphore(0);           // ... and create new one
                        // fill parameter structure for thread and start thread
       tp.type_name=typeid(T).name();
       tp.object=this;
       tp.data=data;
       tp.size=_size;
       tp.blocks=blocks;
#ifdef XEON_HYPERTHREADING_BUG
       tp.thread_number=0;
#endif
       thread_running=true;
       // ahc
       // ThreadCreate(&tid, executeThread_WriteFile, &tp);
       ThreadCreate(&tid, executeThread_WriteFile(), &tp);
     }
     catch (...)
      { thread_result=NULL;
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
        if (thread_running)
         { ThreadJoin(tid, &thread_result);
               // we might have caught an exception from the terminating thread
           thread_running=false;
         }
        if (block_finished_sem != NULL) { delete block_finished_sem;
                                          block_finished_sem=NULL;
                                        }
                    // if the thread has thrown an exception we throw this one,
                    // not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }
